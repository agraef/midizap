
/*

 Contour ShuttlePro v2 interface

 Copyright 2013 Eric Messick (FixedImagePhoto.com/Contact)

 Copyright 2018 Albert Graef <aggraef@gmail.com>, various improvements

 Based on a version (c) 2006 Trammell Hudson <hudson@osresearch.net>

 which was in turn

 Based heavily on code by Arendt David <admin@prnet.org>

*/

#include "midizap.h"
#include "jackdriver.h"

typedef struct input_event EV;

extern int debug_regex;
extern translation *default_translation;

unsigned short jogvalue = 0xffff;
int shuttlevalue = 0xffff;
struct timeval last_shuttle;
int need_synthetic_shuttle;
Display *display;

JACK_SEQ seq;
int enable_jack_output = 0, debug_jack = 0;

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
send_midi(int status, int data, int step, int incr, int index, int dir)
{
  if (!enable_jack_output) return; // MIDI output not enabled
  uint8_t msg[3];
  int chan = status & 0x0f;
  msg[0] = status;
  msg[1] = data;
  switch (status & 0xf0) {
  case 0x90:
    if (!index) {
      msg[2] = 127;
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
	if (!step) return;
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
      msg[2] = 127;
    } else {
      msg[2] = 0;
    }
    break;
  case 0xe0: {
    // pitch bends are treated similarly to a controller, but with a 14 bit
    // range (0..16383, with 8192 being the center value)
    int pbval = 0;
    if (dir) {
      if (!step) return;
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
      pbval = 16383;
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
  queue_midi(&seq, msg);
}

stroke *
fetch_stroke(translation *tr, int status, int chan, int data,
	     int index, int dir)
{
  if (tr != NULL) {
    switch (status) {
    case 0x90:
      return tr->note[chan][data][index];
    case 0xc0:
      return tr->pc[chan][data][index];
    case 0xb0:
      if (dir)
	return tr->ccs[chan][data][dir>0];
      else
	return tr->cc[chan][data][index];
    case 0xe0:
      if (dir)
	return tr->pbs[chan][dir>0];
      else
	return tr->pb[chan][index];
    default:
      return NULL;
    }
  } else
    return NULL;
}

static char *note_names[] = { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "G#", "A", "Bb", "B" };

void
send_strokes(translation *tr, int status, int chan, int data,
	     int index, int dir)
{
  int nkeys = 0;
  stroke *s = fetch_stroke(tr, status, chan, data, index, dir);

  if (s == NULL) {
    tr = default_translation;
    s = fetch_stroke(tr, status, chan, data, index, dir);
  }

  if (debug_keys && s) {
    char name[100] = "??", *suffix = "";
    switch (status) {
    case 0x90:
      sprintf(name, "%s%d-%d", note_names[data % 12], data / 12, chan+1);
      break;
    case 0xb0: {
      int step = tr->cc_step[chan][data][dir>0];
      if (!dir)
	suffix = "";
      else if (tr->is_incr[chan][data])
	suffix = (dir<0)?"<":">";
      else
	suffix = (dir<0)?"-":"+";
      if (dir && step != 1)
	sprintf(name, "CC%d[%d]-%d%s", data, step, chan+1, suffix);
      else
	sprintf(name, "CC%d-%d%s", data, chan+1, suffix);
      break;
    }
    case 0xc0:
      sprintf(name, "PC%d-%d", data, chan+1);
      break;
    case 0xe0: {
      int step = tr->pb_step[chan][dir>0];
      if (!dir)
	suffix = "";
      else
	suffix = (dir<0)?"-":"+";
      if (dir && step != 1)
	sprintf(name, "PB[%d]-%d%s", step, chan+1, suffix);
      else
	sprintf(name, "PB-%d%s", chan+1, suffix);
      break;
    }
    default: // this can't happen
      break;
    }
    print_stroke_sequence(name, dir?"":index?"U":"D", s);
  }
  while (s) {
    if (s->keysym) {
      send_key(s->keysym, s->press);
      nkeys++;
    } else {
      send_midi(s->status, s->data, s->step, s->incr, index, dir);
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

static Window last_focused_window = 0;
static translation *last_window_translation = NULL;

translation *
get_focused_window_translation()
{
  Window focus;
  int revert_to;
  char *window_name = NULL, *window_class = NULL;
  char *name;

  XGetInputFocus(display, &focus, &revert_to);
  if (focus != last_focused_window) {
    last_focused_window = focus;
    window_name = walk_window_tree(focus, &window_class);
    if (window_name == NULL) {
      name = "-- Unlabeled Window --";
    } else {
      name = window_name;
    }
    last_window_translation = get_translation(name, window_class);
    if (debug_regex) {
      if (last_window_translation != NULL) {
	printf("translation: %s for %s (class %s)\n",
	       last_window_translation->name, name, window_class);
      } else {
	printf("no translation found for %s (class %s)\n", name, window_class);
      }
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

static int8_t inccvalue[16][128];
static int16_t inpbvalue[16] =
  {8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
   8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192};

static uint8_t notedown[16][128];
static uint8_t inccdown[16][128];
static uint8_t inpbdown[16];

int
check_incr(translation *tr, int chan, int data)
{
  if (tr->ccs[chan][data][0] || tr->ccs[chan][data][1])
    return tr->is_incr[chan][data];
  tr = default_translation;
  if (tr->ccs[chan][data][0] || tr->ccs[chan][data][1])
    return tr->is_incr[chan][data];
  return 0;
}

int
check_pbs(translation *tr, int chan)
{
  if (tr->pbs[chan][0] || tr->pbs[chan][1])
    return 1;
  tr = default_translation;
  if (tr->pbs[chan][0] || tr->pbs[chan][1])
    return 1;
  return 0;
}

void
handle_event(uint8_t *msg)
{
  translation *tr = get_focused_window_translation();

  //fprintf(stderr, "midi: %0x %0x %0x\n", msg[0], msg[1], msg[2]);
  if (tr != NULL) {
    int status = msg[0] & 0xf0, chan = msg[0] & 0x0f;
    if (status == 0x80) {
      status = 0x90;
      msg[0] = status | chan;
      msg[2] = 0;
    }
    switch (status) {
    case 0xc0:
      send_strokes(tr, status, chan, msg[1], 0, 0);
      send_strokes(tr, status, chan, msg[1], 1, 0);
      break;
    case 0x90:
      if (msg[2]) {
	if (!notedown[chan][msg[1]]) {
	  send_strokes(tr, status, chan, msg[1], 0, 0);
	  notedown[chan][msg[1]] = 1;
	}
      } else {
	if (notedown[chan][msg[1]]) {
	  send_strokes(tr, status, chan, msg[1], 1, 0);
	  notedown[chan][msg[1]] = 0;
	}
      }
      break;
    case 0xb0:
      if (msg[2]) {
	if (!inccdown[chan][msg[1]]) {
	  send_strokes(tr, status, chan, msg[1], 0, 0);
	  inccdown[chan][msg[1]] = 1;
	}
      } else {
	if (inccdown[chan][msg[1]]) {
	  send_strokes(tr, status, chan, msg[1], 1, 0);
	  inccdown[chan][msg[1]] = 0;
	}
      }
      if (check_incr(tr, chan, msg[1])) {
	// Incremental controller a la MCU. NB: This assumes a signed bit
	// representation (values above 0x40 indicate counter-clockwise
	// rotation), which seems to be what most DAWs expect nowadays.
	// But some DAWs may also have it the other way round, so that you may
	// have to swap the actions for increment and decrement. XXXTODO:
	// Maybe the encoding should be a configurable parameter?
	if (msg[2] < 64) {
	  int d = msg[2];
	  while (d) {
	    send_strokes(tr, status, chan, msg[1], 0, 1);
	    d--;
	  }
	} else if (msg[2] > 64) {
	  int d = msg[2]-64;
	  while (d) {
	    send_strokes(tr, status, chan, msg[1], 0, -1);
	    d--;
	  }
	}
      } else if (inccvalue[chan][msg[1]] != msg[2]) {
	int dir = inccvalue[chan][msg[1]] > msg[2] ? -1 : 1;
	int step = tr->cc_step[chan][msg[1]][dir>0];
	if (step) {
	  while (inccvalue[chan][msg[1]] != msg[2]) {
	    int d = abs(inccvalue[chan][msg[1]] - msg[2]);
	    if (d > step) d = step;
	    if (d < step) break;
	    send_strokes(tr, status, chan, msg[1], 0, dir);
	    inccvalue[chan][msg[1]] += dir*d;
	  }
	}
      }
      break;
    case 0xe0: {
      int bend = ((msg[2] << 7) | msg[1]) - 8192;
      //fprintf(stderr, "pb %d\n", bend);
      if (bend) {
	if (!inpbdown[chan]) {
	  send_strokes(tr, status, chan, 0, 0, 0);
	  inpbdown[chan] = 1;
	}
      } else {
	if (inpbdown[chan]) {
	  send_strokes(tr, status, chan, 0, 1, 0);
	  inpbdown[chan] = 0;
	}
      }
      if (check_pbs(tr, chan) && inpbvalue[chan] - 8192 != bend) {
	int dir = inpbvalue[chan] - 8192 > bend ? -1 : 1;
	int step = tr->pb_step[chan][dir>0];
	if (step) {
	  while (inpbvalue[chan] - 8192 != bend) {
	    int d = abs(inpbvalue[chan] - 8192 - bend);
	    if (d > step) d = step;
	    if (d < step) break;
	    send_strokes(tr, status, chan, 0, 0, dir);
	    inpbvalue[chan] += dir*d;
	  }
	}
      }
      break;
    }
    default:
      // ignore everything else
      break;
    }
  }
}

void help(char *progname)
{
  fprintf(stderr, "Usage: %s [-h] [-o] [-r rcfile] [-d[rskj]]\n", progname);
  fprintf(stderr, "-h print this message\n");
  fprintf(stderr, "-o enable MIDI output\n");
  fprintf(stderr, "-r config file name (default: MIDIZAP_CONFIG_FILE variable or ~/.midizaprc)\n");
  fprintf(stderr, "-d debug (r = regex, s = strokes, k = keys, j = jack; default: all)\n");
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

  while ((opt = getopt(argc, argv, "hod::r:")) != -1) {
    switch (opt) {
    case 'h':
      help(argv[0]);
      exit(0);
    case 'o':
      enable_jack_output = 1;
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
	default_debug_regex = default_debug_strokes = default_debug_keys = 1;
	debug_jack = 1;
      }
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

  seq.usein = 1; seq.useout = enable_jack_output;
  if (!init_jack(&seq, debug_jack)) {
    exit(1);
  }

  signal(SIGINT, quitter);
  // force the config file to be loaded initially
  count = MAX_COUNT;
  while (!quit) {
    while (pop_midi(&seq, msg)) {
      handle_event(msg);
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
