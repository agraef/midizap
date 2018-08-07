
// Copyright 2013 Eric Messick (FixedImagePhoto.com/Contact)
// Copyright 2018 Albert Graef <aggraef@gmail.com>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <linux/input.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include<signal.h>

#include <regex.h>

#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>


// delay in ms before processing each XTest event
// CurrentTime means no delay
#define DELAY CurrentTime

// we define these as extra KeySyms to represent mouse events
#define XK_Button_0 0x2000000 // just an offset, not a real button
#define XK_Button_1 0x2000001
#define XK_Button_2 0x2000002
#define XK_Button_3 0x2000003
#define XK_Scroll_Up 0x2000004
#define XK_Scroll_Down 0x2000005

#define PRESS 1
#define RELEASE 2
#define PRESS_RELEASE 3
#define HOLD 4

typedef struct _stroke {
  struct _stroke *next;
  // nonzero keysym indicates a key event
  KeySym keysym;
  int press; // zero -> release, non-zero -> press
  // keysym == 0 => MIDI event
  int status, data; // status and, if applicable, first data byte
  // the dirty bit indicates a MIDI event for which a release event still
  // needs to be generated in key events
  int dirty;
} stroke;

#define NUM_KEYS 128
#define NUM_CHAN 16

typedef struct _translation {
  struct _translation *next;
  char *name;
  int is_default, is_incr;
  regex_t regex;
  // XXFIXME: This is really inefficient, we should rather use some kind of
  // dictionary here.
  stroke *pc[NUM_CHAN][NUM_KEYS][2];
  stroke *note[NUM_CHAN][NUM_KEYS][2];
  stroke *cc[NUM_CHAN][NUM_KEYS][2];
  stroke *ccs[NUM_CHAN][NUM_KEYS][2];
  stroke *pb[NUM_CHAN][2];
  stroke *pbs[NUM_CHAN][2];
} translation;

extern translation *get_translation(char *win_title, char *win_class);
extern void print_stroke_sequence(char *name, char *up_or_down, stroke *s);
extern int debug_regex, debug_strokes, debug_keys;
extern int default_debug_regex, default_debug_strokes, default_debug_keys;
extern char *config_file_name;
