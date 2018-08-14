
/*

 Copyright 2013 Eric Messick (FixedImagePhoto.com/Contact)

 Copyright 2018 Albert Graef <aggraef@gmail.com>, various improvements

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
static int16_t ccvalue[16][128];
static int16_t pbvalue[16] =
  {8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
   8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192};

void
send_midi(uint8_t portno, int status, int data, int step, int incr, int index, int dir)
{
  if (!jack_num_outputs) return; // MIDI output not enabled
  uint8_t msg[3];
  int chan = status & 0x0f;
  msg[0] = status;
  msg[1] = data;
  switch (status & 0xf0) {
  case 0x90:
    if (!index) {
      msg[2] = step?step:127;
      if (msg[2] > 127) msg[2] = 127;
    } else {
      msg[2] = 0;
    }
    break;
  case 0xb0:
    if (dir) {
      if (incr) {
	// incremental controller, simply spit out a relative sign bit value
	msg[2] = dir>0?1:65;
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
    } else if (!index) {
      msg[2] = step?step:127;
      if (msg[2] > 127) msg[2] = 127;
    } else {
      msg[2] = 0;
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
    } else if (!index) {
      pbval = 8192+(step?step:8191);
      if (pbval > 16383) pbval = 16383;
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
    // just send the message
    break;
  default:
    return;
  }
  queue_midi(&seq, msg, portno);
}

stroke *
fetch_stroke(translation *tr, uint8_t portno, int status, int chan, int data,
	     int index, int dir)
{
  if (tr && tr->portno == portno) {
    switch (status) {
    case 0x90:
      return tr->note[shift][chan][data][index];
    case 0xc0:
      return tr->pc[shift][chan][data][index];
    case 0xb0:
      if (dir)
	return tr->ccs[shift][chan][data][dir>0];
      else
	return tr->cc[shift][chan][data][index];
    case 0xe0:
      if (dir)
	return tr->pbs[shift][chan][dir>0];
      else
	return tr->pb[shift][chan][index];
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

static char *debug_key(translation *tr, char *name,
		       int status, int chan, int data, int dir)
{
  static char *note_names[] = { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "G#", "A", "Bb", "B" };
  char *prefix = shift?"^":"", *suffix = "";
  strcpy(name, "??");
  switch (status) {
  case 0x90:
    sprintf(name, "%s%s%d-%d", prefix, note_names[data % 12],
	    data / 12 + midi_octave, chan+1);
    break;
  case 0xb0: {
    int step = tr->cc_step[shift][chan][data][dir>0];
    if (!dir)
      suffix = "";
    else if (tr->is_incr[shift][chan][data])
      suffix = (dir<0)?"<":">";
    else
      suffix = (dir<0)?"-":"+";
    if (dir && step != 1)
      sprintf(name, "%sCC%d[%d]-%d%s", prefix, data, step, chan+1, suffix);
    else
      sprintf(name, "%sCC%d-%d%s", prefix, data, chan+1, suffix);
    break;
  }
  case 0xc0:
    sprintf(name, "%sPC%d-%d", prefix, data, chan+1);
    break;
  case 0xe0: {
    int step = tr->pb_step[shift][chan][dir>0];
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

static void debug_input(translation *tr, int portno,
			int status, int chan, int data, int data2)
{
  char name[100];
  if (status == 0xe0)
    // translate LSB,MSB to a pitch bend value in the range -8192..8191
    data2 = ((data2 << 7) | data) - 8192;
  if (status == 0xc0)
    printf("[%d] %s\n", portno,
	   debug_key(tr, name, status, chan, data, 0));
  else
    printf("[%d] %s value = %d\n", portno,
	   debug_key(tr, name, status, chan, data, 0), data2);
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

void
send_strokes(translation *tr, uint8_t portno, int status, int chan, int data,
	     int index, int dir)
{
  int nkeys = 0;
  stroke *s = fetch_stroke(tr, portno, status, chan, data, index, dir);
  // If there's no press/release translation, check whether we have got at
  // least the corresponding release/press translation, in order to prevent
  // spurious error messages if either the press or release translation just
  // happens to be empty.
  int chk = s ||
    (!dir && fetch_stroke(tr, portno, status, chan, data, !index, dir));

  if (!s && jack_num_outputs) {
    // fall back to default MIDI translation
    tr = default_midi_translation[portno];
    s = fetch_stroke(tr, portno, status, chan, data, index, dir);
    chk = chk || s ||
      (!dir && fetch_stroke(tr, portno, status, chan, data, !index, dir));
    // Ignore all MIDI input on the second port if no translation was found in
    // the [MIDI2] section (or the section is missing altogether).
    if (portno && !s) return;
  }

  if (!s) {
    // fall back to the default translation
    tr = default_translation;
    s = fetch_stroke(tr, portno, status, chan, data, index, dir);
    chk = chk || s ||
      (!dir && fetch_stroke(tr, portno, status, chan, data, !index, dir));
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
			  dir?"":index?"U":"D", s);
  }
  while (s) {
    if (s->keysym) {
      send_key(s->keysym, s->press);
      nkeys++;
    } else if (s->shift) {
      // toggle shift status
      shift = !shift;
    } else {
      send_midi(portno, s->status, s->data, s->step, s->incr, index, dir);
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

static int8_t inccvalue[2][16][128];
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

static uint8_t notedown[2][16][128];
static uint8_t inccdown[2][16][128];
static uint8_t inpbdown[2][16];

int
check_incr(translation *tr, uint8_t portno, int chan, int data)
{
  if (tr && tr->portno == portno &&
      (tr->ccs[shift][chan][data][0] || tr->ccs[shift][chan][data][1]))
    return tr->is_incr[shift][chan][data];
  tr = default_midi_translation[portno];
  if (tr && tr->portno == portno &&
      (tr->ccs[shift][chan][data][0] || tr->ccs[shift][chan][data][1]))
    return tr->is_incr[shift][chan][data];
  tr = default_translation;
  if (tr && tr->portno == portno &&
      (tr->ccs[shift][chan][data][0] || tr->ccs[shift][chan][data][1]))
    return tr->is_incr[shift][chan][data];
  return 0;
}

int
get_cc_step(translation *tr, uint8_t portno, int chan, int data, int dir)
{
  if (tr && tr->portno == portno &&
      tr->ccs[shift][chan][data][dir>0])
    return tr->cc_step[shift][chan][data][dir>0];
  tr = default_midi_translation[portno];
  if (tr && tr->portno == portno &&
      tr->ccs[shift][chan][data][dir>0])
    return tr->cc_step[shift][chan][data][dir>0];
  tr = default_translation;
  if (tr && tr->portno == portno &&
      tr->ccs[shift][chan][data][dir>0])
    return tr->cc_step[shift][chan][data][dir>0];
  return 1;
}

int
check_pbs(translation *tr, uint8_t portno, int chan)
{
  if (tr && tr->portno == portno &&
      (tr->pbs[shift][chan][0] || tr->pbs[shift][chan][1]))
    return 1;
  tr = default_midi_translation[portno];
  if (tr && tr->portno == portno &&
      (tr->pbs[shift][chan][0] || tr->pbs[shift][chan][1]))
    return 1;
  tr = default_translation;
  if (tr && tr->portno == portno &&
      (tr->pbs[shift][chan][0] || tr->pbs[shift][chan][1]))
    return 1;
  return 0;
}

int
get_pb_step(translation *tr, uint8_t portno, int chan, int dir)
{
  if (tr && tr->portno == portno &&
      tr->pbs[shift][chan][dir>0])
    return tr->pb_step[shift][chan][dir>0];
  tr = default_midi_translation[portno];
  if (tr && tr->portno == portno &&
      tr->pbs[shift][chan][dir>0])
    return tr->pb_step[shift][chan][dir>0];
  tr = default_translation;
  if (tr && tr->portno == portno &&
      tr->pbs[shift][chan][dir>0])
    return tr->pb_step[shift][chan][dir>0];
  return 1;
}

void
handle_event(uint8_t *msg, uint8_t portno)
{
  translation *tr = get_focused_window_translation();

  //fprintf(stderr, "midi [%d]: %0x %0x %0x\n", portno, msg[0], msg[1], msg[2]);
  int status = msg[0] & 0xf0, chan = msg[0] & 0x0f;
  if (status == 0x80) {
    status = 0x90;
    msg[0] = status | chan;
    msg[2] = 0;
  }
  if (debug_midi) debug_input(tr, portno, status, chan, msg[1], msg[2]);
  switch (status) {
  case 0xc0:
    start_debug();
    send_strokes(tr, portno, status, chan, msg[1], 0, 0);
    send_strokes(tr, portno, status, chan, msg[1], 1, 0);
    end_debug();
    break;
  case 0x90:
    start_debug();
    if (msg[2]) {
      if (!keydown_tracker || !notedown[portno][chan][msg[1]]) {
	send_strokes(tr, portno, status, chan, msg[1], 0, 0);
	notedown[portno][chan][msg[1]] = 1;
      }
    } else {
      if (!keydown_tracker || notedown[portno][chan][msg[1]]) {
	send_strokes(tr, portno, status, chan, msg[1], 1, 0);
	notedown[portno][chan][msg[1]] = 0;
      }
    }
    end_debug();
    break;
  case 0xb0:
    start_debug();
    if (msg[2]) {
      if (!keydown_tracker || !inccdown[portno][chan][msg[1]]) {
	send_strokes(tr, portno, status, chan, msg[1], 0, 0);
	inccdown[portno][chan][msg[1]] = 1;
      }
    } else {
      if (!keydown_tracker || inccdown[portno][chan][msg[1]]) {
	send_strokes(tr, portno, status, chan, msg[1], 1, 0);
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
      // But some DAWs may also have it the other way round, so that you may
      // have to swap the actions for increment and decrement. XXXTODO:
      // Maybe the encoding should be a configurable parameter?
      if (msg[2] < 64) {
	int d = msg[2];
	while (d) {
	  send_strokes(tr, portno, status, chan, msg[1], 0, 1);
	  d--;
	}
      } else if (msg[2] > 64) {
	int d = msg[2]-64;
	while (d) {
	  send_strokes(tr, portno, status, chan, msg[1], 0, -1);
	  d--;
	}
      }
    } else if (inccvalue[portno][chan][msg[1]] != msg[2]) {
      int dir = inccvalue[portno][chan][msg[1]] > msg[2] ? -1 : 1;
      int step = get_cc_step(tr, portno, chan, msg[1], dir);
      if (step) {
	while (inccvalue[portno][chan][msg[1]] != msg[2]) {
	  int d = abs(inccvalue[portno][chan][msg[1]] - msg[2]);
	  if (d > step) d = step;
	  if (d < step) break;
	  send_strokes(tr, portno, status, chan, msg[1], 0, dir);
	  inccvalue[portno][chan][msg[1]] += dir*d;
	}
      }
    }
    end_debug();
    break;
  case 0xe0: {
    int bend = ((msg[2] << 7) | msg[1]) - 8192;
    start_debug();
    //fprintf(stderr, "pb %d\n", bend);
    if (bend) {
      if (!keydown_tracker || !inpbdown[portno][chan]) {
	send_strokes(tr, portno, status, chan, 0, 0, 0);
	inpbdown[portno][chan] = 1;
      }
    } else {
      if (!keydown_tracker || inpbdown[portno][chan]) {
	send_strokes(tr, portno, status, chan, 0, 1, 0);
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
	  send_strokes(tr, portno, status, chan, 0, 0, dir);
	  inpbvalue[portno][chan] += dir*d;
	}
      }
    }
    end_debug();
    break;
  }
  default:
    // ignore everything else
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

// poll interval in microsec (this shouldn't be too large to avoid jitter)
#define POLL_INTERVAL 1000
// how often we check the config file per sec (> 0, < 1000000/POLL_INTERVAL)
#define CONF_FREQ 1
#define MAX_COUNT (1000000/CONF_FREQ/POLL_INTERVAL)

int
main(int argc, char **argv)
{
  uint8_t msg[3];
  int opt, count = 0;

  while ((opt = getopt(argc, argv, "hko::d::j:r:")) != -1) {
    switch (opt) {
    case 'h':
      help(argv[0]);
      exit(0);
    case 'k':
      // see comment on -k and keydown_tracker above
      keydown_tracker = 1;
      break;
    case 'o':
      jack_num_outputs = 1;
      if (optarg && *optarg) {
	const char *a = optarg;
	if (*a == '2') {
	  jack_num_outputs = 2;
	} else if (*a && *a != '1') {
	  fprintf(stderr, "%s: wrong port number (-o), must be 1 or 2\n", argv[0]);
	  fprintf(stderr, "Try -h for help.\n");
	  exit(1);
	}
      }
      break;
    case 'd':
      if (optarg && *optarg) {
	const char *a = optarg;
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
      }
      break;
    case 'j':
      jack_client_name = optarg;
      break;
    case 'r':
      config_file_name = optarg;
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

  signal(SIGINT, quitter);
  while (!quit) {
    uint8_t portno;
    while (pop_midi(&seq, msg, &portno)) {
      handle_event(msg, portno);
      count = 0;
    }
    usleep(POLL_INTERVAL);
    if (++count >= MAX_COUNT) {
      // Check whether to reload the config file if we haven't seen any MIDI
      // input in a while. Note that if the file *is* reloaded, then we also
      // need to reset last_focused_window here, so that the translations of
      // the focused window are recomputed the next time we handle an event.
      if (read_config_file()) last_focused_window = 0;
      count = 0;
    }
  }
  printf(" [exiting]\n");
  close_jack(&seq);
}
