#ifndef JACKDRIVER_H
#define JACKDRIVER_H

#include <jack/jack.h>
#include <jack/ringbuffer.h>

#include <regex.h>

typedef struct _jseq
{
  char *client_name;
  jack_ringbuffer_t **ringbuffer_out;
  jack_ringbuffer_t **ringbuffer_in;
  jack_client_t	*jack_client;
  jack_port_t	**output_port;
  jack_port_t	**input_port;
  uint8_t n_in, n_out, passthrough[2];
  char *in[2], *out[2];
  regex_t inre[2], outre[2];
} JACK_SEQ;

extern int jack_quit;
// This is supposed to be set properly by main().
extern char *jack_command_line;

int init_jack(JACK_SEQ* seq, uint8_t verbose);
void process_connections(JACK_SEQ* seq);
void close_jack(JACK_SEQ* seq);
void queue_midi(void* seqq, uint8_t msg[], uint8_t port_no);
int pop_midi(void* seqq, uint8_t msg[], uint8_t *port_no);

#endif
