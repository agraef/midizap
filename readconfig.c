
/*

  Copyright 2013 Eric Messick (FixedImagePhoto.com/Contact)

  Copyright 2018 Albert Graef <aggraef@gmail.com>, various improvements

  Read and process the configuration file ~/.shuttlepro

  Lines starting with # are comments.

  Sequence of sections defining translation classes, each section is:

  [name] regex
  K<1..15> output
  S<-7..7> output
  I<LR> output
  J<LR> output

  When focus is on a window whose class or title matches regex, the
  following translation class is in effect.  An empty regex for the last
  class will always match, allowing default translations.  Any output
  sequences not bound in a matched section will be loaded from the
  default section if they are bound there.

  Each "[name] regex" line introduces the list of key and shuttle
  translations for the named translation class.  The name is only used
  for debugging output, and needn't be unique.  The following lines
  with K, S, I and J labels indicate what output should be produced for
  the given keypress, shuttle position, shuttle direction, or jog direction.

  output is a sequence of one or more key codes with optional up/down
  indicators, or strings of printable characters enclosed in double
  quotes, separated by whitespace.  Sequences bound to keys may have
  separate press and release sequences, separated by the word RELEASE.

  Examples:

  K1 "qwer"
  K2 XK_Right
  K3 XK_Alt_L/D XK_Right
  K4 "V" XK_Left XK_Page_Up "v"
  K5 XK_Alt_L/D "v" XK_Alt_L/U "x" RELEASE "q"

  Any keycode can be followed by an optional /D, /U, or /H, indicating
  that the key is just going down (without being released), going up,
  or going down and being held until the shuttlepro key is released.

  So, in general, modifier key codes will be followed by /D, and
  precede the keycodes they are intended to modify.  If a sequence
  requires different sets of modifiers for different keycodes, /U can
  be used to release a modifier that was previously pressed with /D.

  At the end of shuttle and jog sequences, all down keys will be
  released.

  Keypresses translate to separate press and release sequences.

  At the end of the press sequence for key sequences, all down keys
  marked by /D will be released, and the last key not marked by /D,
  /U, or /H will remain pressed.  The release sequence will begin by
  releasing the last held key.  If keys are to be pressed as part of
  the release sequence, then any keys marked with /D will be repressed
  before continuing the sequence.  Keycodes marked with /H remain held
  between the press and release sequences.

  JACK MIDI support (added by aggraef@gmail.com Fri Aug 3 11:01:32 CEST 2018):

  The MIDI output option is useful if you want to be able to send control
  messages from a Shuttle device to any kind of MIDI-capable program, such as
  a synthesizer or a DAW. Also, if you put the MIDI translations into a
  special "MIDI" default section of the shuttlerc file, then the target
  application will be able to receive data from the device no matter which
  window has the keyboard focus. (This special "MIDI" section will only be
  active if Jack MIDI support is actually enabled with the -j option, see
  below. This allows you to have another default section after the MIDI
  section for non-MIDI operation.)

  To enable MIDI output, add the -j option when invoking the program (also,
  the -dj option can be used to get verbose output from Jack if needed). This
  causes a Jack client named "shuttlepro" with a single MIDI output port to be
  created, and will also start up Jack if it is not already running. Any MIDI
  messages in the translations will be sent on that port. You can then use any
  Jack patchbay such as qjackctl to connect the output to any other Jack MIDI
  client (use the a2jmidid program to connect to non-Jack ALSA MIDI
  applications).

  Here is the complete list of tokens recognized as MIDI messages, with an
  explanation of how they work. Note that not all MIDI messages are supported
  right now (no aftertouch, no system messages), but that subset should be
  enough to handle most common use cases. (In any case, adding more message
  types should be a piece of cake.) Also note that bindings can involve as
  many MIDI messages as you want, and these can be combined freely with
  keypress events in any order. There's no limitation on the type or number of
  MIDI messages that you can put into a binding (except that program change
  and note messages can only be bound to key inputs).

  CCn: Generates a MIDI control change message for controller number n, where
  n must be in the range 0..127. These can be bound to any kind of input event
  (key, jog, or shuttle). In the case of jog or shuttle, the controller value
  will correspond to the jog/shuttle position, clamped to the 0..127 (single
  data byte) range. For key input, the control change message will be sent
  once with a value of 127 when the key is pressed, and then again with a
  value of 0 when the key is released.

  Example: CC7 generates a MIDI message to change the volume controller
  (controller #7). You can bind this, e.g., to the jog wheel or a key as
  follows:

  JL CC7
  JR CC7
  K5 CC1

  PB: Generates a MIDI pitch bend message. This works pretty much like a MIDI
  control change message, but with an extended range of 0..16383, where 8192
  denotes the center value. Obviously, this message is best bound to the
  shuttle (albeit with a resolution limited to 14 steps), but it also works
  with the jog wheel (with each tick representing 1/128th of the full pitch
  bend range) and even key input (in this case, 8192 is used as the "off"
  value, so the pitch only bends up, never down).

  Example: Just PB generates a pitch bend message. You usually want to bind
  this to the shuttle (in incremental mode), so the corresponding translations
  would normally look like this:

  IL PB
  IR PB

  PCn: This generates a MIDI program change message for the given program
  number n, which must be in the 0..127 range. This type of message only works
  with key input, it will be ignored in jog and shuttle assignments. Also, by
  default the PC message is generated only at the time the key is pressed. To
  have another PC message generated at key release time, it must be put
  explicitly into the RELEASE part of the key binding.

  Example: The following will output a change to program 5 when K5 is pressed,
  and another change to program 0 when the key is released (note that if you
  leaved away the "RELEASE PC0" part, then only the PC5 will be output when
  pressing the key, nothing happens when the key is released):

  K5 PC5 RELEASE PC0

  C0..G10 (MIDI notes): This uses the customary MIDI note names, consisting of
  the letters A..G (denoting the seven white keys in an octave, in either
  upper- or lowercase), optionally followed by b or # (denoting accidentals,
  flat and sharp), and terminated with an octave number (0..10). Middle C is
  denoted C5. Like PC messages, these can only be bound to key inputs; they
  will be ignored when used with jog or shuttle. The note starts (sending a
  note on MIDI message) when pressing the key, and finishes (sending the
  corresponding note off message) when releasing the key.

  Example: The following binds key K6 to a C-7 chord in the middle octave:

  K6 C5 E5 G5 Bb5

  CHk: This doesn't actually output any MIDI message, but merely changes the
  default MIDI channel for all subsequent MIDI messages. k denotes the MIDI
  channel, which must be in the range 1..16. By default (if the CH command
  isn't used), MIDI messages will be sent on MIDI channel 1.

  Example: CH10 C3 outputs the note C3 (MIDI note 36) on MIDI channel 10
  (usually the drum channel). Here's how you can assign keys K5..K9 to play a
  little drumkit:

  K5 CH10 B2
  K6 CH10 C3
  K7 CH10 C#3
  K8 CH10 D3
  K9 CH10 D#3

  Instead of using "CH", you can also specify the MIDI channel of a single
  message directly as a suffix, separating message and channel number with a
  dash, like so:

  K5 B2-10
  K6 C3-10
  K7 C#3-10
  K8 D3-10
  K9 D#3-10

  This is also the format used when printing MIDI messages in translations.
  Note that the MIDI channel suffix only applies to a single message, other
  messages without a suffix will still use the default MIDI channel.

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
	printf("CC%d-%d ", s->data, channel);
	break;
      case 0xc0:
	printf("PC%d-%d ", s->data, channel);
	break;
      case 0xe0:
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
int is_keystroke;
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
  s->status = s->data = s->dirty = 0;
  if (*first_stroke) {
    last_stroke->next = s;
  } else {
    *first_stroke = s;
  }
  last_stroke = s;
}

void
append_midi(int status, int data)
{
  stroke *s = (stroke *)allocate(sizeof(stroke));

  s->next = NULL;
  s->keysym = 0;
  s->press = 0;
  s->status = status;
  s->data = data;
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

   tok  ::= ( note | msg ) [number] [ "-" number] [incr]
   note ::= ( "a" | ... | "g" ) [ "#" | "b" ]
   msg  ::= "ch" | "pb" | "pc" | "cc"
   incr ::= "-" | "+" | "<" | ">"

   Numbers are always in decimal. The meaning of the first number depends on
   the context (octave number for notes, the actual data byte for other
   messages). If present, the suffix with the second number (after the dash)
   denotes the MIDI channel, otherwise the default MIDI channel is used. Note
   that not all combinations are possible -- "pb" is *not* followed by a data
   byte, and "ch" may *not* have a channel number suffix on it. (In fact, "ch"
   is no real MIDI message at all; it just sets the default MIDI channel for
   subsequent messages.) The incr flag is only permitted in the first token of
   a translation, and only in conjunction with "pb" or "cc", whereas "ch" must
   *not* occur as the first token. */

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
parse_midi(char *tok, char *s, int *incr, int *status, int *data)
{
  char *p = tok, *t;
  int n, m = -1, k = midi_channel;
  s[0] = 0;
  while (*p && !isdigit(*p) && !strchr("+-<>", *p)) p++;
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
  if (*p && incr && strchr("+-<>", *p)) {
    // increment flag ("pb" and "cc" only)
    if (strcmp(s, "pb") && strcmp(s, "cc")) return 0;
    if ((*p == '<' || *p == '>') && strcmp(s, "cc")) return 0;
    *incr = (*p == '-')?0:(*p == '+')?1:(*p == '<')?2:3;
    p++;
  } else if (incr) {
    *incr = -1;
  }
  // check for trailing garbage
  if (*p) return 0;
  if (strcmp(s, "ch") == 0) {
    if (incr) return 0;
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
  int status, data, incr;
  char buf[100];

  //printf("start_translation(%s)\n", which_key);

  if (tr == NULL) {
    fprintf(stderr, "need to start translation section before defining key: %s\n", which_key);
    return 1;
  }
  current_translation = tr->name;
  key_name = which_key;
  is_keystroke = is_midi = 0;
  first_release_stroke = 0;
  regular_key_down = 0;
  modifier_count = 0;
  midi_channel = 0;
  if (parse_midi(which_key, buf, &incr, &status, &data)) {
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
      // supposed to be treated as a "down" sequence immediately followed by
      // the corresponding "up" sequence.
      first_stroke = &(tr->pc[chan][data][0]);
      release_first_stroke = &(tr->pc[chan][data][1]);
      is_keystroke = 1;
      break;
    case 0xb0:
      if (incr == -1) {
	// cc on/off
	first_stroke = &(tr->cc[chan][data][0]);
	release_first_stroke = &(tr->cc[chan][data][1]);
	is_keystroke = 1;
      } else {
	// cc (step up, down)
	tr->is_incr = incr/2 != 0;
	first_stroke = &(tr->ccs[chan][data][incr%2]);
      }
      break;
    case 0xe0:
      if (incr == -1) {
	// pb on/off
	first_stroke = &(tr->pb[chan][0]);
	release_first_stroke = &(tr->pb[chan][1]);
	is_keystroke = 1;
      } else {
	// pb (step up, down)
	first_stroke = &(tr->pbs[chan][incr]);
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
  if (*first_stroke != NULL) {
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
	  append_midi(s->status, s->data);
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
  int status, data;
  char buf[100];
  if (parse_midi(tok, buf, NULL, &status, &data)) {
    if (status == 0) {
      // 'ch' token; this doesn't actually generate any output, it just sets
      // the default MIDI channel
      midi_channel = data;
    } else {
      append_midi(status, data);
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

void
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
    if (!config_file_default) perror(config_file_name);
    config_file_name = "/etc/midizaprc";
    config_file_modification_time = 0;
  }
  if (stat(config_file_name, &buf) < 0) {
    perror(config_file_name);
    return;
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
      perror(config_file_name);
      return;
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
