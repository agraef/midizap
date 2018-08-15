/* AG: This is a trimmed-down version of the Jack MIDI driver pilfered from
   Spencer Jackson's osc2midi program, cf. https://github.com/ssj71/OSC2MIDI. */

/*-
 * Copyright (c) 2014 Spencer Jackson <ssjackson71@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <sysexits.h>
#include <errno.h>
#include <jack/jack.h>
#include <jack/midiport.h>
#include <jack/ringbuffer.h>
#include <jack/session.h>
#include "jackdriver.h"


typedef struct _MidiMessage
{
    jack_nframes_t	time;
    int		len;	/* Length of MIDI message, in bytes. */
    uint8_t	data[3];
} MidiMessage;

#define RINGBUFFER_SIZE		16384*sizeof(MidiMessage)

/* Will emit a warning if time between jack callbacks is longer than this. */
#define MAX_TIME_BETWEEN_CALLBACKS	0.1

/* Will emit a warning if execution of jack callback takes longer than this. */
#define MAX_PROCESSING_TIME	0.01


///////////////////////////////////////////////
//These functions operate in the JACK RT Thread
///////////////////////////////////////////////

double
get_time(void)
{
    double seconds;
    int ret;
    struct timeval tv;

    ret = gettimeofday(&tv, NULL);

    if (ret)
    {
        perror("gettimeofday");
        exit(EX_OSERR);
    }

    seconds = tv.tv_sec + tv.tv_usec / 1000000.0;

    return (seconds);
}

double
get_delta_time(void)
{
    static double previously = -1.0;
    double now;
    double delta;

    now = get_time();

    if (previously == -1.0)
    {
        previously = now;

        return (0);
    }

    delta = now - previously;
    previously = now;

    assert(delta >= 0.0);

    return (delta);
}


double
nframes_to_ms(jack_client_t* jack_client,jack_nframes_t nframes)
{
    jack_nframes_t sr;

    sr = jack_get_sample_rate(jack_client);

    assert(sr > 0);

    return ((nframes * 1000.0) / (double)sr);
}

void
queue_message(jack_ringbuffer_t* ringbuffer, MidiMessage *ev)
{
    int written;

    if (jack_ringbuffer_write_space(ringbuffer) < sizeof(*ev))
    {
        fprintf(stderr, "Not enough space in the ringbuffer, MIDI LOST.\n");
        return;
    }

    written = jack_ringbuffer_write(ringbuffer, (char *)ev, sizeof(*ev));

    if (written != sizeof(*ev))
        fprintf(stderr, "jack_ringbuffer_write failed, MIDI LOST.\n");
}

void
process_midi_input(JACK_SEQ* seq,jack_nframes_t nframes)
{
  int k;

  for (k = 0; k < seq->n_in; k++) {

    int read, events, i;
    MidiMessage rev;
    jack_midi_event_t event;

    void *port_buffer = jack_port_get_buffer(seq->input_port[k], nframes);
    if (port_buffer == NULL)
    {
      fprintf(stderr, "jack_port_get_buffer failed, cannot receive anything.\n");
      return;
    }

#ifdef JACK_MIDI_NEEDS_NFRAMES
    events = jack_midi_get_event_count(port_buffer, nframes);
#else
    events = jack_midi_get_event_count(port_buffer);
#endif

    for (i = 0; i < events; i++)
    {

#ifdef JACK_MIDI_NEEDS_NFRAMES
      read = jack_midi_event_get(&event, port_buffer, i, nframes);
#else
      read = jack_midi_event_get(&event, port_buffer, i);
#endif
      if (!read)
      {
	//successful event get

	if (event.size <= 3 && event.size >= 1)
	{
	  //not sysex or something

	  //PUSH ONTO CIRCULAR BUFFER
	  //not sure if its a true copy onto buffer, if not this won't work
	  rev.len = event.size;
	  rev.time = event.time;
	  memcpy(rev.data, event.buffer, rev.len);
	  queue_message(seq->ringbuffer_in[k],&rev);
	}
      }

    }
  }
}

void
process_midi_output(JACK_SEQ* seq,jack_nframes_t nframes)
{
  jack_nframes_t last_frame_time = jack_last_frame_time(seq->jack_client);
  int k;

  for (k = 0; k < seq->n_out; k++) {
      
    int read, t;
    uint8_t *buffer;
    void *port_buffer;
    MidiMessage ev;

    port_buffer = jack_port_get_buffer(seq->output_port[k], nframes);
    if (port_buffer == NULL)
    {
      fprintf(stderr, "jack_port_get_buffer failed, cannot send anything.\n");
      return;
    }

#ifdef JACK_MIDI_NEEDS_NFRAMES
    jack_midi_clear_buffer(port_buffer, nframes);
#else
    jack_midi_clear_buffer(port_buffer);
#endif

    while (jack_ringbuffer_read_space(seq->ringbuffer_out[k]))
    {
      read = jack_ringbuffer_peek(seq->ringbuffer_out[k], (char *)&ev, sizeof(ev));

      if (read != sizeof(ev))
      {
	//warn_from_jack_thread_context("Short read from the ringbuffer, possible note loss.");
	jack_ringbuffer_read_advance(seq->ringbuffer_out[k], read);
	continue;
      }

      t = ev.time + nframes - last_frame_time;

      /* If computed time is too much into the future, we'll need
	 to send it later. */
      if (t >= (int)nframes)
	break;

      /* If computed time is < 0, we missed a cycle because of xrun. */
      if (t < 0)
	t = 0;

      jack_ringbuffer_read_advance(seq->ringbuffer_out[k], sizeof(ev));

#ifdef JACK_MIDI_NEEDS_NFRAMES
      buffer = jack_midi_event_reserve(port_buffer, t, ev.len, nframes);
#else
      buffer = jack_midi_event_reserve(port_buffer, t, ev.len);
#endif

      if (buffer == NULL)
      {
	//warn_from_jack_thread_context("jack_midi_event_reserve failed, NOTE LOST.");
	break;
      }

      memcpy(buffer, ev.data, ev.len);
    }
  }
}

int
process_callback(jack_nframes_t nframes, void *seqq)
{
    JACK_SEQ* seq = (JACK_SEQ*)seqq;
#ifdef MEASURE_TIME
    if (get_delta_time() > MAX_TIME_BETWEEN_CALLBACKS)
        fprintf(stderr, "Had to wait too long for JACK callback; scheduling problem?\n");
#endif

    if(seq->n_in)
        process_midi_input( seq,nframes );
    if(seq->n_out)
        process_midi_output( seq,nframes );

#ifdef MEASURE_TIME
    if (get_delta_time() > MAX_PROCESSING_TIME)
        fprintf(stderr, "Processing took too long; scheduling problem?\n");
#endif

    return (0);
}

///////////////////////////////////////////////
//these functions are executed in other threads
///////////////////////////////////////////////
void queue_midi(void* seqq, uint8_t msg[], uint8_t port_no)
{
    MidiMessage ev;
    JACK_SEQ* seq = (JACK_SEQ*)seqq;
    ev.len = 3;

    // At least with JackOSX, Jack will transmit the bytes verbatim, so make
    // sure that we look at the status byte and trim the message accordingly,
    // in order not to transmit any invalid MIDI data.
    switch (msg[0] & 0xf0)
    {
    case 0x80:
    case 0x90:
    case 0xa0:
    case 0xb0:
    case 0xe0:
        break; // 2 data bytes
    case 0xc0:
    case 0xd0:
        ev.len = 2; // 1 data byte
        break;
    case 0xf0: // system message
        switch (msg[0])
        {
        case 0xf2:
            break; // 2 data bytes
        case 0xf1:
        case 0xf3:
            ev.len = 2; // 1 data byte
            break;
        case 0xf6:
        case 0xf8:
        case 0xf9:
        case 0xfa:
        case 0xfb:
        case 0xfc:
        case 0xfe:
        case 0xff:
            ev.len = 1; // no data byte
            break;
        default:
            // ignore unknown (most likely sysex)
            return;
        }
        break;
    default:
        return; // not a valid MIDI message, bail out
    }

    ev.data[0] = msg[0];
    ev.data[1] = msg[1];
    ev.data[2] = msg[2];

    ev.time = jack_frame_time(seq->jack_client);
    queue_message(seq->ringbuffer_out[port_no],&ev);
}

int pop_midi(void* seqq, uint8_t msg[], uint8_t *port_no)
{
  int read, k;
  MidiMessage ev;
  JACK_SEQ* seq = (JACK_SEQ*)seqq;

  for (k = 0; k < seq->n_in; k++) {

    if (jack_ringbuffer_read_space(seq->ringbuffer_in[k]))
    {
      read = jack_ringbuffer_peek(seq->ringbuffer_in[k], (char *)&ev, sizeof(ev));

      if (read != sizeof(ev))
      {
	//warn_from_jack_thread_context("Short read from the ringbuffer, possible note loss.");
	jack_ringbuffer_read_advance(seq->ringbuffer_in[k], read);
	return -1;
      }

      jack_ringbuffer_read_advance(seq->ringbuffer_in[k], sizeof(ev));

      memcpy(msg,ev.data,ev.len);
      *port_no = k;

      return ev.len;
    }
  }
  return 0;
}

int jack_quit;

void
shutdown_callback()
{
  // we can't do anything fancy here, just ping the main thread
  jack_quit = -1;
}

char *jack_command_line = "midizap";

void
session_callback(jack_session_event_t *event, void *seqq)
{
  JACK_SEQ* seq = (JACK_SEQ*)seqq;
  // XXXTODO: In order to better support Jack session management in the future
  // we may want to copy over the loaded midizaprc file and store it in the
  // session dir, so that we can reload it from there later. For the time
  // being, we simply record the command line here.
  //printf("path %s, uuid %s, type: %s\n", event->session_dir, event->client_uuid, event->type == JackSessionSave ? "save" : "quit");

  event->command_line = strdup(jack_command_line);
  jack_session_reply(seq->jack_client, event);

  if (event->type == JackSessionSaveAndQuit) {
    jack_quit = 1;
  }

  jack_session_event_free (event);
}

////////////////////////////////
//this is run in the main thread
////////////////////////////////
int
init_jack(JACK_SEQ* seq, uint8_t verbose)
{
    int err, k;
    char portname[100],
      *client_name = seq->client_name?seq->client_name:"midizap";
    jack_status_t status;

    if(verbose)printf("opening client...\n");
    seq->jack_client = jack_client_open(client_name, JackNullOption, &status);

    if (seq->jack_client == NULL)
    {
      fprintf(stderr, "Could not connect to the JACK server; run jackd first?\n");
      return 0;
    }

    if (verbose && (status & JackServerStarted)) {
      printf("JACK server started\n");
    }
    if (verbose && status & JackNameNotUnique) {
      client_name = jack_get_client_name(seq->jack_client);
      printf("JACK client name changed to: %s\n", client_name);
    }

    if(verbose)printf("assigning shutdown callback...\n");
    jack_on_shutdown(seq->jack_client, shutdown_callback, (void*)seq);

    if(verbose)printf("assigning session callback...\n");
    jack_set_session_callback(seq->jack_client, session_callback, (void*)seq);

    if(verbose)printf("assigning process callback...\n");
    err = jack_set_process_callback(seq->jack_client, process_callback, (void*)seq);
    if (err)
    {
      fprintf(stderr, "Could not register JACK process callback.\n");
      return 0;
    }


    seq->ringbuffer_in = NULL;
    seq->input_port = NULL;
    if(seq->n_in)
    {

      if(verbose)printf("initializing JACK input: \ncreating ringbuffer...\n");
      seq->ringbuffer_in = calloc(seq->n_in, sizeof(jack_ringbuffer_t*));
      seq->input_port = calloc(seq->n_in, sizeof(jack_port_t*));
      if (!seq->ringbuffer_in || !seq->input_port)
      {
	fprintf(stderr, "Cannot allocate memory for ports and ringbuffers.\n");
	return 0;
      }

      for (k = 0; k < seq->n_in; k++) {
	seq->ringbuffer_in[k] = jack_ringbuffer_create(RINGBUFFER_SIZE);

	if (seq->ringbuffer_in[k] == NULL)
	{
	  fprintf(stderr, "Cannot create JACK ringbuffer.\n");
	  return 0;
	}

	jack_ringbuffer_mlock(seq->ringbuffer_in[k]);

	if (k)
	  sprintf(portname, "midi_in%d", k+1);
	else
	  strcpy(portname, "midi_in");
	seq->input_port[k] = jack_port_register(seq->jack_client, portname,
						JACK_DEFAULT_MIDI_TYPE,
						JackPortIsInput, 0);

	if (seq->input_port[k] == NULL)
	{
	  fprintf(stderr, "Could not register JACK port.\n");
	  return 0;
	}
      }
    }

    seq->ringbuffer_out = NULL;
    seq->output_port = NULL;
    if(seq->n_out)
    {

      if(verbose)printf("initializing JACK output: \ncreating ringbuffer...\n");
      seq->ringbuffer_out = calloc(seq->n_out, sizeof(jack_ringbuffer_t*));
      seq->output_port = calloc(seq->n_out, sizeof(jack_port_t*));
      if (!seq->ringbuffer_out || !seq->output_port)
      {
	fprintf(stderr, "Cannot allocate memory for ports and ringbuffers.\n");
	return 0;
      }

      for (k = 0; k < seq->n_out; k++) {
	seq->ringbuffer_out[k] = jack_ringbuffer_create(RINGBUFFER_SIZE);

	if (seq->ringbuffer_out[k] == NULL)
	{
	  fprintf(stderr, "Cannot create JACK ringbuffer.\n");
	  return 0;
	}

	jack_ringbuffer_mlock(seq->ringbuffer_out[k]);

	if (k)
	  sprintf(portname, "midi_out%d", k+1);
	else
	  strcpy(portname, "midi_out");
	seq->output_port[k] = jack_port_register(seq->jack_client, portname,
						 JACK_DEFAULT_MIDI_TYPE,
						 JackPortIsOutput, 0);

	if (seq->output_port[k] == NULL)
	{
	  fprintf(stderr, "Could not register JACK port.\n");
	  return 0;
	}
      }
    }

    if (jack_activate(seq->jack_client))
    {
        fprintf(stderr, "Cannot activate JACK client.\n");
        return 0;
    }
    return 1;
}

void close_jack(JACK_SEQ* seq)
{
  int k;
  if(seq->n_out) {
    for (k = 0; k < seq->n_out; k++)
      jack_ringbuffer_free(seq->ringbuffer_out[k]);
  }
  if(seq->n_in) {
    for (k = 0; k < seq->n_in; k++)
      jack_ringbuffer_free(seq->ringbuffer_in[k]);
  }
  jack_client_close(seq->jack_client);
}
