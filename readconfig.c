
/*

  Copyright 2013 Eric Messick (FixedImagePhoto.com/Contact)
  Copyright 2018 Albert Graef <aggraef@gmail.com>

  Read and process the configuration file ~/.midizaprc

  Lines starting with # are comments.

  The file is a sequence of sections defining translation classes. Each
  section takes the following form:

  [name] regex
  CC<0..127> output             # control change
  PC<0..127> output             # program change
  PB output                     # pitch bend
  CP output                     # channel pressure
  KP:<A-G>[#b]<-11..11> output  # key pressure (aftertouch)
  <A-G>[#b]<-11..11> output     # note

  When focus is on a window whose class or title matches regex, the
  following translation class is in effect.  An empty regex for the last
  class will always match, allowing default translations.  Any output
  sequences not bound in a matched section will be loaded from the
  default section if they are bound there.

  Each "[name] regex" line introduces the list of MIDI message
  translations for the named translation class.  The name is only used
  for debugging output, and needn't be unique.  The following lines
  indicate what output should be produced for the given MIDI messages.

  MIDI messages are on channel 1 by default; a suffix of the form
  -<1..16> can be used to specify a different MIDI channel.  E.g., C3-10
  denotes note C3 on MIDI channel 10.

  Note messages are specified using the cutomary notation (note name
  A..G, optionally followed by an accidental, # or b, followed by a
  (zero-based) MIDI octave number. Note that all MIDI octaves start at
  the note C, so B0 comes before C1.  By default, C5 denotes middle C, A5
  is the chamber pitch (usually at 440 Hz).  Enharmonic spellings are
  equivalent, so, e.g., D# and Eb denote exactly the same MIDI note.

  More details on the syntax of MIDI messages can be found in the
  comments preceding the parse_midi() routine below.

 */

#include "midizap.h"

int default_debug_regex = 0;
int default_debug_strokes = 0;
int default_debug_keys = 0;
int default_debug_midi = 0;

int debug_regex = 0;
int debug_strokes = 0;
int debug_keys = 0;
int debug_midi = 0;

int midi_octave = 0;

char *jack_client_name = NULL;

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
translation *default_translation, *default_midi_translation[2];

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
    if (!strcmp(name, "MIDI"))
      default_midi_translation[0] = ret;
    else if (!strcmp(name, "MIDI2")) {
      default_midi_translation[1] = ret;
      ret->portno = 1;
    } else
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

static int stroke_data_cmp(const void *a, const void *b)
{
  const stroke_data *ad = (const stroke_data*)a;
  const stroke_data *bd = (const stroke_data*)b;
  if (ad->chan == bd->chan)
    return ad->data - bd->data;
  else
    return ad->chan - bd->chan;
}

static void finish_stroke_data(stroke_data **sd,
			       uint16_t *n, uint16_t *a)
{
  if (*a && *a > *n) {
    // realloc to needed size
    *sd = realloc(*sd, (*n)*sizeof(stroke_data));
    *a = *n;
  }
  // sort by chan/data for faster access
  qsort(*sd, *n, sizeof(stroke_data), stroke_data_cmp);
}

static void free_stroke_data(stroke_data *sd, uint16_t n)
{
  uint16_t i;
  for (i = 0; i < n; i++) {
    free_strokes(sd[i].s[0]);
    free_strokes(sd[i].s[1]);
  }
  free(sd);
}

static stroke **find_stroke_data(stroke_data **sd,
				 int chan, int data, int index,
				 int step, int incr,
				 uint16_t *n, uint16_t *a)
{
  uint16_t i;
  for (i = 0; i < *n; i++) {
    if ((*sd)[i].chan == chan && (*sd)[i].data == data) {
      // existing entry
      (*sd)[i].step[index] = step;
      (*sd)[i].is_incr = incr;
      return &(*sd)[i].s[index];
    }
  }
  // add a new entry
  if (*n >= *a) {
    // make some room
    *a = (*a)?2*(*a):8;
    *sd = realloc(*sd, (*a)*sizeof(stroke_data));
  }
  memset(&(*sd)[*n], 0, sizeof(stroke_data));
  (*sd)[*n].chan = chan;
  (*sd)[*n].data = data;
  (*sd)[*n].step[index] = step;
  (*sd)[*n].is_incr = incr;
  return &(*sd)[(*n)++].s[index];
}

static stroke **find_note(translation *tr, int shift,
			  int chan, int data, int index)
{
  return find_stroke_data(&tr->note[shift], chan, data, index, 0, 0,
			  &tr->n_note[shift], &tr->a_note[shift]);
}

static stroke **find_pc(translation *tr, int shift,
			 int chan, int data, int index)
{
  return find_stroke_data(&tr->pc[shift], chan, data, index, 0, 0,
			  &tr->n_pc[shift], &tr->a_pc[shift]);
}

static stroke **find_cc(translation *tr, int shift,
			 int chan, int data, int index)
{
  return find_stroke_data(&tr->cc[shift], chan, data, index, 0, 0,
			  &tr->n_cc[shift], &tr->a_cc[shift]);
}

static stroke **find_ccs(translation *tr, int shift,
			 int chan, int data, int index, int step, int incr)
{
  return find_stroke_data(&tr->ccs[shift], chan, data, index, step, incr,
			  &tr->n_ccs[shift], &tr->a_ccs[shift]);
}

static stroke **find_kp(translation *tr, int shift,
			 int chan, int data, int index)
{
  return find_stroke_data(&tr->kp[shift], chan, data, index, 0, 0,
			  &tr->n_kp[shift], &tr->a_kp[shift]);
}

static stroke **find_kps(translation *tr, int shift,
			 int chan, int data, int index, int step)
{
  return find_stroke_data(&tr->kps[shift], chan, data, index, step, 0,
			  &tr->n_kps[shift], &tr->a_kps[shift]);
}

static stroke **find_cp(translation *tr, int shift,
			 int chan, int index)
{
  return find_stroke_data(&tr->cp[shift], chan, 0, index, 0, 0,
			  &tr->n_cp[shift], &tr->a_cp[shift]);
}

static stroke **find_cps(translation *tr, int shift,
			 int chan, int index, int step)
{
  return find_stroke_data(&tr->cps[shift], chan, 0, index, step, 0,
			  &tr->n_cps[shift], &tr->a_cps[shift]);
}

static stroke **find_pb(translation *tr, int shift,
			 int chan, int index)
{
  return find_stroke_data(&tr->pb[shift], chan, 0, index, 0, 0,
			  &tr->n_pb[shift], &tr->a_pb[shift]);
}

static stroke **find_pbs(translation *tr, int shift,
			 int chan, int index, int step)
{
  return find_stroke_data(&tr->pbs[shift], chan, 0, index, step, 0,
			  &tr->n_pbs[shift], &tr->a_pbs[shift]);
}

void
finish_translation_section(translation *tr)
{
  int k;

  if (tr) {
    for (k=0; k<2; k++) {
      finish_stroke_data(&tr->pc[k], &tr->n_pc[k], &tr->a_pc[k]);
      finish_stroke_data(&tr->note[k], &tr->n_note[k], &tr->a_note[k]);
      finish_stroke_data(&tr->cc[k], &tr->n_cc[k], &tr->a_cc[k]);
      finish_stroke_data(&tr->ccs[k], &tr->n_ccs[k], &tr->a_ccs[k]);
      finish_stroke_data(&tr->pb[k], &tr->n_pb[k], &tr->a_pb[k]);
      finish_stroke_data(&tr->pbs[k], &tr->n_pbs[k], &tr->a_pbs[k]);
    }
  }
}

void
free_translation_section(translation *tr)
{
  int k;

  if (tr != NULL) {
    free(tr->name);
    if (!tr->is_default) {
      regfree(&tr->regex);
    }
    for (k=0; k<2; k++) {
      free_stroke_data(tr->pc[k], tr->n_pc[k]);
      free_stroke_data(tr->note[k], tr->n_note[k]);
      free_stroke_data(tr->cc[k], tr->n_cc[k]);
      free_stroke_data(tr->ccs[k], tr->n_ccs[k]);
      free_stroke_data(tr->pb[k], tr->n_pb[k]);
      free_stroke_data(tr->pbs[k], tr->n_pbs[k]);
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
  default_translation = default_midi_translation[0] =
    default_midi_translation[1] = NULL;
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
    } else if (s->shift) {
      printf("SHIFT ");
    } else {
      int status = s->status & 0xf0;
      int channel = (s->status & 0x0f) + 1;
      switch (status) {
      case 0x90:
	if (s->step)
	  printf("%s%d[%d]-%d ", note_names[s->data % 12],
		 s->data / 12 + midi_octave, s->step, channel);
	else
	  printf("%s%d-%d ", note_names[s->data % 12],
		 s->data / 12 + midi_octave, channel);
	break;
      case 0xa0:
	if (s->step)
	  printf("KP:%s%d[%d]-%d ", note_names[s->data % 12],
		 s->data / 12 + midi_octave, s->step, channel);
	else
	  printf("KP:%s%d-%d ", note_names[s->data % 12],
		 s->data / 12 + midi_octave, channel);
	break;
      case 0xb0:
	if (s->step)
	  printf("CC%d[%d]-%d%s ", s->data, s->step, channel, s->incr?"~":"");
	else
	  printf("CC%d-%d%s ", s->data, channel, s->incr?"~":"");
	break;
      case 0xc0:
	printf("PC%d-%d ", s->data, channel);
	break;
      case 0xd0:
	if (s->step)
	  printf("CP[%d]-%d ", s->step, channel);
	else
	  printf("CP-%d ", channel);
	break;
      case 0xe0:
	if (s->step)
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
  if (up_or_down && *up_or_down)
    printf("%s[%s]: ", name, up_or_down);
  else
    printf("%s: ", name);
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
stroke **alt_press_stroke;
stroke **alt_release_stroke;
int is_keystroke, is_bidirectional, is_anyshift, is_midi, mode;
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
  s->shift = 0;
  s->status = s->data = s->step = s->incr = s->dirty = 0;
  if (*first_stroke) {
    last_stroke->next = s;
  } else {
    *first_stroke = s;
  }
  last_stroke = s;
}

void
append_shift(void)
{
  stroke *s = (stroke *)allocate(sizeof(stroke));

  s->next = NULL;
  s->shift = 1;
  s->keysym = 0;
  s->press = 0;
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
  s->shift = 0;
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

/* Parser for the MIDI message syntax. The same parser is used for both
   the left-hand side (lhs) and the right-hand side (rhs) of a translation.
   The syntax we actually parse here is:

   tok  ::= ( note | msg ) [ number ] [ "[" number "]" ] [ "-" number] [ incr ]
   note ::= ( "a" | ... | "g" ) [ "#" | "b" ]
   msg  ::= "ch" | "pb" | "pc" | "cc" | "cp" | "kp:" note
   incr ::= "-" | "+" | "=" | "<" | ">" | "~"

   Case is insignificant. Numbers are always in decimal. The meaning of
   the first number depends on the context (octave number for notes and
   key pressure, the actual data byte for other messages). This can
   optionally be followed by a number in brackets, denoting a step
   size. Also optionally, the suffix with the third number (after the
   dash) denotes the MIDI channel; otherwise the default MIDI channel is
   used.

   Note that not all combinations are possible -- "pb" and "cp" have no
   data byte; on the lhs, a step size in brackets is only permitted with
   "cc", "pb", "cp" and "kp"; and "ch" must *not* occur on the lhs at
   all, and is followed by just a channel number.  (In fact, "ch" is no
   real MIDI message at all; it just sets the default MIDI channel for
   subsequent messages in the output sequence.)

   The incr flag indicates an "incremental" controller or pitch bend
   value which responds to up ("+") and down ("-") changes; it is only
   permitted in conjunction with "cc", "pb", "cp" and "kp", and (with
   one exception, see below) only on the lhs of a translation. In
   addition, "<" and ">" can be used in lieu of "-" and "-" to indicate
   a relative controller in "sign bit" representation, where controller
   values > 64 denote down, and values < 64 up changes. This notation is
   only permitted with "cc". It is used for endless rotary encoders, jog
   wheels and the like, as can be found, e.g., on Mackie-like units.

   Finally, the flags "=" and "~" are used in lieu of "+"/"-" or
   "<"/">", respectively, to denote a "bidirectional" translation which
   applies to both positive and negative changes of the controller or
   pitch bend value. Since bidirectional translations cannot have
   distinct keystroke sequences for up and down changes associated with
   them, this makes most sense with pure MIDI translations.

   The only incr flag which is also permitted on the rhs of a
   translation, and only with "cc", is the "~" flag, which is used to
   denote a relative (sign bit) controller change on output. */

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
parse_midi(char *tok, char *s, int lhs, int mode,
	   int *status, int *data, int *step, int *incr, int *dir)
{
  char *p = tok, *t;
  int n, m = -1, k = midi_channel, l;
  s[0] = 0;
  while (*p && !isdigit(*p) && !strchr("+-=<>~[:", *p)) p++;
  if (p == tok || p-tok > 10) return 0; // no valid token
  // the token by itself
  strncpy(s, tok, p-tok); s[p-tok] = 0;
  // normalize to lowercase
  for (t = s; *t; t++) *t = tolower(*t);
  // octave number or data byte
  if (strcmp(s, "pb") && strcmp(s, "cp")) {
    if ((*p == '-' || isdigit(*p))) {
      if (sscanf(p, "%d%n", &m, &n) == 1)
	p += n;
      else
	return 0;
    } else if (!strcmp(s, "kp")) {
      // key pressure, must be followed by colon and note name
      if (*p == ':' && p[1]) {
	char c = *++p, b = *++p;
	if (*p == '#' || tolower(*p) == 'b')
	  p++;
	else
	  b = 0;
	int k = note_number(c, b, 0);
	if (k < 0) return 0;
	if ((*p == '-' || isdigit(*p)) &&
	    sscanf(p, "%d%n", &m, &n) == 1) {
	  // octave number
	  m = k + 12 * (m - midi_octave);
	  p += n;
	} else {
	  return 0;
	}
      } else {
	return 0;
      }
    } else {
      return 0;
    }
  }
  // step size
  if (*p == '[') {
    if (sscanf(++p, "%d%n", &l, &n) == 1) {
      if (!l || (lhs && l<0)) return 0; // must be nonzero / positive on lhs
      p += n;
      if (*p != ']') return 0;
      p++;
      *step = l;
    } else {
      return 0;
    }
  } else {
    // sentinel value; for the lhs, this will be filled in below; for
    // the rhs this indicates the default value
    *step = 0;
  }
  // suffix with MIDI channel (not permitted with 'ch')
  if (p[0] == '-' && isdigit(p[1])) {
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
  // incremental flag ("cc", "pb", "cp" and "kp" only)
  if (*p && strchr("+-=<>~", *p)) {
    if (strcmp(s, "pb") && strcmp(s, "cc") &&
	strcmp(s, "cp") && strcmp(s, "kp")) return 0;
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
      // an endless, sign-bit encoder
      if (*p != '~')
	return 0;
      else if (!mode)
	fprintf(stderr, "warning: incremental flag ignored in key mode: %s\n", tok);
      *incr = 2; *dir = 0;
    }
    p++;
  } else {
    *incr = *dir = 0;
  }
  // check for trailing garbage
  if (*p) return 0;
  // check for the different messages types we support
  if (strcmp(s, "ch") == 0) {
    if (lhs) return 0; // not permitted on lhs
    if (*step) return 0; // step size not permitted
    // we return a bogus status of 0 here, along with the MIDI channel
    // in the data byte; also check that the MIDI channel is in the
    // proper range
    if (m < 1 || m > 16) return 0;
    *status = 0; *data = m-1;
    return 1;
  } else if (strcmp(s, "pb") == 0) {
    // pitch bend, no data byte
    *status = 0xe0 | k; *data = 0;
    // negative step size is always permitted on rhs here, even in key mode
    // step size only permitted on lhs if incremental
    if (lhs && *step && !*incr) return 0;
    if (lhs && !*step) *step = 1; // default
    return 1;
  } else if (strcmp(s, "cp") == 0) {
    // channel pressure, no data byte
    *status = 0xd0 | k; *data = 0;
    // negative step size not permitted on rhs in key mode
    if (!lhs && *step < 0 && !mode) return 0;
    // step size only permitted on lhs if incremental
    if (lhs && *step && !*incr) return 0;
    if (lhs && !*step) *step = 1; // default
    return 1;
  } else if (strcmp(s, "pc") == 0) {
    // program change
    if (*step) return 0; // step size not permitted
    if (m < 0 || m > 127) return 0;
    *status = 0xc0 | k; *data = m;
    return 1;
  } else if (strcmp(s, "cc") == 0) {
    // control change
    if (m < 0 || m > 127) return 0;
    *status = 0xb0 | k; *data = m;
    // negative step size not permitted on rhs in key mode
    if (!lhs && *step < 0 && !mode) return 0;
    // step size only permitted on lhs if incremental
    if (lhs && *step && !*incr) return 0;
    if (lhs && !*step) *step = 1; // default
    return 1;
  } else if (strcmp(s, "kp") == 0) {
    // key pressure
    if (m < 0 || m > 127) return 0;
    *status = 0xa0 | k; *data = m;
    // negative step size not permitted on rhs in key mode
    if (!lhs && *step < 0 && !mode) return 0;
    // step size only permitted on lhs if incremental
    if (lhs && *step && !*incr) return 0;
    if (lhs && !*step) *step = 1; // default
    return 1;
  } else {
    // negative step size not permitted on rhs
    if (!lhs && *step < 0) return 0;
    // step size not permitted on lhs
    if (lhs && *step) return 0;
    // we must be looking at a MIDI note here, with m denoting the
    // octave number; first character is the note name (must be a..g);
    // optionally, the second character may denote an accidental (# or b)
    n = note_number(s[0], s[1], m - midi_octave);
    if (n < 0 || n > 127) return 0;
    *status = 0x90 | k; *data = n;
    return 1;
  }
}

int
start_translation(translation *tr, char *which_key)
{
  int k, status, data, step, incr, dir;
  char buf[100];

  //printf("start_translation(%s)\n", which_key);

  if (tr == NULL) {
    fprintf(stderr, "need to start translation section before defining key: %s\n", which_key);
    return 1;
  }
  current_translation = tr->name;
  key_name = which_key;
  is_keystroke = is_bidirectional = is_midi = is_anyshift = 0;
  first_release_stroke = 0;
  regular_key_down = 0;
  modifier_count = 0;
  midi_channel = 0;
  k = *which_key == '^' || (is_anyshift = *which_key == '?');
  if (parse_midi(which_key+k, buf, 1, 0, &status, &data, &step, &incr, &dir)) {
    int chan = status & 0x0f;
    mode = !!incr;
    switch (status & 0xf0) {
    case 0x90:
      // note on/off
      first_stroke = find_note(tr, k, chan, data, 0);
      release_first_stroke = find_note(tr, k, chan, data, 1);
      if (is_anyshift) {
	alt_press_stroke = find_note(tr, 0, chan, data, 0);
	alt_release_stroke = find_note(tr, 0, chan, data, 1);
      }
      is_keystroke = 1;
      break;
    case 0xc0:
      // pc: To make our live easier and for consistency with the other
      // messages, we treat this exactly like a note/cc on/off, even though
      // this message has no off state. Thus, when we receive a pc, it's
      // supposed to be treated as a "press" sequence immediately followed by
      // the corresponding "release" sequence.
      first_stroke = find_pc(tr, k, chan, data, 0);
      release_first_stroke = find_pc(tr, k, chan, data, 1);
      if (is_anyshift) {
	alt_press_stroke = find_pc(tr, 0, chan, data, 0);
	alt_release_stroke = find_pc(tr, 0, chan, data, 1);
      }
      is_keystroke = 1;
      break;
    case 0xb0:
      if (!incr) {
	// cc on/off
	first_stroke = find_cc(tr, k, chan, data, 0);
	release_first_stroke = find_cc(tr, k, chan, data, 1);
	if (is_anyshift) {
	  alt_press_stroke = find_cc(tr, 0, chan, data, 0);
	  alt_release_stroke = find_cc(tr, 0, chan, data, 1);
	}
	is_keystroke = 1;
      } else {
	// cc (step up, down)
	if (step <= 0) {
	  fprintf(stderr, "zero or negative step size not permitted here: [%s]%s\n", current_translation, which_key);
	  return 1;
	}
	first_stroke = find_ccs(tr, k, chan, data, dir>0, step, incr>1);
	if (is_anyshift) {
	  alt_press_stroke = find_ccs(tr, 0, chan, data, dir>0, step, incr>1);
	}
	if (!dir) {
	  // This is a bidirectional translation (=, ~). We first fill in the
	  // "down" part (pointed to by first_stroke). When finishing off the
	  // translation, we then create an exact duplicate of the sequence
	  // for the "up" part. Note that we (ab)use the release_first_stroke
	  // variable, which normally records the release part of a key
	  // translation, here to remember the "up" part of the translation,
	  // so that we can fill in that part later.
	  is_bidirectional = 1;
	  release_first_stroke = find_ccs(tr, k, chan, data, 1, step, incr>1);
	  if (is_anyshift) {
	    alt_release_stroke = find_ccs(tr, 0, chan, data, 1, step, incr>1);
	  }
	}
      }
      break;
    case 0xa0:
      if (!incr) {
	// kp on/off
	first_stroke = find_kp(tr, k, chan, data, 0);
	release_first_stroke = find_kp(tr, k, chan, data, 1);
	if (is_anyshift) {
	  alt_press_stroke = find_kp(tr, 0, chan, data, 0);
	  alt_release_stroke = find_kp(tr, 0, chan, data, 1);
	}
	is_keystroke = 1;
      } else {
	// kp (step up, down)
	if (step <= 0) {
	  fprintf(stderr, "zero or negative step size not permitted here: [%s]%s\n", current_translation, which_key);
	  return 1;
	}
	first_stroke = find_kps(tr, k, chan, data, dir>0, step);
	if (is_anyshift) {
	  alt_press_stroke = find_kps(tr, 0, chan, data, dir>0, step);
	}
	if (!dir) {
	  is_bidirectional = 1;
	  release_first_stroke = find_kps(tr, k, chan, data, 1, step);
	  if (is_anyshift) {
	    alt_release_stroke = find_kps(tr, 0, chan, data, 1, step);
	  }
	}
      }
      break;
    case 0xd0:
      if (!incr) {
	// cp on/off
	first_stroke = find_cp(tr, k, chan, 0);
	release_first_stroke = find_cp(tr, k, chan, 1);
	if (is_anyshift) {
	  alt_press_stroke = find_cp(tr, 0, chan, 0);
	  alt_release_stroke = find_cp(tr, 0, chan, 1);
	}
	is_keystroke = 1;
      } else {
	// cp (step up, down)
	if (step <= 0) {
	  fprintf(stderr, "zero or negative step size not permitted here: [%s]%s\n", current_translation, which_key);
	  return 1;
	}
	first_stroke = find_cps(tr, k, chan, dir>0, step);
	if (is_anyshift) {
	  alt_press_stroke = find_cps(tr, 0, chan, dir>0, step);
	}
	if (!dir) {
	  is_bidirectional = 1;
	  release_first_stroke = find_cps(tr, k, chan, 1, step);
	  if (is_anyshift) {
	    alt_release_stroke = find_cps(tr, 0, chan, 1, step);
	  }
	}
      }
      break;
    case 0xe0:
      if (!incr) {
	// pb on/off
	first_stroke = find_pb(tr, k, chan, 0);
	release_first_stroke = find_pb(tr, k, chan, 1);
	if (is_anyshift) {
	  alt_press_stroke = find_pb(tr, 0, chan, 0);
	  alt_release_stroke = find_pb(tr, 0, chan, 1);
	}
	is_keystroke = 1;
      } else {
	// pb (step up, down)
	if (step <= 0) {
	  fprintf(stderr, "zero or negative step size not permitted here: [%s]%s\n", current_translation, which_key);
	  return 1;
	}
	first_stroke = find_pbs(tr, k, chan, dir>0, step);
	if (is_anyshift) {
	  alt_press_stroke = find_pbs(tr, 0, chan, dir>0, step);
	}
	if (!dir) {
	  is_bidirectional = 1;
	  release_first_stroke = find_pbs(tr, k, chan, 1, step);
	  if (is_anyshift) {
	    alt_release_stroke = find_pbs(tr, 0, chan, 1, step);
	  }
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
      (is_bidirectional && *release_first_stroke != NULL) ||
      (is_anyshift &&
       (*alt_press_stroke != NULL ||
	(is_bidirectional && *alt_release_stroke != NULL)))) {
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
	if (!s->keysym && !s->shift && s->dirty) {
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
      } else if (s->shift) {
	append_shift();
      } else {
	append_midi(s->status, s->data, s->step, s->incr);
      }
      s = s->next;
    }
  }
  if (all_keys && is_anyshift) {
    // create a duplicate for any-shift translations (?)
    stroke *s = *press_first_stroke;
    first_stroke = alt_press_stroke;
    while (s) {
      if (s->keysym) {
	append_stroke(s->keysym, s->press);
      } else if (s->shift) {
	append_shift();
      } else {
	append_midi(s->status, s->data, s->step, s->incr);
      }
      s = s->next;
    }
    if (is_keystroke || is_bidirectional) {
      s = *release_first_stroke;
      first_stroke = alt_release_stroke;
      while (s) {
	if (s->keysym) {
	  append_stroke(s->keysym, s->press);
	} else if (s->shift) {
	  append_shift();
	} else {
	  append_midi(s->status, s->data, s->step, s->incr);
	}
	s = s->next;
      }
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
  if (parse_midi(tok, buf, 0, mode, &status, &data, &step, &incr, &dir)) {
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
    if (default_debug_regex || default_debug_strokes || default_debug_keys ||
	default_debug_midi) {
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
    reload_callback();
    debug_regex = default_debug_regex;
    debug_strokes = default_debug_strokes;
    debug_keys = default_debug_keys;
    debug_midi = default_debug_midi;
    midi_octave = 0;

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
	finish_translation_section(tr);
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
      if (!strcmp(tok, "DEBUG_MIDI")) {
	debug_midi = 1;
	continue;
      }
      if (!strcmp(tok, "JACK_NAME")) {
	char *a = token(NULL, &delim);
	if (!jack_client_name) {
	  static char buf[100];
	  strncpy(buf, a, 100); buf[99] = 0; // just in case...
	  jack_client_name = buf;
	}
	continue;
      }
      if (!strcmp(tok, "JACK_PORTS")) {
	char *a = token(NULL, &delim);
	int k, n;
	if (!jack_num_outputs) {
	  if (sscanf(a, "%d%n", &k, &n) == 1 && !a[n] && k>=1 && k<=2) {
	    jack_num_outputs = k;
	  } else {
	    fprintf(stderr, "invalid port number: %s, must be 1 or 2\n", a);
	  }
	}
	continue;
      }
      if (!strncmp(tok, "MIDI_OCTAVE", 11)) {
	char *a = tok+11;
	int k, n;
	if (!*a)
	  // look for the offset in the next token
	  a = token(NULL, &delim);
	if (sscanf(a, "%d%n", &k, &n) == 1 && !a[n]) {
	  midi_octave = k;
	} else {
	  fprintf(stderr, "invalid octave offset: %s\n", a);
	}
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
	  if (!strcmp(tok, "RELEASE"))
	    add_keystroke(tok, PRESS_RELEASE);
	  else if (!strcmp(tok, "SHIFT"))
	    append_shift();
	  else if (strncmp(tok, "XK", 2))
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
    finish_translation_section(tr);

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
    if (!tr->is_default) {
      // AG: We first try to match the class name, since it usually provides
      // better identification clues.
      if (win_class && *win_class &&
	  regexec(&tr->regex, win_class, 0, NULL, 0) == 0) {
	return tr;
      }
      if (win_title && *win_title &&
	  regexec(&tr->regex, win_title, 0, NULL, 0) == 0) {
	return tr;
      }
    }
    tr = tr->next;
  }
  return NULL;
}
