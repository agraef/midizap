
// Copyright 2013 Eric Messick (FixedImagePhoto.com/Contact)
// Copyright 2018 Albert Graef <aggraef@gmail.com>

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
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
  int8_t press; // zero -> release, non-zero -> press
  // nonzero value indicates a shift event
  int8_t shift;
  // keysym == shift == 0 => MIDI event
  int status, data; // status and, if applicable, first data byte
  int step; // step size (1, 127 or 8191 by default, depending on status)
  // discrete steps (for special "modulus" translations only)
  int n_steps, *steps;
  // the incremental bit indicates an incremental control change (typically
  // used with endless rotary encoders) to be represented as a sign bit value
  uint8_t incr;
  // the swap bit indicates that 1st and 2nd data byte are to be swapped in
  // mod translations
  uint8_t swap;
  // the change bit indicates that the message should only be output if its
  // value has changed since the last time
  uint8_t change;
  // cached values for the change bit
  int d, v;
  // the recursive bit indicates a MIDI message which is to be translated
  // recursively
  uint8_t recursive;
  // the feedback bit indicates a MIDI message which is to be sent back to
  // device or application (flipping the output port)
  uint8_t feedback;
  // the dirty bit indicates a MIDI event for which a release event still
  // needs to be generated in key events
  uint8_t dirty;
} stroke;

typedef struct _stroke_data {
  // key (MIDI channel and, for note/CC/PB, data byte)
  uint8_t chan, data;
  // stroke data, indexed by press/release or up/down index
  stroke *s[2];
  // step size
  int step[2], n_steps[2], *steps[2];
  // incr flag (CC only)
  uint8_t is_incr;
  // modulus
  uint16_t mod;
  // anyshift flag (default rule)
  uint8_t anyshift;
} stroke_data;

#define N_SHIFTS 4 // number of distinct shift states
#define N_ST (N_SHIFTS+1)

typedef struct _translation {
  struct _translation *next;
  char *name;
  int is_default;
  regex_t regex;
  uint8_t portno;
  // these are indexed by shift status
  stroke_data *note[N_ST];
  stroke_data *notes[N_ST];
  stroke_data *pc[N_ST];
  stroke_data *cc[N_ST];
  stroke_data *ccs[N_ST];
  stroke_data *pb[N_ST];
  stroke_data *pbs[N_ST];
  stroke_data *kp[N_ST];
  stroke_data *kps[N_ST];
  stroke_data *cp[N_ST];
  stroke_data *cps[N_ST];
  // actual and allocated sizes (can be at most 16*128)
  uint16_t n_note[N_ST], n_notes[N_ST], n_pc[N_ST],
    n_cc[N_ST], n_ccs[N_ST], n_pb[N_ST], n_pbs[N_ST],
    n_kp[N_ST], n_kps[N_ST], n_cp[N_ST], n_cps[N_ST];
  uint16_t a_note[N_ST], a_notes[N_ST], a_pc[N_ST],
    a_cc[N_ST], a_ccs[N_ST], a_pb[N_ST], a_pbs[N_ST],
    a_kp[N_ST], a_kps[N_ST], a_cp[N_ST], a_cps[N_ST];
} translation;

extern void reload_callback(void);
extern int read_config_file(void);
extern translation *get_translation(char *win_title, char *win_class);
extern void print_stroke_sequence(char *name, char *up_or_down, stroke *s,
				  int mod, int step, int n_steps, int *steps,
				  int val);
extern translation *default_translation, *default_midi_translation[2];
extern int debug_regex, debug_strokes, debug_keys, debug_midi;
extern int default_debug_regex, default_debug_strokes, default_debug_keys,
  default_debug_midi;
extern char *config_file_name;
extern int jack_num_outputs, auto_feedback;
extern int passthrough[2], system_passthrough[2];
extern int midi_octave, shift;
extern char *jack_client_name;
