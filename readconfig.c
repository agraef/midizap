
/*

  Copyright 2013 Eric Messick (FixedImagePhoto.com/Contact)
  Copyright 2018 Albert Graef <aggraef@gmail.com>

  Read and process the configuration file ~/.midizaprc

  Lines starting with # are comments.

  The file is a sequence of sections defining translation classes. Each
  section takes the following form:

  [name] regex
  CC<0..127> output         # control change
  PC<0..127> output         # program change
  PB output                 # pitch bend
  <A-G>[#b]<0..11> output   # note

  When focus is on a window whose class or title matches regex, the
  following translation class is in effect.  An empty regex for the last
  class will always match, allowing default translations.  Any output
  sequences not bound in a matched section will be loaded from the
  default section if they are bound there.

  Each "[name] regex" line introduces the list of MIDI message
  translations for the named translation class.  The name is only used
  for debugging output, and needn't be unique.  The following lines
  indicate what output should be produced for the given MIDI messages.

  Note that not all MIDI message types are supported right now (no
  aftertouch, no system messages), but that subset should be enough to
  handle most common use cases.  (In any case, adding more message types
  should be a piece of cake.)

  Note messages are specified using the cutomary notation (note name
  A..G, optionally followed by an accidental, # or b, followed by a
  (zero-based) MIDI octave number. Note that all MIDI octaves start at
  the note C, so B0 comes before C1. C5 denotes middle C, A5 is the
  chamber pitch (usually at 440 Hz).

  MIDI messages are on channel 1 by default; a suffix of the form
  -<1..16> can be used to specify a different MIDI channel.  E.g., C3-10
  denotes note C3 on MIDI channel 10.

  By default, all messages are interpreted in the same way as keys on a
  computer keyboard, i.e., they can be "on" ("pressed") or "off"
  ("released").  For notes, a nonzero velocity means "pressed", zero
  "released".  Similarly, for control changes any nonzero value
  indicates "pressed".  Same goes for pitch bends, but in this case 0
  denotes the center value (considering pitch bend values as signed
  quantities in the range -8192..8191).  Again, any nonzero (positive or
  negative) value means "pressed", and 0 (the center value) "released".
  Finally, while program changes don't actually come in "on"/"off"
  pairs, they are treated in the same key-like fashion, assuming that
  they are "pressed" and then "released" immediately afterwards.

  output is a sequence of one or more key codes with optional up/down
  indicators, or strings of printable characters enclosed in double
  quotes, separated by whitespace.  Sequences may have separate press
  and release sequences, separated by the word RELEASE.

  Examples:

  C5 "qwer"
  D5 XK_Right
  E5 XK_Alt_L/D XK_Right
  F5 "V" XK_Left XK_Page_Up "v"
  G5 XK_Alt_L/D "v" XK_Alt_L/U "x" RELEASE "q"

  Any keycode can be followed by an optional /D, /U, or /H, indicating
  that the key is just going down (without being released), going up,
  or going down and being held until the "off" event is received.

  So, in general, modifier key codes will be followed by /D, and
  precede the keycodes they are intended to modify.  If a sequence
  requires different sets of modifiers for different keycodes, /U can
  be used to release a modifier that was previously pressed with /D.

  By default, MIDI messages translate to separate press and release
  sequences.  At the end of the press sequence, all down keys marked by
  /D will be released, and the last key not marked by /D, /U, or /H will
  remain pressed.  The release sequence will begin by releasing the last
  held key.  If keys are to be pressed as part of the release sequence,
  then any keys marked with /D will be repressed before continuing the
  sequence.  Keycodes marked with /H remain held between the press and
  release sequences.

  By marking CC (control change) and PB (pitch bend) messages with a
  trailing "+" or "-", they can also be used to report incremental
  changes.  These work a bit differently from the key press semantics.
  Instead of providing separate press and release sequences, the output
  of such translations is executed whenever the controller increases or
  decreases, respectively.  At the end of such sequences, all down keys
  will be released.  For instance, the following translations output the
  letter "a" whenever the volume controller (CC7) is increased, and the
  letter "b" if it is decreased.  Also, the number of times one of these
  keys is output corresponds to the actual change in the controller
  value.  (Thus, if in the example CC7 increases by 32, say, 32 "a"s
  will be output.)

  CC7+ "a"
  CC7+ "b"

  CC also has an alternative "incremental" mode which handles relative
  control changes encoded in "sign bit" format.  Here, a value < 64
  denotes an increase, and a value > 64 a decrease (thus the 7th bit is
  the sign of the value change).  The lower 6 bits then denote the
  amount of change (e.g., 2 increments the control by 2, whereas 66
  decrements by 2).  This format is often used with endless rotary
  encoders, such as the jog wheel on some DAW controllers like the
  Mackie MCU.  It is denoted by using "<" and ">" in lieu of "-" and "+"
  as the suffix of the CC message. Example:

  CC60< XK_Left
  CC60> XK_Right

  If the "up" and "down" sequences for controller and pitch bend changes
  are the same, the notation "=" can be used to indicate that the same
  sequence should be output in either case. This most commonly arises in
  pure MIDI translations. For instance, to map the modulation wheel
  (CC1) to the volume controller (CC7):

  CC1= CC7

  Which is exactly the same as the two translations:

  CC1+ CC7
  CC1- CC7

  The same goes for "<"/">" and "~" in incremental mode. E.g., CC1~ CC7
  is exactly the same as:

  CC1< CC7
  CC1> CC7

  Furthermore, PB (pitch bends) can have a step size associated with
  them.  The default step size is 1.  To indicate a different step size,
  the notation PB[<step size>] is used.  E.g., PB[1170] will give you
  about 7 steps up and down, which is useful to emulate a shuttle wheel
  such as those on the Contour Design devices.  Example:

  PB[1170]- "j"
  PB[1170]+ "l"

  Most of the notations for MIDI messages also carry over to the output
  side, in order to translate MIDI input to MIDI output.  To make this
  work, you need to invoke the midizap program with the -o option, which
  equips the program with an additional MIDI output port, to which the
  translated MIDI messages are sent.  (Otherwise, MIDI messages in the
  output translations will be ignored.)

  Bindings can involve as many MIDI messages as you want, and these can
  be combined freely with keypress events in any order.  There's no
  limitation on the type or number of MIDI messages that you can put
  into a binding.

  Note that on output, the +-=<> suffixes aren't supported, because the
  *input* message determines whether it is a key press or value change
  type of event, and which direction it goes in the latter case.  Only
  the "~" suffix can be used to indicate an incremental CC message in
  sign bit encoding.

  Finally, on the output side there's a special token of the form
  CH<1..16>, which doesn't actually generate any MIDI message.  Rather,
  it sets the default MIDI channel for subsequent MIDI messages in the
  same output sequence, which is convenient if multiple messages are
  output to the same MIDI channel.

 */

#include "midizap.h"

int default_debug_regex = 0;
int default_debug_strokes = 0;
int default_debug_keys = 0;

int debug_regex = 0;
int debug_strokes = 0;
int debug_keys = 0;

char *
allocate(size_t len)
{
  char *ret = (char *)calloc(1, len);
  if (ret == NULL) {
    fprintf(stderr, "Out of memory!\n");
    exit(1);
  }
  return ret;
}

char *
alloc_strcat(char *a, char *b)
{
  size_t len = 0;
  char *result;

  if (a != NULL) {
    len += strlen(a);
  }
  if (b != NULL) {
    len += strlen(b);
  }
  result = allocate(len+1);
  result[0] = '\0';
  if (a != NULL) {
    strcpy(result, a);
  }
  if (b != NULL) {
    strcat(result, b);
  }
  return result;
}

static char *read_line_buffer = NULL;
static int read_line_buffer_length = 0;

#define BUF_GROWTH_STEP 1024


// read a line of text from the given file into a managed buffer.
// returns a partial line at EOF if the file does not end with \n.
// exits with error message on read error.
char *
read_line(FILE *f, char *name)
{
  int pos = 0;
  char *new_buffer;
  int new_buffer_length;

  if (read_line_buffer == NULL) {
    read_line_buffer_length = BUF_GROWTH_STEP;
    read_line_buffer = allocate(read_line_buffer_length);
    read_line_buffer[0] = '\0';
  }

  while (1) {
    read_line_buffer[read_line_buffer_length-1] = '\377';
    if (fgets(read_line_buffer+pos, read_line_buffer_length-pos, f) == NULL) {
      if (feof(f)) {
	if (pos > 0) {
	  // partial line at EOF
	  return read_line_buffer;
	} else {
	  return NULL;
	}
      }
      perror(name);
      exit(1);
    }
    if (read_line_buffer[read_line_buffer_length-1] != '\0') {
      return read_line_buffer;
    }
    if (read_line_buffer[read_line_buffer_length-2] == '\n') {
      return read_line_buffer;
    }
    new_buffer_length = read_line_buffer_length + BUF_GROWTH_STEP;
    new_buffer = allocate(new_buffer_length);
    memcpy(new_buffer, read_line_buffer, read_line_buffer_length);
    free(read_line_buffer);
    pos = read_line_buffer_length-1;
    read_line_buffer = new_buffer;
    read_line_buffer_length = new_buffer_length;
  }
}

static translation *first_translation_section = NULL;
static translation *last_translation_section = NULL;

translation *default_translation;

translation *
new_translation_section(char *name, char *regex)
{
  translation *ret = (translation *)allocate(sizeof(translation));
  int err;

  memset(ret, 0, sizeof(translation));
  if (debug_strokes) {
    printf("------------------------\n[%s] %s\n\n", name, regex);
  }
  ret->next = NULL;
  ret->name = alloc_strcat(name, NULL);
  if (regex == NULL || *regex == '\0') {
    ret->is_default = 1;
    default_translation = ret;
  } else {
    ret->is_default = 0;
    err = regcomp(&ret->regex, regex, REG_NOSUB);
    if (err != 0) {
      regerror(err, &ret->regex, read_line_buffer, read_line_buffer_length);
      fprintf(stderr, "error compiling regex for [%s]: %s\n", name, read_line_buffer);
      regfree(&ret->regex);
      free(ret->name);
      free(ret);
      return NULL;
    }
  }
  if (first_translation_section == NULL) {
    first_translation_section = ret;
    last_translation_section = ret;
  } else {
    last_translation_section->next = ret;
    last_translation_section = ret;
  }
  return ret;
}

void
free_strokes(stroke *s)
{
  stroke *next;
  while (s != NULL) {
    next = s->next;
    free(s);
    s = next;
  }
}

void
free_translation_section(translation *tr)
{
  int i, j;

  if (tr != NULL) {
    free(tr->name);
    if (!tr->is_default) {
      regfree(&tr->regex);
    }
    for (i=0; i<NUM_CHAN; i++) {
      for (j=0; j<NUM_KEYS; j++) {
	free_strokes(tr->pc[i][j][0]);
	free_strokes(tr->pc[i][j][1]);
	free_strokes(tr->note[i][j][0]);
	free_strokes(tr->note[i][j][1]);
	free_strokes(tr->cc[i][j][0]);
	free_strokes(tr->cc[i][j][1]);
	free_strokes(tr->ccs[i][j][0]);
	free_strokes(tr->ccs[i][j][1]);
      }
      free_strokes(tr->pb[i][0]);
      free_strokes(tr->pb[i][1]);
      free_strokes(tr->pbs[i][0]);
      free_strokes(tr->pbs[i][1]);
    }
    free(tr);
  }
}

void
free_all_translations(void)
{
  translation *tr = first_translation_section;
  translation *next;

  while (tr != NULL) {
    next = tr->next;
    free_translation_section(tr);
    tr = next;
  }
  first_translation_section = NULL;
  last_translation_section = NULL;
}

char *config_file_name = NULL;
static time_t config_file_modification_time;

static char *token_src = NULL;

// similar to strtok, but it tells us what delimiter was found at the
// end of the token, handles double quoted strings specially, and
// hardcodes the delimiter set.
char *
token(char *src, char *delim_found)
{
  char *delims = " \t\n/\"";
  char *d;
  char *token_start;

  if (src == NULL) {
    src = token_src;
  }
  if (src == NULL) {
    *delim_found = '\0';
    return NULL;
  }
  token_start = src;
  while (*src) {
    d = delims;
    while (*d && *src != *d) {
      d++;
    }
    if (*d) {
      if (src == token_start) {
	src++;
	token_start = src;
	if (*d == '"') {
	  while (*src && *src != '"' && *src != '\n') {
	    src++;
	  }
	} else {
	  continue;
	}
      }
      *delim_found = *d;
      if (*src) {
	*src = '\0';
	token_src = src+1;
      } else {
	token_src = NULL;
      }
      return token_start;
    }
    src++;
  }
  token_src = NULL;
  *delim_found = '\0';
  if (src == token_start) {
    return NULL;
  }
  return token_start;
}

typedef struct _keysymmapping {
  char *str;
  KeySym sym;
} keysymmapping;

static keysymmapping key_sym_mapping[] = {
#include "keys.h"
  { "XK_Button_1", XK_Button_1 },
  { "XK_Button_2", XK_Button_2 },
  { "XK_Button_3", XK_Button_3 },
  { "XK_Scroll_Up", XK_Scroll_Up },
  { "XK_Scroll_Down", XK_Scroll_Down },
  { NULL, 0 }
};

KeySym
string_to_KeySym(char *str)
{
  size_t len = strlen(str) + 1;
  int i = 0;

  while (key_sym_mapping[i].str != NULL) {
    if (!strncmp(str, key_sym_mapping[i].str, len)) {
      return key_sym_mapping[i].sym;
    }
    i++;
  }
  return 0;
}

char *
KeySym_to_string(KeySym ks)
{
  int i = 0;

  while (key_sym_mapping[i].sym != 0) {
    if (key_sym_mapping[i].sym == ks) {
      return key_sym_mapping[i].str;
    }
    i++;
  }
  return NULL;
}

static char *note_names[] = { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "G#", "A", "Bb", "B" };

void
print_stroke(stroke *s)
{
  char *str;

  if (s != NULL) {
    if (s->keysym) {
      str = KeySym_to_string(s->keysym);
      if (str == NULL) {
	printf("0x%x", (int)s->keysym);
	str = "???";
      }
      printf("%s/%c ", str, s->press ? 'D' : 'U');
    } else {
      int status = s->status & 0xf0;
      int channel = (s->status & 0x0f) + 1;
      switch (status) {
      case 0x90:
	printf("%s%d-%d ", note_names[s->data % 12], s->data / 12, channel);
	break;
      case 0xb0:
	if (s->step != 1)
	  printf("CC%d[%d]-%d%s ", s->data, s->step, channel, s->incr?"~":"");
	else
	  printf("CC%d-%d%s ", s->data, channel, s->incr?"~":"");
	break;
      case 0xc0:
	printf("PC%d-%d ", s->data, channel);
	break;
      case 0xe0:
	if (s->step != 1)
	  printf("PB[%d]-%d ", s->step, channel);
	else
	  printf("PB-%d ", channel);
	break;
      default: // this can't happen
	break;
      }
    }
  }
}

void
print_stroke_sequence(char *name, char *up_or_down, stroke *s)
{
  printf("%s[%s]: ", name, up_or_down);
  while (s) {
    print_stroke(s);
    s = s->next;
  }
  printf("\n");
}

stroke **first_stroke;
stroke *last_stroke;
stroke **press_first_stroke;
stroke **release_first_stroke;
int is_keystroke, is_bidirectional;
int is_midi;
char *current_translation;
char *key_name;
int first_release_stroke; // is this the first stroke of a release?
KeySym regular_key_down;

#define NUM_MODIFIERS 64

stroke modifiers_down[NUM_MODIFIERS];
int modifier_count;

int midi_channel;

void
append_stroke(KeySym sym, int press)
{
  stroke *s = (stroke *)allocate(sizeof(stroke));

  s->next = NULL;
  s->keysym = sym;
  s->press = press;
  s->status = s->data = s->step = s->incr = s->dirty = 0;
  if (*first_stroke) {
    last_stroke->next = s;
  } else {
    *first_stroke = s;
  }
  last_stroke = s;
}

void
append_midi(int status, int data, int step, int incr)
{
  stroke *s = (stroke *)allocate(sizeof(stroke));

  s->next = NULL;
  s->keysym = 0;
  s->press = 0;
  s->status = status;
  s->data = data;
  s->step = step;
  s->incr = incr;
  // if this is a keystroke event, for all messages but program change (which
  // has no "on" and "off" states), mark the event as "dirty" so that the
  // corresponding "off" event gets added later to the "release" strokes
  s->dirty = is_keystroke && ((status&0xf0) != 0xc0);
  if (*first_stroke) {
    last_stroke->next = s;
  } else {
    *first_stroke = s;
  }
  last_stroke = s;
  is_midi = 1;
}

// s->press values in modifiers_down:
// PRESS -> down
// HOLD -> held
// PRESS_RELEASE -> released, but to be re-pressed if necessary
// RELEASE -> up

void
mark_as_down(KeySym sym, int hold)
{
  int i;

  for (i=0; i<modifier_count; i++) {
    if (modifiers_down[i].keysym == sym) {
      modifiers_down[i].press = hold ? HOLD : PRESS;
      return;
    }
  }
  if (modifier_count > NUM_MODIFIERS) {
    fprintf(stderr, "too many modifiers down in [%s]%s\n", current_translation, key_name);
    return;
  }
  modifiers_down[modifier_count].keysym = sym;
  modifiers_down[modifier_count].press = hold ? HOLD : PRESS;
  modifier_count++;
}

void
mark_as_up(KeySym sym)
{
  int i;

  for (i=0; i<modifier_count; i++) {
    if (modifiers_down[i].keysym == sym) {
      modifiers_down[i].press = RELEASE;
      return;
    }
  }
}

void
release_modifiers(int allkeys)
{
  int i;

  for (i=0; i<modifier_count; i++) {
    if (modifiers_down[i].press == PRESS) {
      append_stroke(modifiers_down[i].keysym, 0);
      modifiers_down[i].press = PRESS_RELEASE;
    } else if (allkeys && modifiers_down[i].press == HOLD) {
      append_stroke(modifiers_down[i].keysym, 0);
      modifiers_down[i].press = RELEASE;
    }
  }
}

void
re_press_temp_modifiers(void)
{
  int i;

  for (i=0; i<modifier_count; i++) {
    if (modifiers_down[i].press == PRESS_RELEASE) {
      append_stroke(modifiers_down[i].keysym, 1);
      modifiers_down[i].press = PRESS;
    }
  }
}

/* Parser for the MIDI message syntax. The syntax we actually parse here is:

   tok  ::= ( note | msg ) [ number ] [ "[" number "]" ] [ "-" number] [ incr ]
   note ::= ( "a" | ... | "g" ) [ "#" | "b" ]
   msg  ::= "ch" | "pb" | "pc" | "cc"
   incr ::= "-" | "+" | "=" | "<" | ">" | "~"

   Numbers are always in decimal. The meaning of the first number depends on
   the context (octave number for notes, the actual data byte for other
   messages). This can optionally be followed by a number in brackets,
   denoting a step size. Also optionally, the suffix with the third number
   (after the dash) denotes the MIDI channel; otherwise the default MIDI
   channel is used.

   Note that not all combinations are possible -- "pb" has no data byte; only
   "cc" and "pb" may be followed by a step size in brackets; and "ch" must
   *not* occur as the first token and is followed by just a channel number.
   (In fact, "ch" is no real MIDI message at all; it just sets the default
   MIDI channel for subsequent messages in the output sequence.)

   The incr flag indicates an "incremental" controller or pitch bend value
   which responds to up ("+") and down ("-") changes; it is only permitted in
   conjunction with "cc" and "pb", and (with one exception, see below) only on
   the left-hand side of a translation. In addition, "<" and ">" can be used
   in lieu of "-" and "-" to indicate a relative controller in "sign bit"
   representation, where controller values > 64 denote down, and values < 64
   up changes. This notation is only permitted with "cc". It is used for
   endless rotary encoders, jog wheels and the like, as can be found, e.g., on
   Mackie-like units.

   Finally, the flags "=" and "~" are used in lieu of "+"/"-" or "<"/">",
   respectively, to denote a "bidirectional" translation which applies to both
   positive and negative changes of the controller or pitch bend value. Since
   bidirectional translations cannot have distinct keystroke sequences for up
   and down changes associated with them, this makes most sense with pure MIDI
   translations.

   The only incr flag which is also permitted on the right-hand side of a
   translation, and only with "cc", is the "~" flag, which is used to denote a
   relative (sign bit) controller change on output. */

static int note_number(char c, char b, int k)
{
  c = tolower(c); b = tolower(b);
  if (c < 'a' || c > 'g' || (b && b != '#' && b != 'b'))
    return -1; // either wrong note name or invalid accidental
  else {
    static int note_numbers[] = { -3, -1, 0, 2, 4, 5, 7 };
    int m = note_numbers[c-'a'], a = (b=='#')?1:(b=='b')?-1:0;
    if (m<0) k++;
    return m + a + 12*k;
  }
}

int
parse_midi(char *tok, char *s, int lhs,
	   int *status, int *data, int *step, int *incr, int *dir)
{
  char *p = tok, *t;
  int n, m = -1, k = midi_channel, l;
  s[0] = 0;
  while (*p && !isdigit(*p) && !strchr("+-=<>~[", *p)) p++;
  if (p == tok || p-tok > 10) return 0; // no valid token
  // the token by itself
  strncpy(s, tok, p-tok); s[p-tok] = 0;
  // normalize to lowercase
  for (t = s; *t; t++) *t = tolower(*t);
  // octave number or data byte (not permitted with 'pb', otherwise required)
  if (isdigit(*p) && sscanf(p, "%d%n", &m, &n) == 1) {
    if (strcmp(s, "pb") == 0) return 0;
    p += n;
  } else if (strcmp(s, "pb")) {
    return 0;
  }
  // step size ('cc' and 'pb' only)
  if (*p == '[') {
    if (strcmp(s, "cc") && strcmp(s, "pb")) return 0;
    if (sscanf(++p, "%d%n", &l, &n) == 1) {
      p += n;
      if (*p != ']') return 0;
      p++;
      *step = l;
    } else {
      return 0;
    }
  } else {
    *step = 1;
  }
  if (p[0] == '-' && isdigit(p[1])) {
    // suffix with MIDI channel (not permitted with 'ch')
    if (strcmp(s, "ch") == 0) return 0;
    if (sscanf(++p, "%d%n", &k, &n) == 1) {
      // check that it is a valid channel number
      if (k < 1 || k > 16) return 0;
      k--; // actual MIDI channel in the range 0..15
      p += n;
    } else {
      return 0;
    }
  }
  if (*p && strchr("+-=<>~", *p)) {
    // incremental flag ("pb" and "cc" only)
    if (strcmp(s, "pb") && strcmp(s, "cc")) return 0;
    // these are only permitted with "cc"
    if (strchr("<>~", *p) && strcmp(s, "cc")) return 0;
    if (lhs) {
      // *incr = 2 indicates an endless, sign-bit controller
      *incr = strchr("+-=", *p) ? 1 : 2;
      // *dir is -1 or +1 for down and up changes, but can also be zero for
      // *bidirectional translations ("=" and "~")
      *dir = (*p == '-' || *p == '<') ? -1 :
	(*p == '+' || *p == '>') ? 1 : 0;
    } else {
      // only the "~" form is permitted in output messages, where it indicates
      // an endless, sign-bit controller
      if (*p != '~') return 0;
      *incr = 2; *dir = 0;
    }
    p++;
  } else {
    *incr = *dir = 0;
  }
  // check for trailing garbage
  if (*p) return 0;
  if (strcmp(s, "ch") == 0) {
    if (lhs) return 0;
    // we return a bogus status of 0 here, along with the MIDI channel in the
    // data byte; also check that the MIDI channel is in the proper range
    if (m < 1 || m > 16) return 0;
    *status = 0; *data = m-1;
    return 1;
  } else if (strcmp(s, "pb") == 0) {
    // pitch bend, no data byte
    *status = 0xe0 | k; *data = 0;
    return 1;
  } else if (strcmp(s, "pc") == 0) {
    // program change
    if (m < 0 || m > 127) return 0;
    *status = 0xc0 | k; *data = m;
    return 1;
  } else if (strcmp(s, "cc") == 0) {
    // control change
    if (m < 0 || m > 127) return 0;
    *status = 0xb0 | k; *data = m;
    return 1;
  } else {
    // we must be looking at a MIDI note here, with m denoting the octave
    // number; first character is the note name (must be a..g); optionally,
    // the second character may denote an accidental (# or b)
    n = note_number(s[0], s[1], m);
    if (n < 0 || n > 127) return 0;
    *status = 0x90 | k; *data = n;
    return 1;
  }
}

int
start_translation(translation *tr, char *which_key)
{
  int status, data, step, incr, dir;
  char buf[100];

  //printf("start_translation(%s)\n", which_key);

  if (tr == NULL) {
    fprintf(stderr, "need to start translation section before defining key: %s\n", which_key);
    return 1;
  }
  current_translation = tr->name;
  key_name = which_key;
  is_keystroke = is_bidirectional = is_midi = 0;
  first_release_stroke = 0;
  regular_key_down = 0;
  modifier_count = 0;
  midi_channel = 0;
  if (parse_midi(which_key, buf, 1, &status, &data, &step, &incr, &dir)) {
    int chan = status & 0x0f;
    switch (status & 0xf0) {
    case 0x90:
      // note on/off
      first_stroke = &(tr->note[chan][data][0]);
      release_first_stroke = &(tr->note[chan][data][1]);
      is_keystroke = 1;
      break;
    case 0xc0:
      // pc: To make our live easier and for consistency with the other
      // messages, we treat this exactly like a note/cc on/off, even though
      // this message has no off state. Thus, when we receive a pc, it's
      // supposed to be treated as a "press" sequence immediately followed by
      // the corresponding "release" sequence.
      first_stroke = &(tr->pc[chan][data][0]);
      release_first_stroke = &(tr->pc[chan][data][1]);
      is_keystroke = 1;
      break;
    case 0xb0:
      if (!incr) {
	// cc on/off
	first_stroke = &(tr->cc[chan][data][0]);
	release_first_stroke = &(tr->cc[chan][data][1]);
	is_keystroke = 1;
      } else {
	// cc (step up, down)
	tr->is_incr[chan][data] = incr>1;
	first_stroke = &(tr->ccs[chan][data][dir>0]);
	tr->cc_step[chan][data][dir>0] = step;
	if (!dir) {
	  // This is a bidirectional translation (=, ~). We first fill in the
	  // "down" part (pointed to by first_stroke). When finishing off the
	  // translation, we then create an exact duplicate of the sequence
	  // for the "up" part. Note that we (ab)use the release_first_stroke
	  // variable, which normally records the release part of a key
	  // translation, here to remember the "up" part of the translation,
	  // so that we can fill in that part later.
	  is_bidirectional = 1;
	  release_first_stroke = &(tr->ccs[chan][data][1]);
	  tr->cc_step[chan][data][1] = step;
	}
      }
      break;
    case 0xe0:
      if (!incr) {
	// pb on/off
	first_stroke = &(tr->pb[chan][0]);
	release_first_stroke = &(tr->pb[chan][1]);
	is_keystroke = 1;
      } else {
	// pb (step up, down)
	if (step <= 0) {
	  fprintf(stderr, "zero or negative step size not permitted here: [%s]%s\n", current_translation, which_key);
	  return 1;
	}
	first_stroke = &(tr->pbs[chan][dir>0]);
	tr->pb_step[chan][dir>0] = step;
	if (!dir) {
	  is_bidirectional = 1;
	  release_first_stroke = &(tr->pbs[chan][1]);
	}
      }
      break;
    default:
      // this can't happen
      fprintf(stderr, "bad message name: [%s]%s\n", current_translation, which_key);
      return 1;
    }
  } else {
    fprintf(stderr, "bad message name: [%s]%s\n", current_translation, which_key);
    return 1;
  }
  if (*first_stroke != NULL ||
      (is_bidirectional && *release_first_stroke != NULL)) {
    fprintf(stderr, "can't redefine message: [%s]%s\n", current_translation, which_key);
    return 1;
  }
  press_first_stroke = first_stroke;
  return 0;
}

void
add_keysym(KeySym sym, int press_release)
{
  //printf("add_keysym(0x%x, %d)\n", (int)sym, press_release);
  switch (press_release) {
  case PRESS:
    append_stroke(sym, 1);
    mark_as_down(sym, 0);
    break;
  case RELEASE:
    append_stroke(sym, 0);
    mark_as_up(sym);
    break;
  case HOLD:
    append_stroke(sym, 1);
    mark_as_down(sym, 1);
    break;
  case PRESS_RELEASE:
  default:
    if (first_release_stroke) {
      re_press_temp_modifiers();
    }
    if (regular_key_down != 0) {
      append_stroke(regular_key_down, 0);
    }
    append_stroke(sym, 1);
    regular_key_down = sym;
    first_release_stroke = 0;
    break;
  }
}

void
add_release(int all_keys)
{
  //printf("add_release(%d)\n", all_keys);
  release_modifiers(all_keys);
  if (!all_keys) {
    first_stroke = release_first_stroke;
    if (is_midi) {
      // walk the list of "press" strokes, find all "dirty" (as yet unhandled)
      // MIDI events in there and add them to the "release" strokes
      stroke *s = *press_first_stroke;
      while (s) {
	if (!s->keysym && s->dirty) {
	  append_midi(s->status, s->data, s->step, s->incr);
	  s->dirty = 0;
	}
	s = s->next;
      }
    }
  }
  if (regular_key_down != 0) {
    append_stroke(regular_key_down, 0);
  }
  regular_key_down = 0;
  first_release_stroke = 1;
  if (all_keys && is_bidirectional) {
    // create a duplicate for bidirectional translations (=, ~)
    stroke *s = *press_first_stroke;
    first_stroke = release_first_stroke;
    while (s) {
      if (s->keysym) {
	append_stroke(s->keysym, s->press);
      } else {
	append_midi(s->status, s->data, s->step, s->incr);
      }
      s = s->next;
    }
  }
}

void
add_keystroke(char *keySymName, int press_release)
{
  KeySym sym;

  if (is_keystroke && !strncmp(keySymName, "RELEASE", 8)) {
    add_release(0);
    return;
  }
  sym = string_to_KeySym(keySymName);
  if (sym != 0) {
    add_keysym(sym, press_release);
  } else {
    fprintf(stderr, "unrecognized KeySym: %s\n", keySymName);
  }
}

void
add_string(char *str)
{
  while (str && *str) {
    if (*str >= ' ' && *str <= '~') {
      add_keysym((KeySym)(*str), PRESS_RELEASE);
    }
    str++;
  }
}

void
add_midi(char *tok)
{
  int status, data, step, incr, dir = 0;
  char buf[100];
  if (parse_midi(tok, buf, 0, &status, &data, &step, &incr, &dir)) {
    if (status == 0) {
      // 'ch' token; this doesn't actually generate any output, it just sets
      // the default MIDI channel
      midi_channel = data;
    } else {
      if ((status & 0xf0) != 0xe0 || step != 0)
	append_midi(status, data, step, incr!=0);
      else
	fprintf(stderr, "zero step size not permitted: %s\n", tok);
    }
  } else {
    // inspect the token that was actually recognized (if any) to give some
    // useful error message here
    if (strcmp(buf, "ch"))
      fprintf(stderr, "bad MIDI message: %s\n", tok);
    else
      fprintf(stderr, "bad MIDI channel: %s\n", tok);
  }
}

void
finish_translation(void)
{
  //printf("finish_translation()\n");
  if (is_keystroke) {
    add_release(0);
  }
  add_release(1);
  if (debug_strokes) {
    if (is_keystroke) {
      print_stroke_sequence(key_name, "D", *press_first_stroke);
      print_stroke_sequence(key_name, "U", *release_first_stroke);
    } else {
      print_stroke_sequence(key_name, "", *first_stroke);
    }
    printf("\n");
  }
}

int
read_config_file(void)
{
  struct stat buf;
  char *home;
  char *line;
  char *s;
  char *name = NULL;
  char *regex;
  char *tok;
  char *which_key;
  char *updown;
  char delim;
  translation *tr = NULL;
  FILE *f;
  int config_file_default = 0;
  static int errors = 0;

  if (config_file_name == NULL) {
    config_file_name = getenv("MIDIZAP_CONFIG_FILE");
    if (config_file_name == NULL) {
      home = getenv("HOME");
      config_file_name = alloc_strcat(home, "/.midizaprc");
      config_file_default = 1;
    } else {
      config_file_name = alloc_strcat(config_file_name, NULL);
    }
    config_file_modification_time = 0;
  }
  if (stat(config_file_name, &buf) < 0) {
    // AG: Fall back to the system-wide configuration file.
    if (!config_file_default && !errors) {
      perror(config_file_name);
      errors++;
    }
    config_file_name = "/etc/midizaprc";
    config_file_modification_time = 0;
  }
  if (stat(config_file_name, &buf) < 0) {
    if (!errors) {
      perror(config_file_name);
      errors++;
    }
    return 0;
  }
  if (buf.st_mtime == 0) {
    buf.st_mtime = 1;
  }
  if (buf.st_mtime > config_file_modification_time) {
    config_file_modification_time = buf.st_mtime;
    if (default_debug_regex || default_debug_strokes || default_debug_keys) {
      printf("Loading configuration: %s\n", config_file_name);
    }

    f = fopen(config_file_name, "r");
    if (f == NULL) {
      if (!errors) {
	perror(config_file_name);
	errors++;
      }
      return 0;
    }

    free_all_translations();
    debug_regex = default_debug_regex;
    debug_strokes = default_debug_strokes;
    debug_keys = default_debug_keys;

    while ((line=read_line(f, config_file_name)) != NULL) {
      //printf("line: %s", line);
      
      s = line;
      while (*s && isspace(*s)) {
	s++;
      }
      if (*s == '#') {
	continue;
      }
      if (*s == '[') {
	//  [name] regex\n
	name = ++s;
	while (*s && *s != ']') {
	  s++;
	}
	regex = NULL;
	if (*s) {
	  *s = '\0';
	  s++;
	  while (*s && isspace(*s)) {
	    s++;
	  }
	  regex = s;
	  while (*s) {
	    s++;
	  }
	  s--;
	  while (s > regex && isspace(*s)) {
	    s--;
	  }
	  s[1] = '\0';
	}
	tr = new_translation_section(name, regex);
	continue;
      }

      tok = token(s, &delim);
      if (tok == NULL) {
	continue;
      }
      if (!strcmp(tok, "DEBUG_REGEX")) {
	debug_regex = 1;
	continue;
      }
      if (!strcmp(tok, "DEBUG_STROKES")) {
	debug_strokes = 1;
	continue;
      }
      if (!strcmp(tok, "DEBUG_KEYS")) {
	debug_keys = 1;
	continue;
      }
      which_key = tok;
      if (start_translation(tr, which_key)) {
	continue;
      }
      tok = token(NULL, &delim);
      while (tok != NULL) {
	if (delim != '"' && tok[0] == '#') {
	  break; // skip rest as comment
	}
	//printf("token: [%s] delim [%d]\n", tok, delim);
	switch (delim) {
	case ' ':
	case '\t':
	case '\n':
	  if (strncmp(tok, "XK", 2) && strncmp(tok, "RELEASE", 8))
	    add_midi(tok);
	  else
	    add_keystroke(tok, PRESS_RELEASE);
	  break;
	case '"':
	  add_string(tok);
	  break;
	default: // should be slash
	  updown = token(NULL, &delim);
	  if (updown != NULL) {
	    switch (updown[0]) {
	    case 'U':
	      add_keystroke(tok, RELEASE);
	      break;
	    case 'D':
	      add_keystroke(tok, PRESS);
	      break;
	    case 'H':
	      add_keystroke(tok, HOLD);
	      break;
	    default:
	      fprintf(stderr, "invalid up/down modifier [%s]%s: %s\n", name, which_key, updown);
	      add_keystroke(tok, PRESS);
	      break;
	    }
	  }
	}
	tok = token(NULL, &delim);
      }
      finish_translation();
    }

    fclose(f);
    return 1;

  } else {
    return 0;
  }
}

translation *
get_translation(char *win_title, char *win_class)
{
  translation *tr;

  read_config_file();
  tr = first_translation_section;
  while (tr != NULL) {
    extern int enable_jack_output;
    if (tr->is_default &&
	(strcmp(tr->name, "MIDI") || enable_jack_output)) {
      return tr;
    } else if (!tr->is_default) {
      // AG: We first try to match the class name, since it usually provides
      // better identification clues.
      if (win_class && *win_class &&
	  regexec(&tr->regex, win_class, 0, NULL, 0) == 0) {
	return tr;
      }
      if (regexec(&tr->regex, win_title, 0, NULL, 0) == 0) {
	return tr;
      }
    }
    tr = tr->next;
  }
  return NULL;
}
