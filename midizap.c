
/*

 Copyright 2013 Eric Messick (FixedImagePhoto.com/Contact)
 Copyright 2018 Albert Graef <aggraef@gmail.com>

 Based on a version (c) 2006 Trammell Hudson <hudson@osresearch.net>

 which was in turn

 Based heavily on code by Arendt David <admin@prnet.org>

*/

#include "midizap.h"
#include "jackdriver.h"

typedef struct input_event EV;

Display *display;

JACK_SEQ seq;
int jack_num_outputs = 0, debug_jack = 0;
int shift = 0;

void
initdisplay(void)
{
  int event, error, major, minor;

  display = XOpenDisplay(0);
  if (!display) {
    fprintf(stderr, "unable to open X display\n");
    exit(1);
  }
  if (!XTestQueryExtension(display, &event, &error, &major, &minor)) {
    fprintf(stderr, "Xtest extensions not supported\n");
    XCloseDisplay(display);
    exit(1);
  }
}

void
send_button(unsigned int button, int press)
{
  XTestFakeButtonEvent(display, button, press ? True : False, DELAY);
}

void
send_key(KeySym key, int press)
{
  KeyCode keycode;

  if (key >= XK_Button_1 && key <= XK_Scroll_Down) {
    send_button((unsigned int)key - XK_Button_0, press);
    return;
  }
  keycode = XKeysymToKeycode(display, key);
  XTestFakeKeyEvent(display, keycode, press ? True : False, DELAY);
}

// cached controller and pitch bend values
static int16_t notevalue[16][128];
static int16_t ccvalue[16][128];
static int16_t kpvalue[16][128];
static int16_t cpvalue[16];
static int16_t pbvalue[16] =
  {8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
   8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192};

static int dataval(int val, int min, int max)
{
  if (!val || val > max)
    return max;
  else if (val < min)
    return min;
  else
    return val;
}

static int datavals(int val, int step, int *steps, int n_steps)
{
  if (val < 0)
    return -datavals(-val, step, steps, n_steps);
  else if (val < n_steps)
    return steps[val];
  else if (n_steps)
    return steps[n_steps-1];
  else if (step)
    return step*val;
  else
    return val;
}

void
handle_event(uint8_t *msg, uint8_t portno, int depth, int recursive);

void
send_midi(uint8_t portno, int status, int data,
	  int step, int n_steps, int *steps,
	  int incr, int index, int dir,
	  int mod, int mod_step, int mod_n_steps, int *mod_steps, int val,
	  int recursive, int depth)
{
  if (!jack_num_outputs) return; // MIDI output not enabled
  uint8_t msg[3];
  int chan = status & 0x0f;
  msg[0] = status;
  msg[1] = data;
  switch (status & 0xf0) {
  case 0x90:
    if (dir) {
      // increment (dir==1) or decrement (dir==-1) the current value,
      // clamping it to the 0..127 data byte range
      if (!step) step = 1;
      dir *= step;
      if (dir > 0) {
	if (notevalue[chan][data] >= 127) return;
	notevalue[chan][data] += dir;
	if (notevalue[chan][data] > 127) notevalue[chan][data] = 127;
      } else {
	if (notevalue[chan][data] == 0) return;
	notevalue[chan][data] += dir;
	if (notevalue[chan][data] < 0) notevalue[chan][data] = 0;
      }
      msg[2] = notevalue[chan][data];
    } else if (mod) {
      int d = msg[1] + datavals(val/mod, mod_step, mod_steps, mod_n_steps);
      int v = datavals(val%mod, step, steps, n_steps);
      if (d > 127 || d < 0) return;
      if (v > 127 || v < 0) return;
      msg[1] = d;
      msg[2] = v;
    } else if (!index) {
      msg[2] = dataval(step, 0, 127);
    } else {
      msg[2] = 0;
    }
    break;
  case 0xb0:
    if (dir) {
      if (incr) {
	// incremental controller, simply spit out a relative sign bit value
	if (!step) step = 1;
	dir *= step;
	if (dir < -63) dir = -63;
	if (dir > 63) dir = 63;
	msg[2] = dir>0?dir:dir<0?64-dir:0;
      } else {
	// increment (dir==1) or decrement (dir==-1) the current value,
	// clamping it to the 0..127 data byte range
	if (!step) step = 1;
	dir *= step;
	if (dir > 0) {
	  if (ccvalue[chan][data] >= 127) return;
	  ccvalue[chan][data] += dir;
	  if (ccvalue[chan][data] > 127) ccvalue[chan][data] = 127;
	} else {
	  if (ccvalue[chan][data] == 0) return;
	  ccvalue[chan][data] += dir;
	  if (ccvalue[chan][data] < 0) ccvalue[chan][data] = 0;
	}
	msg[2] = ccvalue[chan][data];
      }
    } else if (mod) {
      int d = msg[1] + datavals(val/mod, mod_step, mod_steps, mod_n_steps);
      int v = datavals(val%mod, step, steps, n_steps);
      if (d > 127 || d < 0) return;
      if (v > 127 || v < 0) return;
      msg[1] = d;
      msg[2] = v;
    } else if (!index) {
      msg[2] = dataval(step, 0, 127);
    } else {
      msg[2] = 0;
    }
    break;
  case 0xa0:
    if (dir) {
      // increment (dir==1) or decrement (dir==-1) the current value,
      // clamping it to the 0..127 data byte range
      if (!step) step = 1;
      dir *= step;
      if (dir > 0) {
	if (kpvalue[chan][data] >= 127) return;
	kpvalue[chan][data] += dir;
	if (kpvalue[chan][data] > 127) kpvalue[chan][data] = 127;
      } else {
	if (kpvalue[chan][data] == 0) return;
	kpvalue[chan][data] += dir;
	if (kpvalue[chan][data] < 0) kpvalue[chan][data] = 0;
      }
      msg[2] = kpvalue[chan][data];
    } else if (mod) {
      int d = msg[1] + datavals(val/mod, mod_step, mod_steps, mod_n_steps);
      int v = datavals(val%mod, step, steps, n_steps);
      if (d > 127 || d < 0) return;
      if (v > 127 || v < 0) return;
      msg[1] = d;
      msg[2] = v;
    } else if (!index) {
      msg[2] = dataval(step, 0, 127);
    } else {
      msg[2] = 0;
    }
    break;
  case 0xd0:
    if (dir) {
      // increment (dir==1) or decrement (dir==-1) the current value,
      // clamping it to the 0..127 data byte range
      if (!step) step = 1;
      dir *= step;
      if (dir > 0) {
	if (cpvalue[chan] >= 127) return;
	cpvalue[chan] += dir;
	if (cpvalue[chan] > 127) cpvalue[chan] = 127;
      } else {
	if (cpvalue[chan] == 0) return;
	cpvalue[chan] += dir;
	if (cpvalue[chan] < 0) cpvalue[chan] = 0;
      }
      msg[1] = cpvalue[chan];
    } else if (mod) {
      int v = datavals(val%mod, step, steps, n_steps);
      if (v > 127 || v < 0) return;
      msg[1] = v;
    } else if (!index) {
      msg[1] = dataval(step, 0, 127);
    } else {
      msg[1] = 0;
    }
    break;
  case 0xe0: {
    // pitch bends are treated similarly to a controller, but with a 14 bit
    // range (0..16383, with 8192 being the center value)
    int pbval = 0;
    if (dir) {
      if (!step) step = 1;
      dir *= step;
      if (dir > 0) {
	if (pbvalue[chan] >= 16383) return;
	pbvalue[chan] += dir;
	if (pbvalue[chan] > 16383) pbvalue[chan] = 16383;
      } else {
	if (pbvalue[chan] == 0) return;
	pbvalue[chan] += dir;
	if (pbvalue[chan] < 0) pbvalue[chan] = 0;
      }
      pbval = pbvalue[chan];
    } else if (mod) {
      int v = datavals(val%mod, step, steps, n_steps);
      if (v > 8191 || v < -8192) return;
      pbval = 8192+v;
    } else if (!index) {
      pbval = 8192+dataval(step, -8192, 8191);
    } else {
      // we use 8192 (center) as the "home" (a.k.a. "off") value, so the pitch
      // will only bend up, never down below the center value
      pbval = 8192;
    }
    // the result is a 14 bit value which gets encoded as a combination of two
    // 7 bit values which become the data bytes of the message
    msg[1] = pbval & 0x7f; // LSB (lower 7 bits)
    msg[2] = pbval >> 7;   // MSB (upper 7 bits)
    break;
  }
  case 0xc0:
    if (mod) {
      int d = msg[1] + datavals(val/mod, mod_step, mod_steps, mod_n_steps);
      if (d > 127 || d < 0) return;
      msg[1] = d;
    }
    // just send the message
    break;
  default:
    return;
  }
  if (!recursive)
    queue_midi(&seq, msg, portno);
  else
    handle_event(msg, portno, depth+1, recursive);
}

static int stroke_data_cmp(const void *a, const void *b)
{
  const stroke_data *ad = (const stroke_data*)a;
  const stroke_data *bd = (const stroke_data*)b;
  if (ad->chan == bd->chan)
    return ad->data - bd->data;
  else
    return ad->chan - bd->chan;
}

static stroke *find_stroke_data(stroke_data *sd,
				int chan, int data, int index,
				int *step, int *n_steps, int **steps,
				int *incr, int *mod,
				uint16_t n)
{
  if (n < 16) {
    // Linear search is presumably faster for small arrays, and we also avoid
    // function calls for doing the comparisons here. Not sure where it breaks
    // even with glibc's bsearch(), though (TODO: measure).
    uint16_t i;
    for (i = 0; i < n; i++) {
      if (sd[i].chan == chan && sd[i].data == data) {
	if (step) *step = sd[i].step[index];
	if (n_steps) *n_steps = sd[i].n_steps[index];
	if (steps) *steps = sd[i].steps[index];
	if (incr) *incr = sd[i].is_incr;
	if (mod) *mod = sd[i].mod;
	return sd[i].s[index];
      } else if (sd[i].chan > chan ||
		 (sd[i].chan == chan && sd[i].data > data))
	return NULL;
    }
    return NULL;
  } else {
    // binary search from libc
    stroke_data *ret, key;
    key.chan = chan; key.data = data;
    ret = bsearch(&key, sd, n, sizeof(stroke_data), stroke_data_cmp);
    if (ret) {
      if (step) *step = ret->step[index];
      if (n_steps) *n_steps = ret->n_steps[index];
      if (steps) *steps = ret->steps[index];
      if (incr) *incr = ret->is_incr;
      if (mod) *mod = ret->mod;
      return ret->s[index];
    } else
      return NULL;
  }
}

static stroke *find_note(translation *tr, int shift,
			 int chan, int data, int index, int *mod,
			 int *step, int *n_steps, int **steps)
{
  return find_stroke_data(tr->note[shift], chan, data, index,
			  step, n_steps, steps, 0, mod,
			  tr->n_note[shift]);
}

static stroke *find_notes(translation *tr, int shift,
			int chan, int data, int index, int *step)
{
  return find_stroke_data(tr->notes[shift], chan, data, index, step, 0, 0, 0, 0,
			  tr->n_notes[shift]);
}

static stroke *find_pc(translation *tr, int shift,
		       int chan, int data, int index)
{
  return find_stroke_data(tr->pc[shift], chan, data, index, 0, 0, 0, 0, 0,
			  tr->n_pc[shift]);
}

static stroke *find_cc(translation *tr, int shift,
		       int chan, int data, int index, int *mod,
		       int *step, int *n_steps, int **steps)
{
  return find_stroke_data(tr->cc[shift], chan, data, index,
			  step, n_steps, steps, 0, mod,
			  tr->n_cc[shift]);
}

static stroke *find_ccs(translation *tr, int shift,
			int chan, int data, int index, int *step, int *incr)
{
  return find_stroke_data(tr->ccs[shift], chan, data, index, step, 0, 0,
			  incr, 0,
			  tr->n_ccs[shift]);
}

static stroke *find_kp(translation *tr, int shift,
		       int chan, int data, int index, int *mod,
		       int *step, int *n_steps, int **steps)
{
  return find_stroke_data(tr->kp[shift], chan, data, index,
			  step, n_steps, steps, 0, mod,
			  tr->n_kp[shift]);
}

static stroke *find_kps(translation *tr, int shift,
			int chan, int data, int index, int *step)
{
  return find_stroke_data(tr->kps[shift], chan, data, index, step, 0, 0, 0, 0,
			  tr->n_kps[shift]);
}

static stroke *find_cp(translation *tr, int shift,
		       int chan, int index, int *mod,
		       int *step, int *n_steps, int **steps)
{
  return find_stroke_data(tr->cp[shift], chan, 0, index,
			  step, n_steps, steps, 0, mod,
			  tr->n_cp[shift]);
}

static stroke *find_cps(translation *tr, int shift,
			int chan, int index, int *step)
{
  return find_stroke_data(tr->cps[shift], chan, 0, index, step, 0, 0, 0, 0,
			  tr->n_cps[shift]);
}

static stroke *find_pb(translation *tr, int shift,
		       int chan, int index, int *mod,
		       int *step, int *n_steps, int **steps)
{
  return find_stroke_data(tr->pb[shift], chan, 0, index,
			  step, n_steps, steps, 0, mod,
			  tr->n_pb[shift]);
}

static stroke *find_pbs(translation *tr, int shift,
			int chan, int index, int *step)
{
  return find_stroke_data(tr->pbs[shift], chan, 0, index, step, 0, 0, 0, 0,
			  tr->n_pbs[shift]);
}

stroke *
fetch_stroke(translation *tr, uint8_t portno, int status, int chan, int data,
	     int index, int dir, int *step, int *n_steps, int **steps,
	     int *incr, int *mod)
{
  if (tr && tr->portno == portno) {
    switch (status) {
    case 0x90:
      if (dir)
	return find_notes(tr, shift, chan, data, dir>0, step);
      else
	return find_note(tr, shift, chan, data, index, mod, step, n_steps, steps);
    case 0xc0:
      return find_pc(tr, shift, chan, data, index);
    case 0xb0:
      if (dir)
	return find_ccs(tr, shift, chan, data, dir>0, step, incr);
      else
	return find_cc(tr, shift, chan, data, index, mod, step, n_steps, steps);
    case 0xa0:
      if (dir)
	return find_kps(tr, shift, chan, data, dir>0, step);
      else
	return find_kp(tr, shift, chan, data, index, mod, step, n_steps, steps);
    case 0xd0:
      if (dir)
	return find_cps(tr, shift, chan, dir>0, step);
      else
	return find_cp(tr, shift, chan, index, mod, step, n_steps, steps);
    case 0xe0:
      if (dir)
	return find_pbs(tr, shift, chan, dir>0, step);
      else
	return find_pb(tr, shift, chan, index, mod, step, n_steps, steps);
    default:
      return NULL;
    }
  } else
    return NULL;
}

#define MAX_WINNAME_SIZE 1024
static char last_window_name[MAX_WINNAME_SIZE];
static char last_window_class[MAX_WINNAME_SIZE];
static Window last_focused_window = 0;
static translation *last_window_translation = NULL, *last_translation = NULL;
static int last_window = 0;

void reload_callback(void)
{
  last_focused_window = 0;
  last_window_translation = last_translation = NULL;
  last_window = 0;
}

static void debug_section(translation *tr)
{
  // we do some caching of the last printed translation here, so that we don't
  // print the same message twice
  if (debug_regex && (!last_window || tr != last_translation)) {
    last_translation = tr;
    last_window = 1;
    if (tr) {
      printf("translation: %s for %s (class %s)\n",
	     tr->name, last_window_name, last_window_class);
    } else {
      printf("no translation found for %s (class %s)\n",
	     last_window_name, last_window_class);
    }
  }
}

static char *note_name(int n)
{
  static char *note_names[] = { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "G#", "A", "Bb", "B" };
  if (n < 0 && n%12)
    return note_names[12+n%12];
  else
    return note_names[n%12];
}

static int note_octave(int n)
{
  if (n < 0 && n%12)
    return n/12-1 + midi_octave;
  else
    return n/12 + midi_octave;
}

static char *debug_key(translation *tr, char *name,
		       int status, int chan, int data, int dir)
{
  char *prefix = shift?"^":"", *suffix = "";
  strcpy(name, "??");
  switch (status) {
  case 0x90: {
    int mod = 0, step, n_steps, *steps;
    if (tr) {
      if (dir) {
	step = 1;
	(void)find_notes(tr, shift, chan, data, dir>0, &step);
      } else
	(void)find_note(tr, shift, chan, data, 0, &mod, &step, &n_steps, &steps);
    }
    if (!dir)
      suffix = "";
    else
      suffix = (dir<0)?"-":"+";
    if (dir && step != 1)
      sprintf(name, "%s%s%d[%d]-%d%s", prefix, note_name(data),
	      note_octave(data), step, chan+1, suffix);
    else if (!dir && mod)
      if (step != 1)
	sprintf(name, "%s%s%d[%d][%d]-%d", prefix, note_name(data),
		note_octave(data), mod, step, chan+1);
      else if (n_steps) {
	sprintf(name, "%s%s%d[%d][", prefix, note_name(data),
	        note_octave(data), mod);
	int l = strlen(name);
	for (int i = 0; i < n_steps; i++, (l = strlen(name)))
	  sprintf(name+l, "%s%d", i?",":"", steps[i]);
	sprintf(name+l, "]-%d", chan);
      } else
	sprintf(name, "%s%s%d[%d]-%d", prefix, note_name(data),
		note_octave(data), mod, chan+1);
    else
      sprintf(name, "%s%s%d-%d%s", prefix, note_name(data),
	      note_octave(data), chan+1, suffix);
    break;
  }
  case 0xa0: {
    int step = 0, n_steps = 0, *steps = 0, mod = 0;
    if (tr) {
      if (dir) {
	step = 1;
	(void)find_kps(tr, shift, chan, data, dir>0, &step);
      } else
	(void)find_kp(tr, shift, chan, data, 0, &mod, &step, &n_steps, &steps);
    }
    if (!dir)
      suffix = "";
    else
      suffix = (dir<0)?"-":"+";
    if (dir && step != 1)
      sprintf(name, "%sKP:%s%d[%d]-%d%s", prefix, note_name(data),
	      note_octave(data), step, chan+1, suffix);
    else if (!dir && mod)
      if (step != 1)
	sprintf(name, "%sKP:%s%d[%d][%d]-%d", prefix, note_name(data),
		note_octave(data), mod, step, chan+1);
      else if (n_steps) {
	sprintf(name, "%sKP:%s%d[%d][", prefix, note_name(data),
	        note_octave(data), mod);
	int l = strlen(name);
	for (int i = 0; i < n_steps; i++, (l = strlen(name)))
	  sprintf(name+l, "%s%d", i?",":"", steps[i]);
	sprintf(name+l, "]-%d", chan);
      } else
	sprintf(name, "%sKP:%s%d[%d]-%d", prefix, note_name(data),
		note_octave(data), mod, chan+1);
    else
      sprintf(name, "%sKP:%s%d-%d%s", prefix, note_name(data),
	      note_octave(data), chan+1, suffix);
    break;
  }
  case 0xb0: {
    int step = 0, n_steps = 0, *steps = 0, mod = 0, is_incr = 0;
    if (tr) {
      if (dir) {
	step = 1;
	(void)find_ccs(tr, shift, chan, data, dir>0, &step, &is_incr);
      } else
	(void)find_cc(tr, shift, chan, data, 0, &mod, &step, &n_steps, &steps);
    }
    if (!dir)
      suffix = "";
    else if (is_incr)
      suffix = (dir<0)?"<":">";
    else
      suffix = (dir<0)?"-":"+";
    if (dir && step != 1)
      sprintf(name, "%sCC%d[%d]-%d%s", prefix, data, step, chan+1, suffix);
    else if (!dir && mod)
      if (step != 1)
	sprintf(name, "%sCC%d[%d][%d]-%d", prefix, data, mod, step, chan+1);
      else if (n_steps) {
	sprintf(name, "%sCC%d[%d][", prefix, data, mod);
	int l = strlen(name);
	for (int i = 0; i < n_steps; i++, (l = strlen(name)))
	  sprintf(name+l, "%s%d", i?",":"", steps[i]);
	sprintf(name+l, "]-%d", chan);
      } else
	sprintf(name, "%sCC%d[%d]-%d", prefix, data, mod, chan+1);
    else
      sprintf(name, "%sCC%d-%d%s", prefix, data, chan+1, suffix);
    break;
  }
  case 0xc0:
    sprintf(name, "%sPC%d-%d", prefix, data, chan+1);
    break;
  case 0xd0: {
    int step = 0, n_steps = 0, *steps = 0, mod = 0;
    if (tr) {
      if (dir) {
	step = 1;
	(void)find_cps(tr, shift, chan, dir>0, &step);
      } else
	(void)find_cp(tr, shift, chan, 0, &mod, &step, &n_steps, &steps);
    }
    if (!dir)
      suffix = "";
    else
      suffix = (dir<0)?"-":"+";
    if (dir && step != 1) 
      sprintf(name, "%sCP[%d]-%d%s", prefix, step, chan+1, suffix);
    else if (!dir && mod)
      if (step != 1)
	sprintf(name, "%sCP[%d][%d]-%d", prefix, mod, step, chan+1);
      else if (n_steps) {
	sprintf(name, "%sCP[%d][", prefix, mod);
	int l = strlen(name);
	for (int i = 0; i < n_steps; i++, (l = strlen(name)))
	  sprintf(name+l, "%s%d", i?",":"", steps[i]);
	sprintf(name+l, "]-%d", chan);
      } else
	sprintf(name, "%sCP[%d]-%d", prefix, mod, chan+1);
    else
      sprintf(name, "%sCP-%d%s", prefix, chan+1, suffix);
    break;
  }
  case 0xe0: {
    int step = 1;
    if (tr) (void)find_pbs(tr, shift, chan, dir>0, &step);
    if (!dir)
      suffix = "";
    else
      suffix = (dir<0)?"-":"+";
    if (dir && step != 1)
      sprintf(name, "%sPB[%d]-%d%s", prefix, step, chan+1, suffix);
    else
      sprintf(name, "%sPB-%d%s", prefix, chan+1, suffix);
    break;
  }
  default: // this can't happen
    break;
  }
  return name;
}

static void debug_input(int portno,
			int status, int chan, int data, int data2)
{
  char name[100];
  if (status == 0xe0)
    // translate LSB,MSB to a pitch bend value in the range -8192..8191
    data2 = ((data2 << 7) | data) - 8192;
  else if (status == 0xd0)
    data2 = data;
  if (status == 0xc0)
    printf("[%d] %s\n", portno,
	   debug_key(0, name, status, chan, data, 0));
  else if (status != 0xf0) // system messages ignored for now
    printf("[%d] %s value = %d\n", portno,
	   debug_key(0, name, status, chan, data, 0), data2);
}

// Some machinery to handle the debugging of section matches. This is
// necessary since some inputs may generate a lot of calls to send_strokes()
// without ever actually matching any output sequence at all. In such cases we
// want to prevent a cascade of useless debugging messages by handling the
// message printing in a lazy manner.

static int debug_state = 0, debug_count = 0;
static translation *debug_tr = NULL;

static void start_debug()
{
  // start a debugging section
  debug_state = debug_regex;
  debug_tr = NULL;
  debug_count = 0;
}

static void end_debug()
{
  // end a debugging section; if we still haven't matched an output sequence,
  // but processed any input at all, we print the last matched translation
  // section now anyway
  if (debug_state && debug_count) debug_section(debug_tr);
  debug_state = 0;
}

// maximum recursion depth
#define MAX_DEPTH 32

void
send_strokes(translation *tr, uint8_t portno, int status, int chan,
	     int data, int data2, int index, int dir, int depth)
{
  int nkeys = 0, step = 0, n_steps = 0, *steps = 0, is_incr = 0, mod = 0;
  stroke *s = fetch_stroke(tr, portno, status, chan, data, index, dir,
			   &step, &n_steps, &steps, &is_incr, &mod);
  // If there's no press/release translation, check whether we have got at
  // least the corresponding release/press translation, in order to prevent
  // spurious error messages if either the press or release translation just
  // happens to be empty.
  int chk = s ||
    (!dir && fetch_stroke(tr, portno, status, chan, data, !index, dir, 0, 0, 0, 0, 0));

  if (!s && jack_num_outputs) {
    // fall back to default MIDI translation
    tr = default_midi_translation[portno];
    s = fetch_stroke(tr, portno, status, chan, data, index, dir,
		     &step, &n_steps, &steps, &is_incr, &mod);
    chk = chk || s ||
      (!dir && fetch_stroke(tr, portno, status, chan, data, !index, dir, 0, 0, 0, 0, 0));
    // Ignore all MIDI input on the second port if no translation was found in
    // the [MIDI2] section (or the section is missing altogether).
    if (portno && !s) return;
  }

  if (!s) {
    // fall back to the default translation
    tr = default_translation;
    s = fetch_stroke(tr, portno, status, chan, data, index, dir,
		     &step, &n_steps, &steps, &is_incr, &mod);
    chk = chk || s ||
      (!dir && fetch_stroke(tr, portno, status, chan, data, !index, dir, 0, 0, 0, 0, 0));
  }

  if (debug_regex) {
    if (s) {
      // found a sequence, print the matching section now
      debug_section(tr);
      debug_state = 0;
    } else if (!chk) {
      // No matches yet. To prevent a cascade of spurious messages, we defer
      // printing the matched section for now and just record it instead; it
      // may then be printed later.
      debug_tr = tr;
      // record that we actually tried to process some input
      debug_count = 1;
    }
  }

  if (s && debug_keys) {
    char name[100];
    print_stroke_sequence(debug_key(tr, name, status, chan, data, dir),
			  (dir||mod)?"":index?"U":"D", s,
			  mod, step, n_steps, steps, data2);
  }
  while (s) {
    if (s->keysym) {
      send_key(s->keysym, s->press);
      nkeys++;
    } else if (s->shift) {
      // toggle shift status
      shift = !shift;
    } else {
      if (s->recursive && depth >= MAX_DEPTH) {
	char name[100];
	if (tr && tr->name)
	  fprintf(stderr, "Error: [%s]$%s: recursion too deep\n",
		  tr->name, debug_key(tr, name, status, chan, data, dir));
	else
	  fprintf(stderr, "Error: $%s: recursion too deep\n",
		  debug_key(tr, name, status, chan, data, dir));
      } else
	send_midi(portno, s->status, s->data, s->step, s->n_steps, s->steps,
		  s->incr, index, dir,
		  mod, step, n_steps, steps, data2, s->recursive, depth);
    }
    s = s->next;
  }
  // no need to flush the display if we didn't send any keys
  if (nkeys) {
    XFlush(display);
  }
}

char *
get_window_name(Window win)
{
  Atom prop = XInternAtom(display, "WM_NAME", False);
  Atom type;
  int form;
  unsigned long remain, len;
  unsigned char *list;

  if (XGetWindowProperty(display, win, prop, 0, 1024, False,
			 AnyPropertyType, &type, &form, &len, &remain,
			 &list) != Success) {
    fprintf(stderr, "XGetWindowProperty failed for window 0x%x\n", (int)win);
    return NULL;
  }

  return (char*)list;
}

char *
get_window_class(Window win)
{
  Atom prop = XInternAtom(display, "WM_CLASS", False);
  Atom type;
  int form;
  unsigned long remain, len;
  unsigned char *list;

  if (XGetWindowProperty(display, win, prop, 0, 1024, False,
			 AnyPropertyType, &type, &form, &len, &remain,
			 &list) != Success) {
    fprintf(stderr, "XGetWindowProperty failed for window 0x%x\n", (int)win);
    return NULL;
  }

  return (char*)list;
}

char *
walk_window_tree(Window win, char **window_class)
{
  char *window_name;
  Window root = 0;
  Window parent;
  Window *children;
  unsigned int nchildren;

  while (win != root) {
    window_name = get_window_name(win);
    if (window_name != NULL) {
      *window_class = get_window_class(win);
      return window_name;
    }
    if (XQueryTree(display, win, &root, &parent, &children, &nchildren)) {
      win = parent;
      XFree(children);
    } else {
      fprintf(stderr, "XQueryTree failed for window 0x%x\n", (int)win);
      return NULL;
    }
  }
  return NULL;
}

translation *
get_focused_window_translation()
{
  Window focus;
  int revert_to;
  char *window_name = NULL, *window_class = NULL;

  XGetInputFocus(display, &focus, &revert_to);
  if (focus != last_focused_window) {
    last_window = 0;
    last_focused_window = focus;
    window_name = walk_window_tree(focus, &window_class);
    last_window_translation = get_translation(window_name, window_class);
    if (window_name && *window_name) {
      strncpy(last_window_name, window_name, MAX_WINNAME_SIZE);
      last_window_name[MAX_WINNAME_SIZE-1] = 0;
    } else {
      strcpy(last_window_name, "Unnamed");;
    }
    if (window_class && *window_class) {
      strncpy(last_window_class, window_class, MAX_WINNAME_SIZE);
      last_window_class[MAX_WINNAME_SIZE-1] = 0;
    } else {
      strcpy(last_window_name, "Unnamed");;
    }
    if (window_name != NULL) {
      XFree(window_name);
    }
    if (window_class != NULL) {
      XFree(window_class);
    }
  }
  return last_window_translation;
}

static int8_t innotevalue[2][16][128];
static int8_t inccvalue[2][16][128];
static int8_t inkpvalue[2][16][128];
static int8_t incpvalue[2][16];
static int16_t inpbvalue[2][16] =
  {{8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192},
   {8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192}};

// If this option is enabled (-k on the command line), we make sure that each
// "key" (note, cc, pb) is "off" before we allow it to go "on" again. This is
// useful to eliminate double note-ons and the like, but interferes with the
// way some controllers work, so it is disabled by default.

static int keydown_tracker = 0;

static uint8_t innotedown[2][16][128];
static uint8_t inccdown[2][16][128];
static uint8_t inpbdown[2][16];
static uint8_t inkpdown[2][16][128];
static uint8_t incpdown[2][16];

int
check_notes(translation *tr, uint8_t portno, int chan, int data)
{
  if (tr && tr->portno == portno &&
      (find_notes(tr, shift, chan, data, 0, 0) ||
       find_notes(tr, shift, chan, data, 1, 0)))
    return 1;
  tr = default_midi_translation[portno];
  if (tr && tr->portno == portno &&
      (find_notes(tr, shift, chan, data, 0, 0) ||
       find_notes(tr, shift, chan, data, 1, 0)))
    return 1;
  tr = default_translation;
  if (tr && tr->portno == portno &&
      (find_notes(tr, shift, chan, data, 0, 0) ||
       find_notes(tr, shift, chan, data, 1, 0)))
    return 1;
  return 0;
}

int
get_note_step(translation *tr, uint8_t portno, int chan, int data, int dir)
{
  int step;
  if (tr && tr->portno == portno &&
      find_notes(tr, shift, chan, data, dir>0, &step))
    return step;
  tr = default_midi_translation[portno];
  if (tr && tr->portno == portno &&
      find_notes(tr, shift, chan, data, dir>0, &step))
    return step;
  tr = default_translation;
  if (tr && tr->portno == portno &&
      find_notes(tr, shift, chan, data, dir>0, &step))
    return step;
  return 1;
}

int
get_note_mod(translation *tr, uint8_t portno, int chan, int data)
{
  int mod;
  if (tr && tr->portno == portno &&
      find_note(tr, shift, chan, data, 0, &mod, 0, 0, 0))
    return mod;
  tr = default_midi_translation[portno];
  if (tr && tr->portno == portno &&
      find_note(tr, shift, chan, data, 0, &mod, 0, 0, 0))
    return mod;
  tr = default_translation;
  if (tr && tr->portno == portno &&
      find_note(tr, shift, chan, data, 0, &mod, 0, 0, 0))
    return mod;
  return 0;
}

int
check_incr(translation *tr, uint8_t portno, int chan, int data)
{
  int is_incr;
  if (tr && tr->portno == portno &&
      (find_ccs(tr, shift, chan, data, 0, 0, &is_incr) ||
       find_ccs(tr, shift, chan, data, 1, 0, &is_incr)))
    return is_incr;
  tr = default_midi_translation[portno];
  if (tr && tr->portno == portno &&
      (find_ccs(tr, shift, chan, data, 0, 0, &is_incr) ||
       find_ccs(tr, shift, chan, data, 1, 0, &is_incr)))
    return is_incr;
  tr = default_translation;
  if (tr && tr->portno == portno &&
      (find_ccs(tr, shift, chan, data, 0, 0, &is_incr) ||
       find_ccs(tr, shift, chan, data, 1, 0, &is_incr)))
    return is_incr;
  return 0;
}

int
check_ccs(translation *tr, uint8_t portno, int chan, int data)
{
  if (tr && tr->portno == portno &&
      (find_ccs(tr, shift, chan, data, 0, 0, 0) ||
       find_ccs(tr, shift, chan, data, 1, 0, 0)))
    return 1;
  tr = default_midi_translation[portno];
  if (tr && tr->portno == portno &&
      (find_ccs(tr, shift, chan, data, 0, 0, 0) ||
       find_ccs(tr, shift, chan, data, 1, 0, 0)))
    return 1;
  tr = default_translation;
  if (tr && tr->portno == portno &&
      (find_ccs(tr, shift, chan, data, 0, 0, 0) ||
       find_ccs(tr, shift, chan, data, 1, 0, 0)))
    return 1;
  return 0;
}

int
get_cc_step(translation *tr, uint8_t portno, int chan, int data, int dir)
{
  int step;
  if (tr && tr->portno == portno &&
      find_ccs(tr, shift, chan, data, dir>0, &step, 0))
    return step;
  tr = default_midi_translation[portno];
  if (tr && tr->portno == portno &&
      find_ccs(tr, shift, chan, data, dir>0, &step, 0))
    return step;
  tr = default_translation;
  if (tr && tr->portno == portno &&
      find_ccs(tr, shift, chan, data, dir>0, &step, 0))
    return step;
  return 1;
}

int
get_cc_mod(translation *tr, uint8_t portno, int chan, int data)
{
  int mod;
  if (tr && tr->portno == portno &&
      find_cc(tr, shift, chan, data, 0, &mod, 0, 0, 0))
    return mod;
  tr = default_midi_translation[portno];
  if (tr && tr->portno == portno &&
      find_cc(tr, shift, chan, data, 0, &mod, 0, 0, 0))
    return mod;
  tr = default_translation;
  if (tr && tr->portno == portno &&
      find_cc(tr, shift, chan, data, 0, &mod, 0, 0, 0))
    return mod;
  return 0;
}

int
check_kps(translation *tr, uint8_t portno, int chan, int data)
{
  if (tr && tr->portno == portno &&
      (find_kps(tr, shift, chan, data, 0, 0) ||
       find_kps(tr, shift, chan, data, 1, 0)))
    return 1;
  tr = default_midi_translation[portno];
  if (tr && tr->portno == portno &&
      (find_kps(tr, shift, chan, data, 0, 0) ||
       find_kps(tr, shift, chan, data, 1, 0)))
    return 1;
  tr = default_translation;
  if (tr && tr->portno == portno &&
      (find_kps(tr, shift, chan, data, 0, 0) ||
       find_kps(tr, shift, chan, data, 1, 0)))
    return 1;
  return 0;
}

int
get_kp_step(translation *tr, uint8_t portno, int chan, int data, int dir)
{
  int step;
  if (tr && tr->portno == portno &&
      find_kps(tr, shift, chan, data, dir>0, &step))
    return step;
  tr = default_midi_translation[portno];
  if (tr && tr->portno == portno &&
      find_kps(tr, shift, chan, data, dir>0, &step))
    return step;
  tr = default_translation;
  if (tr && tr->portno == portno &&
      find_kps(tr, shift, chan, data, dir>0, &step))
    return step;
  return 1;
}

int
get_kp_mod(translation *tr, uint8_t portno, int chan, int data)
{
  int mod;
  if (tr && tr->portno == portno &&
      find_kp(tr, shift, chan, data, 0, &mod, 0, 0, 0))
    return mod;
  tr = default_midi_translation[portno];
  if (tr && tr->portno == portno &&
      find_kp(tr, shift, chan, data, 0, &mod, 0, 0, 0))
    return mod;
  tr = default_translation;
  if (tr && tr->portno == portno &&
      find_kp(tr, shift, chan, data, 0, &mod, 0, 0, 0))
    return mod;
  return 0;
}

int
check_cps(translation *tr, uint8_t portno, int chan)
{
  if (tr && tr->portno == portno &&
      (find_cps(tr, shift, chan, 0, 0) ||
       find_cps(tr, shift, chan, 1, 0)))
    return 1;
  tr = default_midi_translation[portno];
  if (tr && tr->portno == portno &&
      (find_cps(tr, shift, chan, 0, 0) ||
       find_cps(tr, shift, chan, 1, 0)))
    return 1;
  tr = default_translation;
  if (tr && tr->portno == portno &&
      (find_cps(tr, shift, chan, 0, 0) ||
       find_cps(tr, shift, chan, 1, 0)))
    return 1;
  return 0;
}

int
get_cp_step(translation *tr, uint8_t portno, int chan, int dir)
{
  int step;
  if (tr && tr->portno == portno &&
      find_cps(tr, shift, chan, dir>0, &step))
    return step;
  tr = default_midi_translation[portno];
  if (tr && tr->portno == portno &&
      find_cps(tr, shift, chan, dir>0, &step))
    return step;
  tr = default_translation;
  if (tr && tr->portno == portno &&
      find_cps(tr, shift, chan, dir>0, &step))
    return step;
  return 1;
}

int
get_cp_mod(translation *tr, uint8_t portno, int chan)
{
  int mod;
  if (tr && tr->portno == portno &&
      find_cp(tr, shift, chan, 0, &mod, 0, 0, 0))
    return mod;
  tr = default_midi_translation[portno];
  if (tr && tr->portno == portno &&
      find_cp(tr, shift, chan, 0, &mod, 0, 0, 0))
    return mod;
  tr = default_translation;
  if (tr && tr->portno == portno &&
      find_cp(tr, shift, chan, 0, &mod, 0, 0, 0))
    return mod;
  return 0;
}

int
check_pbs(translation *tr, uint8_t portno, int chan)
{
  if (tr && tr->portno == portno &&
      (find_pbs(tr, shift, chan, 0, 0) ||
       find_pbs(tr, shift, chan, 1, 0)))
    return 1;
  tr = default_midi_translation[portno];
  if (tr && tr->portno == portno &&
      (find_pbs(tr, shift, chan, 0, 0) ||
       find_pbs(tr, shift, chan, 1, 0)))
    return 1;
  tr = default_translation;
  if (tr && tr->portno == portno &&
      (find_pbs(tr, shift, chan, 0, 0) ||
       find_pbs(tr, shift, chan, 1, 0)))
    return 1;
  return 0;
}

int
get_pb_step(translation *tr, uint8_t portno, int chan, int dir)
{
  int step;
  if (tr && tr->portno == portno &&
      find_pbs(tr, shift, chan, dir>0, &step))
    return step;
  tr = default_midi_translation[portno];
  if (tr && tr->portno == portno &&
      find_pbs(tr, shift, chan, dir>0, &step))
    return step;
  tr = default_translation;
  if (tr && tr->portno == portno &&
      find_pbs(tr, shift, chan, dir>0, &step))
    return step;
  return 1;
}

int
get_pb_mod(translation *tr, uint8_t portno, int chan)
{
  int mod;
  if (tr && tr->portno == portno &&
      find_pb(tr, shift, chan, 0, &mod, 0, 0, 0))
    return mod;
  tr = default_midi_translation[portno];
  if (tr && tr->portno == portno &&
      find_pb(tr, shift, chan, 0, &mod, 0, 0, 0))
    return mod;
  tr = default_translation;
  if (tr && tr->portno == portno &&
      find_pb(tr, shift, chan, 0, &mod, 0, 0, 0))
    return mod;
  return 0;
}

static int
check_recursive(int status, int chan, int data, int recursive)
{
  // only mod translations can be used in recursive calls
  if (recursive) {
    char name[100];
    fprintf(stderr, "Warning: $%s: undefined\n",
	    debug_key(0, name, status, chan, data, 0));
  }
  return recursive;
}

void
handle_event(uint8_t *msg, uint8_t portno, int depth, int recursive)
{
  translation *tr = get_focused_window_translation();

  //fprintf(stderr, "midi [%d]: %0x %0x %0x\n", portno, msg[0], msg[1], msg[2]);
  int status = msg[0] & 0xf0, chan = msg[0] & 0x0f;
  if (status == 0x80) {
    // convert proper note-off to note-on with vel. 0
    status = 0x90;
    msg[0] = status | chan;
    msg[2] = 0;
  }
  if (debug_midi) debug_input(portno, status, chan, msg[1], msg[2]);
  switch (status) {
  case 0xc0:
    start_debug();
    if (check_recursive(status, chan, msg[1], recursive)) break;
    send_strokes(tr, portno, status, chan, msg[1], 0, 0, 0, depth);
    send_strokes(tr, portno, status, chan, msg[1], 0, 1, 0, depth);
    end_debug();
    break;
  case 0xb0:
    start_debug();
    if (get_cc_mod(tr, portno, chan, msg[1])) {
      send_strokes(tr, portno, status, chan, msg[1], msg[2], 0, 0, depth);
      end_debug();
      break;
    }
    if (check_recursive(status, chan, msg[1], recursive)) break;
    if (msg[2]) {
      if (!keydown_tracker || !inccdown[portno][chan][msg[1]]) {
	send_strokes(tr, portno, status, chan, msg[1], msg[2], 0, 0, depth);
	inccdown[portno][chan][msg[1]] = 1;
      }
    } else {
      if (!keydown_tracker || inccdown[portno][chan][msg[1]]) {
	send_strokes(tr, portno, status, chan, msg[1], msg[2], 1, 0, depth);
	inccdown[portno][chan][msg[1]] = 0;
      }
    }
    // This is a bit of a kludge, since controllers can be used in two
    // different ways. It may happen that we haven't got any translations for
    // the pressed/released state, and that because of a step size >1 the
    // incremental controllers also fail to generate a single stroke. In such
    // a case, we want to pretend that we haven't actually seen any input at
    // all, to prevent spurios section matches when regex debugging is in
    // effect. So we reset the debug counter here, which keeps track of the
    // number of generated strokes.
    debug_count = 0;
    if (check_incr(tr, portno, chan, msg[1])) {
      // Incremental controller a la MCU. NB: This assumes a signed bit
      // representation (values above 0x40 indicate counter-clockwise
      // rotation), which seems to be what most DAWs expect nowadays.
      if (msg[2] < 64) {
	int step = get_cc_step(tr, portno, chan, msg[1], -1);
	if (step) {
	  int d = msg[2]/step;
	  while (d) {
	    send_strokes(tr, portno, status, chan, msg[1], 0, 0, 1, depth);
	    d--;
	  }
	}
      } else if (msg[2] > 64) {
	int step = get_cc_step(tr, portno, chan, msg[1], -1);
	if (step) {
	  int d = (msg[2]-64)/step;
	  while (d) {
	    send_strokes(tr, portno, status, chan, msg[1], 0, 0, -1, depth);
	    d--;
	  }
	}
      }
    } else if (check_ccs(tr, portno, chan, msg[1]) &&
	       inccvalue[portno][chan][msg[1]] != msg[2]) {
      int dir = inccvalue[portno][chan][msg[1]] > msg[2] ? -1 : 1;
      int step = get_cc_step(tr, portno, chan, msg[1], dir);
      if (step) {
	while (inccvalue[portno][chan][msg[1]] != msg[2]) {
	  int d = abs(inccvalue[portno][chan][msg[1]] - msg[2]);
	  if (d > step) d = step;
	  if (d < step) break;
	  send_strokes(tr, portno, status, chan, msg[1], 0, 0, dir, depth);
	  inccvalue[portno][chan][msg[1]] += dir*d;
	}
      }
    }
    end_debug();
    break;
  case 0x90:
    start_debug();
    if (get_note_mod(tr, portno, chan, msg[1])) {
      send_strokes(tr, portno, status, chan, msg[1], msg[2], 0, 0, depth);
      end_debug();
      break;
    }
    if (check_recursive(status, chan, msg[1], recursive)) break;
    if (msg[2]) {
      if (!keydown_tracker || !innotedown[portno][chan][msg[1]]) {
	send_strokes(tr, portno, status, chan, msg[1], msg[2], 0, 0, depth);
	innotedown[portno][chan][msg[1]] = 1;
      }
    } else {
      if (!keydown_tracker || innotedown[portno][chan][msg[1]]) {
	send_strokes(tr, portno, status, chan, msg[1], msg[2], 1, 0, depth);
	innotedown[portno][chan][msg[1]] = 0;
      }
    }
    debug_count = 0;
    if (check_notes(tr, portno, chan, msg[1]) &&
	innotevalue[portno][chan][msg[1]] != msg[2]) {
      int dir = innotevalue[portno][chan][msg[1]] > msg[2] ? -1 : 1;
      int step = get_note_step(tr, portno, chan, msg[1], dir);
      if (step) {
	while (innotevalue[portno][chan][msg[1]] != msg[2]) {
	  int d = abs(innotevalue[portno][chan][msg[1]] - msg[2]);
	  if (d > step) d = step;
	  if (d < step) break;
	  send_strokes(tr, portno, status, chan, msg[1], 0, 0, dir, depth);
	  innotevalue[portno][chan][msg[1]] += dir*d;
	}
      }
    }
    end_debug();
    break;
  case 0xa0:
    start_debug();
    if (get_kp_mod(tr, portno, chan, msg[1])) {
      send_strokes(tr, portno, status, chan, msg[1], msg[2], 0, 0, depth);
      end_debug();
      break;
    }
    if (check_recursive(status, chan, msg[1], recursive)) break;
    if (msg[2]) {
      if (!keydown_tracker || !inkpdown[portno][chan][msg[1]]) {
	send_strokes(tr, portno, status, chan, msg[1], msg[2], 0, 0, depth);
	inkpdown[portno][chan][msg[1]] = 1;
      }
    } else {
      if (!keydown_tracker || inkpdown[portno][chan][msg[1]]) {
	send_strokes(tr, portno, status, chan, msg[1], msg[2], 1, 0, depth);
	inkpdown[portno][chan][msg[1]] = 0;
      }
    }
    debug_count = 0;
    if (check_kps(tr, portno, chan, msg[1]) &&
	inkpvalue[portno][chan][msg[1]] != msg[2]) {
      int dir = inkpvalue[portno][chan][msg[1]] > msg[2] ? -1 : 1;
      int step = get_kp_step(tr, portno, chan, msg[1], dir);
      if (step) {
	while (inkpvalue[portno][chan][msg[1]] != msg[2]) {
	  int d = abs(inkpvalue[portno][chan][msg[1]] - msg[2]);
	  if (d > step) d = step;
	  if (d < step) break;
	  send_strokes(tr, portno, status, chan, msg[1], 0, 0, dir, depth);
	  inkpvalue[portno][chan][msg[1]] += dir*d;
	}
      }
    }
    end_debug();
    break;
  case 0xd0:
    start_debug();
    if (get_cp_mod(tr, portno, chan)) {
      send_strokes(tr, portno, status, chan, 0, msg[1], 0, 0, depth);
      end_debug();
      break;
    }
    if (check_recursive(status, chan, msg[1], recursive)) break;
    if (msg[1]) {
      if (!keydown_tracker || !incpdown[portno][chan]) {
	send_strokes(tr, portno, status, chan, 0, 0, 0, 0, depth);
	incpdown[portno][chan] = 1;
      }
    } else {
      if (!keydown_tracker || incpdown[portno][chan]) {
	send_strokes(tr, portno, status, chan, 0, 0, 1, 0, depth);
	incpdown[portno][chan] = 0;
      }
    }
    debug_count = 0;
    if (check_cps(tr, portno, chan) &&
	incpvalue[portno][chan] != msg[1]) {
      int dir = incpvalue[portno][chan] > msg[1] ? -1 : 1;
      int step = get_cp_step(tr, portno, chan, dir);
      if (step) {
	while (incpvalue[portno][chan] != msg[1]) {
	  int d = abs(incpvalue[portno][chan] - msg[1]);
	  if (d > step) d = step;
	  if (d < step) break;
	  send_strokes(tr, portno, status, chan, 0, 0, 0, dir, depth);
	  incpvalue[portno][chan] += dir*d;
	}
      }
    }
    end_debug();
    break;
  case 0xe0: {
    int bend = ((msg[2] << 7) | msg[1]) - 8192;
    start_debug();
    //fprintf(stderr, "pb %d\n", bend);
    if (get_pb_mod(tr, portno, chan)) {
      send_strokes(tr, portno, status, chan, 0, bend, 0, 0, depth);
      end_debug();
      break;
    }
    if (check_recursive(status, chan, msg[1], recursive)) break;
    if (bend) {
      if (!keydown_tracker || !inpbdown[portno][chan]) {
	send_strokes(tr, portno, status, chan, 0, 0, 0, 0, depth);
	inpbdown[portno][chan] = 1;
      }
    } else {
      if (!keydown_tracker || inpbdown[portno][chan]) {
	send_strokes(tr, portno, status, chan, 0, 0, 1, 0, depth);
	inpbdown[portno][chan] = 0;
      }
    }
    debug_count = 0;
    if (check_pbs(tr, portno, chan) && inpbvalue[portno][chan] - 8192 != bend) {
      int dir = inpbvalue[portno][chan] - 8192 > bend ? -1 : 1;
      int step = get_pb_step(tr, portno, chan, dir);
      if (step) {
	while (inpbvalue[portno][chan] - 8192 != bend) {
	  int d = abs(inpbvalue[portno][chan] - 8192 - bend);
	  if (d > step) d = step;
	  if (d < step) break;
	  send_strokes(tr, portno, status, chan, 0, 0, 0, dir, depth);
	  inpbvalue[portno][chan] += dir*d;
	}
      }
    }
    end_debug();
    break;
  }
  default:
    // ignore everything else for now, specifically system messages
    break;
  }
}

void help(char *progname)
{
  fprintf(stderr, "Usage: %s [-h] [-k] [-o[2]] [-j name] [-r rcfile] [-d[rskmj]]\n", progname);
  fprintf(stderr, "-h print this message\n");
  fprintf(stderr, "-k keep track of key status (ignore double notes)\n");
  fprintf(stderr, "-o enable MIDI output (add 2 for a second pair of ports)\n");
  fprintf(stderr, "-j jack client name (default: midizap)\n");
  fprintf(stderr, "-r config file name (default: MIDIZAP_CONFIG_FILE variable or ~/.midizaprc)\n");
  fprintf(stderr, "-d debug (r = regex, s = strokes, k = keys, m = midi, j = jack; default: all)\n");
}

uint8_t quit = 0;

void quitter()
{
    quit = 1;
}

// Helper functions to process the command line, so that we can pass it to
// Jack session management.

static char *command_line;
static size_t len;

static void add_command(char *arg)
{
  char *a = arg;
  // Do some simplistic quoting if the argument contains blanks. This won't do
  // the right thing if the argument also contains quotes. Oh well.
  if ((strchr(a, ' ') || strchr(a, '\t')) && !strchr(a, '"')) {
    a = malloc(strlen(arg)+3);
    sprintf(a, "\"%s\"", arg);
  }
  if (!command_line) {
    len = strlen(a);
    command_line = malloc(len+1);
    strcpy(command_line, a);
  } else {
    size_t l = strlen(a)+1;
    command_line = realloc(command_line, len+l+1);
    command_line[len] = ' ';
    strcpy(command_line+len+1, a);
    len += l;
  }
  if (a != arg) free(a);
}

static char *absolute_path(char *name)
{
  if (*name == '/') {
    return name;
  } else {
    // This is a relative pathname, we turn it into a canonicalized absolute
    // path.  NOTE: This requires glibc. We should probably rewrite this code
    // to be more portable.
    char *pwd = getcwd(NULL, 0);
    if (!pwd) {
      perror("getcwd");
      return name;
    } else {
      char *path = malloc(strlen(pwd)+strlen(name)+2);
      static char abspath[PATH_MAX];
      sprintf(path, "%s/%s", pwd, name);
      realpath(path, abspath);
      free(path); free(pwd);
      return abspath;
    }
  }
}

// poll interval in microsec (this shouldn't be too large to avoid jitter)
#define POLL_INTERVAL 1000
// how often we check the config file per sec (> 0, < 1000000/POLL_INTERVAL)
#define CONF_FREQ 1
#define MAX_COUNT (1000000/CONF_FREQ/POLL_INTERVAL)

#include <time.h>

int
main(int argc, char **argv)
{
  uint8_t msg[3];
  int opt;

  // Start recording the command line to be passed to Jack session management.
  add_command(argv[0]);

  while ((opt = getopt(argc, argv, "hko::d::j:r:")) != -1) {
    switch (opt) {
    case 'h':
      help(argv[0]);
      exit(0);
    case 'k':
      // see comment on -k and keydown_tracker above
      keydown_tracker = 1;
      add_command("-k");
      break;
    case 'o':
      jack_num_outputs = 1;
      if (optarg && *optarg) {
	const char *a = optarg;
	if (!strcmp(a, "2")) {
	  jack_num_outputs = 2;
	  add_command("-o2");
	} else if (strcmp(a, "1")) {
	  fprintf(stderr, "%s: wrong port number (-o), must be 1 or 2\n", argv[0]);
	  fprintf(stderr, "Try -h for help.\n");
	  exit(1);
	} else
	  add_command("-o1");
      } else
	add_command("-o");
      break;
    case 'd':
      if (optarg && *optarg) {
	const char *a = optarg; char buf[100];
	snprintf(buf, 100, "-d%s", optarg);
	add_command(buf);
	while (*a) {
	  switch (*a) {
	  case 'r':
	    default_debug_regex = 1;
	    break;
	  case 's':
	    default_debug_strokes = 1;
	    break;
	  case 'k':
	    default_debug_keys = 1;
	    break;
	  case 'm':
	    default_debug_midi = 1;
	    break;
	  case 'j':
	    debug_jack = 1;
	    break;
	  default:
	    fprintf(stderr, "%s: unknown debugging option (-d), must be r, s, k or j\n", argv[0]);
	    fprintf(stderr, "Try -h for help.\n");
	    exit(1);
	  }
	  ++a;
	}
      } else {
	default_debug_regex = default_debug_strokes = default_debug_keys =
	  default_debug_midi = 1;
	debug_jack = 1;
	add_command("-d");
      }
      break;
    case 'j':
      jack_client_name = optarg;
      add_command("-j");
      add_command(optarg);
      break;
    case 'r':
      config_file_name = optarg;
      add_command("-r");
      // We need to convert this to an absolute pathname for Jack session
      // management.
      add_command(absolute_path(optarg));
      break;
    default:
      fprintf(stderr, "Try -h for help.\n");
      exit(1);
    }
  }

  if (optind < argc) {
    help(argv[0]);
    exit(1);
  }

  if (command_line) jack_command_line = command_line;

  initdisplay();

  // Force the config file to be loaded initially, so that we pick up the Jack
  // client name to be used (if not set from the command line). This cannot be
  // changed later, so if you want to make changes to the client name in the
  // config file take effect, you need to restart the program.
  read_config_file();

  seq.client_name = jack_client_name;
  seq.n_in = jack_num_outputs>1?jack_num_outputs:1;
  seq.n_out = jack_num_outputs;
  if (!init_jack(&seq, debug_jack)) {
    exit(1);
  }

  int do_flush = debug_regex || debug_strokes || debug_keys || debug_midi ||
    debug_jack;
  signal(SIGINT, quitter);
  time_t t0 = time(0);
  while (!quit) {
    uint8_t portno;
    if (jack_quit) {
      printf("[jack %s, exiting]\n",
	     (jack_quit>0)?"asked us to quit":"shutting down");
      close_jack(&seq);
      exit(0);
    }
    while (pop_midi(&seq, msg, &portno)) {
      handle_event(msg, portno, 0, 0);
      time_t t = time(0);
      if (t > t0) {
	// Check whether to reload the config file every sec.
	if (read_config_file()) last_focused_window = 0;
	t0 = t;
      }
    }
    usleep(POLL_INTERVAL);
    time_t t = time(0);
    if (t > t0) {
      // Check again when polling.
      if (read_config_file()) last_focused_window = 0;
      t0 = t;
    }
    // Make sure that debugging output gets flushed every once in a while (may
    // be buffered when midizap is running inside a QjackCtl session).
    if (do_flush) fflush(NULL);
  }
  printf(" [exiting]\n");
  close_jack(&seq);
}
