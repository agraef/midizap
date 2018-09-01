% midizap(1)

# Name

midizap -- control your multimedia applications with MIDI

# Synopsis

midizap [-h] [-k] [-o[2]] [-j *name*] [-P[*prio*]] [-r *rcfile*] [-d[rskmj]]

# Options

-h
:   Print a short help message.

-k
:   Keep track of key (on/off) status of MIDI notes and control switches. This isn't generally recommended, but may occasionally be useful to deal with quirky controllers sending note- or control-ons without corresponding off messages.

-o[2]
:   Enable MIDI output. Add "2" for a second pair of MIDI ports to be used, e.g., for controller feedback. See Sections *MIDI Output* and *MIDI Feedback*.

-j *name*
:   Set the Jack client name. Default: "midizap". See Section *Jack-Related Options*.

-P[*prio*]
:   Run with the given real-time priority (90 if not specified). See Section *Jack-Related Options*.

-r *rcfile*
:   Set the configuration file name. Default: taken from the MIDIZAP_CONFIG_FILE environment variable if it exists, or ~/.midizaprc if it exists, /etc/midizaprc otherwise. See Section *Configuration File*.

-d[rskmj]
:   Enable various debugging options: r = regex (print matched translation sections), s = strokes (print the parsed configuration file in a human-readable format), k = keys (print executed translations), m = midi (MIDI monitor, print all recognizable MIDI input), j = jack (additional Jack debugging output). Just `-d` enables all debugging options. See Section *Basic Usage*.

# Description

midizap lets you control your multimedia applications using [MIDI][]. To these ends, it translates Jack MIDI input into X keystrokes, mouse button presses, scroll wheel events, and, as an option, MIDI output. It does this by matching the `WM_CLASS` and `WM_NAME` properties of the window that has the keyboard focus against the regular expressions for each application section in its configuration (midizaprc) file. If a regex matches, the corresponding set of translations is used. If a matching section cannot be found, or if it doesn't define a suitable translation, the program falls back to a set of translations in a default section at the end of the file, if available.

The midizaprc file is just an ordinary text file which you can edit to configure the program. The configuration language is described in detail with lots of examples later in this manual. But the syntax is actually pretty straightforward, basically just a list of MIDI messages (denoted with familiar human-readable mnemonics, no hex numbers!) and their translations, which is divided into sections for different applications. An example.midizaprc file containing sample configurations for some applications is included in the sources to get you started, and you can find some more examples of configuration files for various purposes in the examples directory.

midizap provides you with a way to hook up just about any MIDI controller to your applications. Even if your target application already supports MIDI, midizap's MIDI output option will be useful if your controller can't work directly with the application because of protocol incompatibilities. In particular, you can use midizap to turn pretty much any MIDI controller with enough faders and buttons into a Mackie-compatible mixing device ready to be used with most DAW (digital audio workstation) programs. Another common use case is video editing software, which rarely offers built-in MIDI support. midizap allows you to map the faders, encoders and buttons of your MIDI controller to keyboard commands of your video editor for cutting, marking, playback, scrolling, zooming, etc.

In other words, as long as the target application can be controlled with simple keyboard shortcuts and/or MIDI commands, chances are that midizap can make it work (at least to some extent) with your controller.

# Installation

First, make sure that you have the required dependencies installed. The program needs a few X11 libraries and Jack. And of course you need GNU make and gcc (the GNU C compiler). On Ubuntu and other Debian-based systems you should be able to get everything that's needed by running this command:

    sudo apt install build-essential libx11-dev libxtst-dev libjack-dev

Then just run `make` and `sudo make install`. This installs the example.midizaprc file as /etc/midizaprc, and the midizap program and the manual page in the default install location. Usually this will be under /usr/local, but the installation prefix can be changed with the `prefix` variable in the Makefile. Also, package maintainers can use the `DESTDIR` variable as usual to install into a staging directory for packaging purposes.

# Configuration File

After installation the system-wide default configuration file will be in /etc/midizaprc, where the program will be able to find it. We recommend copying this file to your home directory, renaming it to .midizaprc:

    cp /etc/midizaprc ~/.midizaprc

The ~/.midizaprc file, if it exists, takes priority over /etc/midizaprc, so it becomes your personal default midizap configuration. The midizaprc file included in the distribution is really just an example; you're expected to edit this file to adjust the bindings for the MIDI controllers and the applications that you use.

It is also possible to specify the configuration file to be used, by invoking midizap with the `-r` option on the command line, e.g.: `midizap -r myconfig.midizaprc`. This is often used with more specialized configurations dealing with specific applications or MIDI controllers.

The program automatically reloads the midizaprc file whenever it notices that the file has been changed. Thus you can edit the file while the program keeps running, and have the changes take effect immediately without having to restart the program. When working on new translations, you may want to run the program in a terminal, and employ some or all of the debugging options explained below to see exactly how your translations are being processed.

# Basic Usage

The midizap program is a command line application, so you typically run it from the terminal. However, it is also possible to launch it from your Jack session manager (see *Jack-Related Options* below) or from your desktop environment's startup files once you've set up everything to your liking.

Try `midizap -h` for a brief summary of the available options with which the program can be invoked.

midizap uses [Jack][] for doing all its MIDI input and output, so you need to be able to run Jack and connect the Jack MIDI inputs and outputs of the program. While it's possible to do all of that from the command line as well, we recommend using a Jack front-end and patchbay program like [QjackCtl][] to manage Jack and to set up the MIDI connections. In QjackCtl's setup, make sure that you have selected `seq` as the MIDI driver. This exposes the ALSA sequencer ports of your MIDI hardware and other non-Jack ALSA MIDI applications as Jack MIDI ports, so that they can easily be connected to midizap. (We're assuming that you're using Jack1 here. Jack2 works in a very similar way, but may require some more fiddling; in particular, you may have to use [a2jmidid][] as a separate ALSA-Jack MIDI bridge in order to have the ALSA MIDI devices show properly as Jack MIDI devices.)

Having that set up, start Jack, make sure that your MIDI controller is connected, and try running `midizap` from the command line (without any arguments). In QjackCtl, open the Connections dialog and activate the second tab named "MIDI", which shows all available Jack MIDI inputs and outputs. On the right side of the MIDI tab, you should now see a client named `midizap` with one MIDI input port named `midi_in`. That's the one you need to connect to your MIDI controller, whose output port should be visible under the `alsa_midi` client on the left side of the dialog.

To test the waters, you can hook up just about any MIDI keyboard and give it a try with the default section in the distributed midizaprc file, which contains some basic translations for mouse and cursor key emulation. Here is the relevant excerpt from that section:

~~~
[Default]

 C5    XK_Button_1
 D5    XK_Button_2
 E5    XK_Button_3

 F5    XK_Left
 G5    XK_Up
 A5    XK_Down
 B5    XK_Right

 CC1+  XK_Scroll_Up
 CC1-  XK_Scroll_Down
~~~

We refer to Section *Translation Syntax* below for a discussion of the syntax being used here, but it should be fairly obvious that these translations map the white keys of the middle octave (MIDI notes `C5` thru `B5`) to some mouse buttons and cursor commands. Switch the keyboard focus to some window with text in it, such as a terminal or an editor window. Pressing the keys C, D and E should click the mouse buttons, while F thru B should perform various cursor movements. Also, moving the modulation wheel (`CC1`) on your keyboard should scroll the window contents up and down.

One useful feature is that you can invoke the program with various debugging options to get more verbose output as the program recognizes events from the device and translates them to corresponding mouse actions or key presses. E.g., try running `midizap -drk` to have the program print the recognized configuration sections and translations as they are executed. Now press some of the keys and move the modulation wheel. You should see something like:

~~~
$ midizap -drk
Loading configuration: /home/user/.midizaprc
translation: Default for emacs@hostname (class emacs)
CC1-1-: XK_Scroll_Down/D XK_Scroll_Down/U 
CC1-1-: XK_Scroll_Down/D XK_Scroll_Down/U 
G5-1[D]: XK_Up/D 
G5-1[U]: XK_Up/U 
A5-1[D]: XK_Down/D 
A5-1[U]: XK_Down/U 
~~~

It goes without saying that these debugging options will be very helpful when you start developing your own bindings. The `-d` option can be combined with various option characters to choose exactly which kinds of debugging output you want; `r` ("regex") prints the matched translation section (if any) along with the window name and class of the focused window; `s` ("strokes") prints the parsed contents of the configuration file in a human-readable form whenever the file is loaded; `k` ("keys") shows the recognized translations as the program executes them, in the same format as `s`; `m` ("MIDI") prints *any* MIDI input, so that you can figure out which MIDI tokens to use for configuring the translations for your controller; and `j` adds some debugging output from the Jack driver. You can also just use `-d` to enable all debugging output. (Most of these options are also available as directives in the midizaprc file; please check the distributed example.midizaprc for details.)

Have a look at the distributed midizaprc file for more examples. Most of the other translations in the file assume a Mackie-like device with standard playback controls and a jog wheel. Any standard DAW controller which can be switched into Mackie mode should work with these out of the box. There are also some more generic examples, like the one above, which will work with almost any kind of MIDI keyboard. The examples are mostly for illustrative and testing purposes, though, to help you get started. You will want to edit them and add translations for your own controllers and favorite applications.

# MIDI Output

As already mentioned, the midizap program can also be made to function as a MIDI mapper which translates MIDI input to MIDI output. MIDI output is enabled by running the program as `midizap -o`. This equips the Jack client with an additional MIDI output port named `midi_out` (visible on the left side of QjackCtl's Connection window). 

The example.midizaprc file comes with a sample configuration in the special `[MIDI]` default section for illustration purposes. This section is only active if the program is run with the `-o` option. It allows MIDI output to be sent to any connected applications, no matter which window currently has the keyboard focus. This is probably the most common way to use this feature, but of course it is also possible to have application-specific MIDI translations, in the same way as with X11 key bindings. In fact, you can freely mix mouse actions, key presses and MIDI messages in all translations.

You can try it and test that it works by running `midizap -o` along with a MIDI synthesizer such as [FluidSynth][] or its graphical front-end [Qsynth][]. Use QjackCtl to connect FluidSynth's MIDI input to midizap's output port. In the sample configuration, the notes `C4` thru `F4` in the small octave have been set up so that you can use them to operate a little drumkit, and a binding for the volume controller (`CC7`) has been added as well. The relevant portion from the configuration entry looks as follows:

~~~
[MIDI]

 C4    C3-10
 D4    C#3-10
 E4    D3-10
 F4    D#3-10

 CC7=  CC7-10
~~~

Note the `-10` suffix on the output messages in the above example, which indicates that output goes to MIDI channel 10. In midizaprc syntax, MIDI channels are 1-based, so they are numbered 1..16, and 10 denotes the GM (General MIDI) drum channel.

E.g., the input note `C4` is mapped to `C3-10`, the note C in the third MIDI octave, which on channel 10 will produce the sound of a bass drum, at least on GM compatible synthesizers like Fluidsynth. The binding for the volume controller (`CC7`) at the end of the entry sends volume changes to the same drum channel (`CC7-10`), so that you can use the volume control on your keyboard to change the volume on the drum channel. The program keeps track of the values of both input and output controllers on all MIDI channels internally, so with the translations above all that happens automagically.

Besides MIDI notes and control change (`CC`) messages, the midizap program also recognizes key and channel pressure (`KP`, `CP`), program change (`PC`), and pitch bend (`PB`) messages, which should cover most common use cases. These are discussed in more detail in the *Translation Syntax* section below.

# Jack-Related Options

There are some additional directives (and corresponding command line options) to set midizap's Jack client name and the number of input and output ports it uses. (If both the command line options and directives in the midizaprc file are used, the former take priority, so that it's always possible to override the options in the midizaprc file from the command line.)

Firstly, there's the `-j` option and the `JACK_NAME` directive which change the Jack client name from the default (`midizap`) to whatever you want it to be. To use this option, simply invoke midizap as `midizap -j client-name`, or put the following directive into your midizaprc file:

~~~
JACK_NAME "client-name"
~~~

This option is useful, in particular, if you're running multiple instances of midizap with different configurations for different controllers and/or target applications, and you want to have the corresponding Jack clients named appropriately, so that they can be identified more easily when wiring them up. If you're using a persistent MIDI patchbay, such as the one available in QjackCtl, you can then have the right connections automatically set up for you whenever you launch midizap with that specific configuration.

Secondly, we've already seen the `-o` option which is used to equip the Jack client with an additional output port. This can also be achieved with the `JACK_PORTS` directive in the midizaprc file, as follows:

~~~
JACK_PORTS 1
~~~

You may want to place this directive directly into a configuration file if the configuration is primarily aimed at doing MIDI translations, so you'd like to have the MIDI output enabled by default. Typically, such configurations will include just a default `[MIDI]` section and little else. As explained in the *MIDI Feedback* section, it's also possible to have *two* pairs of input and output ports, in order to deal with controller feedback from the application. This is achieved by either invoking midizap with the `-o2` option, or by employing the `JACK_PORTS 2` directive in the configuration file.

**NOTE:** If you notice bad latency or jitter in MIDI output, you should try running midizap with real-time priorities. Jack itself usually does that anyway, but midizap's main thread won't unless you run it with the `-P` option (`midizap -P`, or `midizap -P80` if you also want to specify the priority; the default is 90). Using this option, midizap should be able to get down to MIDI latencies in the 1 msec ballpark which should be good enough for most purposes (YMMV, though).

Last but not least, midizap also supports *Jack session management*, which makes it possible to record the options the program was invoked with, along with all the MIDI connections. This feature can be used with any Jack session management software. QjackCtl has its own built-in Jack session manager which is available in its Session dialog. To use this, launch midizap and any other Jack applications you want to have in the session, use QjackCtl to set up all the connections as needed, and then hit the "Save" button in the Session dialog to have the session recorded. (The "Save and Quit" option does the same, but also asks midizap to quit afterwards.) Now, at any later time you can rerun the recorded session with the "Load" button in the same dialog. Also, your most frequently used sessions are available in the "Recent" menu from where they can be launched quickly.

# Translation Syntax

The midizap configuration file consists of sections defining translation classes. Each section generally looks like this, specifying the name of a translation class, optionally a regular expression to be matched against the window class or title, and a list of translations:

~~~
[name] regex
<A..G><#b><0..12> output  # note
KP:<note> output          # key pressure (aftertouch)
PC<0..127> output         # program change
CC<0..127> output         # control change
CP output                 # channel pressure
PB output                 # pitch bend
~~~

The `#` character at the beginning of a line and after whitespace is special; it indicates that the rest of the line is a comment, which is skipped by the parser. Empty lines and lines containing nothing but whitespace are also ignored.

Each `[name] regex` line introduces the list of MIDI message translations for the named translation class. E.g., the following header might be used to begin a new section for the Kdenlive video editor: 

~~~
[Kdenlive] ^kdenlive$
~~~

Please refer to regex(7) for an explanation of the regular expression syntax. The regex above will match `kdenlive` and nothing else, because we tied the match to beginning and end with the `^` and `$` anchors.

When focus is on a window whose class or title matches the (basic) regular expression `regex`, the corresponding translations are in effect. An empty regex for the last class will always match, allowing default translations. Any output sequences not bound in a matched section will be loaded from the default section if they are bound there. In addition, there are two special default sections labeled `[MIDI]` and `[MIDI2]` which are used specifically for MIDI translations, please see the *MIDI Output* and *MIDI Feedback* sections for details. If these sections are present, they should precede the main default section. All other sections, including the main default section, can be named any way you like; the name is only used for debugging output and diagnostics, and needn't be unique.

The translations define what output should be produced for the given MIDI input. Each translation must be on a line by itself. The left-hand side (first token) of the translation denotes the MIDI message to be translated. The corresponding right-hand side (the rest of the line) is a sequence of one or more tokens, separated by whitespace, indicating either MIDI messages or X11 keyboard and mouse events to be output.

**NOTE:** Translations *must be determined uniquely* in each translation class, i.e., each input message may be bound to at most one output sequence in each section. Otherwise, the parser will print an error message, and the extra translations will be ignored. While the uniqueness requirement may sometimes be a bit inconvenient, it is actually an important feature, because it ensures that midizap's operation is completely *deterministic*. That is, for the same input sequence on the same window the program will always generate exactly the same output sequence.

## MIDI Message Notation
 
Note messages are specified using a human-readable format which should look familiar (to musicians at least): a note name `A..G` is followed by an (optional) accidental (`#` or `b`), and a (mandatory) MIDI octave number. Note that all MIDI octaves start at the note C, so `B0` comes before `C1`. By default, `C5` denotes middle C (you can change this if you want, see *Octave Numbering* below). Enharmonic spellings are equivalent, so, e.g., `D#` and `Eb` denote exactly the same MIDI note.

The other messages are denoted using short mnemonics: `KP` (aftertouch a.k.a. key pressure; followed by `:` and a note designation); `CC` (control change, followed by a controller number); `PC` (program change, followed by a program number); `CP` and `PB` (channel pressure and pitch bend; these don't have a numeric suffix, as they apply to the entire MIDI channel). We will go into the other syntactic bits and pieces of MIDI message designations later, but it's good to have the following grammar in EBNF notation handy for reference. (To keep things simple, the grammar is somewhat abridged, but it covers all the frequently used notation. There is some additional syntax for special forms of translations which will be introduced later.)

~~~
token ::= msg [ "[" number "]" ] [ "-" number ] [ flag ]
msg   ::= ( note | other ) [ number ]
note  ::= ( "A" | ... | "G" ) [ "#" | "b" ]
other ::= "CH" | "PB" | "PC" | "CC" | "CP" | "KP:" note
flag  ::= "-" | "+" | "=" | "<" | ">" | "~"
~~~

Case is ignored here, so `CC`, `cc` or even `Cc` are considered to be exactly the same token by the parser, although by convention we usually write them in uppercase. Numbers are always integers in decimal. The meaning of the `msg` number depends on the context (octave number for notes and key pressure, controller or program number in the range 0..127 for other messages, MIDI channel number in the range 1..16 for the special `CH` token). This can optionally be followed by a number in brackets, denoting a nonzero step size. Also optionally, a suffix with a third number (after the dash) denotes the MIDI channel in the range 1..16; otherwise the default MIDI channel is used (which is always 1 on the left-hand side, but can be set on the right-hand side with the `CH` token). The optional "increment" flag at the end of a token indicates a "data" translation which responds to incremental (up/down) value changes rather than key presses, cf. *Key and Data Input* below.

## Octave Numbering

A note on the octave numbers in MIDI note designations is in order here. There are various different standards for numbering octaves, and different programs use different standards, which can be rather confusing. E.g., there's the ASA (Acoustical Society of America) standard where middle C is C4, also known as "scientific" or "American standard" pitch notation. At least two other standards exist specifically for MIDI octave numbering, one in which middle C is C3 (so the lowest MIDI octave starts at C-2), and zero-based octave numbers, which start at C0 and have middle C at C5. There's not really a single "best" standard here, but the latter tends to appeal to mathematically inclined and computer-savvy people, and is also what is used by default in the midizaprc file.

However, you may want to change this, e.g., if you're working with documentation or MIDI monitoring software which uses a different numbering scheme. To do this, just specify the desired offset for the lowest MIDI octave with the special `MIDI_OCTAVE` directive in the configuration file. For instance:

~~~
MIDI_OCTAVE -1 # ASA pitches (middle C is C4)
~~~

Note that this transposes *all* existing notes in translations following the directive, so if you add this option to an existing configuration, you probably have to edit the note messages in it accordingly.

## Key and Data Input

Translations come in two flavors, "key translations" and "data translations", which differ in the way the extra data payload of the input message, called the *parameter value* (or just *value* for short), is processed. The parameter value depends on the type of MIDI message. Program changes (`PC`) have no value at all. For notes, as well as key and channel pressure messages (`CP`, `KP`), it is a velocity value; for control changes (`CC`), a controller value; and for pitch bend messages (`PB`), a pitch bend value. The latter is actually a 14 bit value composed of the two data bytes in the message, which is considered as a signed quantity in the range -8192..8191, where 0 denotes the center value. In all other cases, the parameter value consists of a single data byte, which denotes an unsigned 7 bit quantity in the range 0..127.

Note that since translations are determined uniquely in each translation class, you can't have both a key *and* a data translation for the same input in the same section; it's either one or the other.

*Key mode* is the default mode and is available for all message types. In this mode, MIDI messages are considered as keys which can be "pressed" ("on") or "released" ("off"). Any nonzero data value means "pressed", zero "released". Two special cases need to be considered here:

- For pitch bends, any positive *or* negative value means "pressed", while 0 (the center value) means "released".

- Since program changes have no parameter value associated with them, they don't really have an "on" or "off" status. But they are treated in the same key-like fashion anyway, assuming that they are "pressed" and then "released" immediately afterwards.

*Data mode* is available for all messages with a parameter value, i.e., anything but `PC`. In this mode, the actual value of the message is processed rather than just the on/off state. Data mode is indicated with a special suffix on the message token which denotes a step size and/or the direction of the value change which the rule should apply to: increment (`+`), decrement (`-`), or both (`=`). The two parts are both optional, but at least one of them must be present (otherwise the rule is interpreted as a key translation).

In the following, we concentrate on "standard" data mode messages having an increment suffix. In this case, the optional step size in brackets indicates the amount of change required to trigger the translation, so its effect is to downscale the amount of change in the input value. (The variant without an increment suffix is more complicated and mostly intended for rather specialized uses, so we'll have a look at it later in the *Advanced Features* section.)

Data mode usually tracks changes in the *absolute* value of a control. However, for `CC` messages there's also an alternative mode for so-called *incremental* controllers, or *encoders* for short, which can found on some DAW controllers. These usually take the form of jog wheels or rotary encoders which can be turned endlessly in either direction. In contrast, absolute-valued controllers are usually faders or knobs which are confined to a range between minimum and maximum values. Encoders emit a special *sign bit* value indicating a *relative* change, where a value < 64 usually denotes an increment (representing clockwise rotation), and a value > 64 a decrement (counter-clockwise rotation). The actual amount of change is in the lower 6 bits of the value. In the message syntax, these kinds of controls are indicated by using the suffixes `<`, `>` and `~` in lieu of `-`, `+` and `=`, respectively. These flags are only permitted with `CC` messages.

## Keyboard and Mouse Translations

Keyboard and mouse output consists of X key codes with optional up/down indicators, or strings of printable characters enclosed in double quotes. The syntax of these items, as well as the special `RELEASE` and `SHIFT` tokens which will be discussed later, are described by the following grammar:

~~~
token   ::= "RELEASE" | "SHIFT" | keycode [ "/" keyflag ] | string
keycode ::= "XK_Button_1" | "XK_Button_2" | "XK_Button_3" |
            "XK_Scroll_Up" | "XK_Scroll_Down" |
            "XK_..." (X keysyms, see /usr/include/X11/keysymdef.h)
keyflag ::= "U" | "D" | "H"
string  ::= '"' { character } '"'
~~~

Here, case *is* significant (except in character strings, see the remarks below), so the special `RELEASE` and `SHIFT` tokens must be in all caps, and the `XK` symbols need to be written in mixed case exactly as they appear in the /usr/include/X11/keysymdef.h file. Besides the key codes from the keysymdef.h file, there are also some special additional key codes to denote mouse button and scroll wheel events (`XK_Button_1`, `XK_Scroll_Up`, etc.).

Any keycode can be followed by an optional `/D`, `/U`, or `/H` flag, indicating that the key is just going down (without being released), going up, or going down and being held until the "off" event is received. So, in general, modifier key codes will be followed by `/D`, and precede the keycodes they are intended to modify. If a sequence requires different sets of modifiers for different keycodes, `/U` can be used to release a modifier that was previously pressed with `/D`. Sequences may also have separate press and release sequences, separated by the special word `RELEASE`. Examples:

~~~
C5 "qwer"
D5 XK_Right
E5 XK_Alt_L/D XK_Right
F5 "V" XK_Left XK_Page_Up "v"
G5 XK_Alt_L/D "v" XK_Alt_L/U "x" RELEASE "q"
~~~

Translations are handled differently depending on the input mode (cf. *Key and Data Input* above). In *key mode*, there are separate press and release sequences. The former is invoked when the input key goes "down" (i.e., when the "on" status is received), the latter when the input key goes "up" again ("off" status). More precisely, at the end of the press sequence, all down keys marked by `/D` will be released, and the last key not marked by `/D`, `/U`, or `/H` will remain pressed. The release sequence will begin by releasing the last held key. If keys are to be pressed as part of the release sequence, then any keys marked with `/D` will be repressed before continuing the sequence. Keycodes marked with `/H` remain held between the press and release sequences. For instance, let's take a look at one of the more conspicuous translations in the example above:

~~~
G5 XK_Alt_L/D "v" XK_Alt_L/U "x" RELEASE "q"
~~~

This translation has the following meaning: When the `G5` key is pressed on the MIDI keyboard, the key sequence `Alt+v x` is initiated, keeping the `x` key pressed (so it may start auto-repeating after a while). The program then sits there waiting (possibly executing other translations) until you release the `G5` key again, at which point the `x` key is released and the `q` key is pressed (and released).

One pitfall here is that character strings in double quotes are just a shorthand for the corresponding X key codes, ignoring case. Thus, e.g., `"abc"` actually denotes the keysym sequence `XK_a XK_b XK_c`, as does `"ABC"`. So in either case the *lowercase* string `abc` will be output. To output uppercase letters, it is always necessary to add one of the shift modifiers to the output sequence. E.g., `XK_Shift_L/D "abc"` will output `ABC` in uppercase.

In *data mode* only a single sequence is output whenever the message value increases or decreases. At the end of the sequence, all down keys will be released. For instance, the following translations move the cursor left or right whenever the volume controller (`CC7`) decreases and increases, respectively. Also, the number of times one of these keys is output corresponds to the actual change in the value. Thus, if in the example `CC7` increases by 4, say, the program will press (and release) `XK_Right` four times, moving the cursor 4 positions to the right.

~~~
CC7- XK_Left
CC7+ XK_Right
~~~

Incremental `CC` messages are treated in an analogous fashion, but in this case the increment or decrement is determined directly by the input message. One example for this type of controller is the big jog wheel (`CC60`) on some Mackie-like devices, which can be processed as follows (using `<` and `>` in lieu of `-` and `+` as the increment flag of the `CC` message):

~~~
CC60< XK_Left
CC60> XK_Right
~~~

The corresponding "bidirectional" translations, which are indicated with the `=` and `~` flags, are rarely used with keyboard and mouse translations. Same goes for the special `SHIFT` token. Thus we'll discuss these in later sections, see *MIDI Translations* and *Shift State* below.

In data mode, input messages can also have a *step size* associated with them, which has the effect of downscaling changes in the parameter value. The default step size is 1 (no scaling). To change it, the desired step size is written in brackets immediately after the message token and before the increment flag. A step size *k* indicates that the translation is executed whenever the input value has changed by *k* units. For instance, to slow down the cursor movement in the example above by a factor of 4:

~~~
CC7[4]- XK_Left
CC7[4]+ XK_Right
~~~

The same goes for incremental `CC` messages:

~~~
CC60[4]< XK_Left
CC60[4]> XK_Right
~~~

Note that since there's no persistent absolute controller state in this case, this simply scales down the actual increment value in the message itself.

## MIDI Translations

Most of the notation for MIDI messages on the left-hand side of a translation rule also carry over to the output side. The only real difference is that the increment flags `+-=<>` aren't permitted here, as they are only used to determine the input mode (key or data) of the entire translation. The `~` flag *is* allowed, however, to indicate output in bit-sign encoder format in data translations, see below. Step sizes are permitted as well on the output side, in *both* key and data translations. Their meaning depends on the kind of translation, however. In key translations, they denote the (nonzero) value to be used for the "on" state in the press sequence; in data translations, they indicate the amount of change for each unit input change (which has the effect of *upscaling* the value change).

The output sequence can involve as many MIDI messages as you want, and these can be combined freely with keyboard and mouse events in any order. However, as already discussed in Section *MIDI Output* above, you also need to invoke the midizap program with the `-o` option to make MIDI output work. Otherwise, MIDI messages in the output translations will just be silently ignored.

There is one special MIDI token `CH` which can only be used on the output side. It is always followed by a MIDI channel number in the range 1..16. This token doesn't actually generate any MIDI message, but merely sets the default MIDI channel for subsequent MIDI messages in the same output sequence, which is convenient if multiple messages are output to the same MIDI channel. For instance, the sequence `C5-2 E5-2 G5-2`, which outputs a C major chord on MIDI channel 2, can also be abbreviated as `CH2 C5 E5 G5`.

For key mode inputs, the corresponding "on" or "off" event is generated for all MIDI messages in the output sequence, where the "on" value defaults to the maximum value (127 for controller values, 8191 for pitch bends). Thus, e.g., the following rule outputs a `CC64` message with controller value 127 each time `C3` is pressed (and another `CC64` message with value 0 when the note is released again):

~~~
C3 CC64 # hold pedal on/off
~~~

The value for the "on" state can also be denoted explicitly with a step size:

~~~
C3 CC64[64] # hold pedal on/off
~~~

For pitch bends, the step size can also be negative. For instance, the following rules assign two keys to bend down and up by the maximum amount possible:

~~~
C2 PB[-8192] # bend down
D2 PB[8191]  # bend up
~~~

Rules similar to the ones above may be useful if your MIDI keyboard doesn't have a hold pedal or pitch bend wheel, as they let you set aside some keys to emulate those functions.

Let's now have a look at data mode. There are two additional flags `=` and `~` for data translations which are most useful with MIDI translations, which is why we deferred their discussion until now. In pure MIDI translations not involving any key or mouse output, the increment and decrement sequences are usually identical, in which case the `=` suffix can be used to indicate that the output is to be used for *both* increments and decrements. For instance, to map the modulation wheel (`CC1`) to the volume controller (`CC7`):

~~~
CC1= CC7
~~~

Which is more convenient to write, but is really just a shorthand for the two separate translations:

~~~
CC1+ CC7
CC1- CC7
~~~

The same goes for `<`, `>` and `~` with sign-bit encoders:

~~~
CC60~ CC7
~~~

Which is equivalent to:

~~~
CC60< CC7
CC60> CC7
~~~

The `~` flag can be used to denote encoders in output messages, too. E.g., to translate a standard (absolute) MIDI controller to an incremental encoder value, you might use a rule like:

~~~
CC48= CC16~
~~~

Step sizes work on the right-hand side of data translations as well. You might use these to scale up value changes, e.g., when translating from control changes to pitch bends:

~~~
CC1= PB[128]
~~~

The step size can also be negative, which allows you to reverse the direction of a controller if needed. E.g., the following will output values going down from 127 to 0 as input values go up from 0 to 127:

~~~
CC1= CC1[-1]
~~~

Another possibility is to place step sizes on *both* the left-hand and right-hand side of a rule, in order to approximate a rational scaling factor:

~~~
CC1[3]= CC1[2]
~~~

The above translation will only be triggered when the input value changes by 3 units, and the change in the output value will then be doubled again, so that the net effect is to scale the amount of change by 2/3. Note that this will only work smoothly enough if the input and output step sizes are reasonably small, otherwise you'll get a fairly rough approximation.

## Shift State

The special `SHIFT` token toggles an internal shift state, which can be used to generate alternative output for certain MIDI messages. Please note that, like the `CH` token, the `SHIFT` token doesn't generate any output by itself; it merely toggles the internal shift bit which can then be queried in other translations to distinguish between shifted and unshifted bindings for the same input message.

To these ends, there are two additional prefixes which indicate the shift status in which a translation is active. Unprefixed translations are active only in unshifted state. The `^` prefix denotes a translation which is active only in shifted state, while the `?` prefix indicates a translation which is active in *both* shifted and unshifted state. Many DAW controllers have some designated shift keys which can be used for this purpose, but the following will actually work with any key-style MIDI message. E.g., to bind the shift key on an AKAI APCmini controller (`D8`):

~~~
?D8 SHIFT
~~~

Note the `?` prefix indicating that this translation is active in both unshifted and shifted state, so it is used to turn shift state both on and off, giving a "Caps Lock"-style of toggle key. If you'd rather have an ordinary shift key which turns on shift state when pressed and immediately turns it off when released again, you can do that as follows:

~~~
?D8 SHIFT RELEASE SHIFT
~~~

Having set up the translation for the shift key itself, we can now indicate that a translation should be valid only in shifted state with the `^` prefix. This makes it possible to assign, depending on the shift state, different functions to buttons and faders. Here's a typical example which maps a control change to either Mackie-style fader values encoded as pitch bends, or incremental encoder values:

~~~
 CC48= PB[128]  # translate to pitch bend when unshifted
^CC48= CC16~    # translate to encoder when shifted
~~~

To keep things simple, only one shift status is available in the present implementation. Also note that when using a shift key in the manner described above, its status is *only* available internally to the midizap program; the host application never gets to see it. If your host software does its own handling of shift keys (as most Mackie-compatible DAWs do), it's usually more convenient to simply pass those keys on to the application. However, `SHIFT` comes in handy if your controller simply doesn't have enough buttons and faders to control all the essential features of your target application. In this case the internal shift feature makes it possible to double the amount of controls available on the device. For instance, you can emulate a Mackie controller with both encoders and faders on a device which only has a single set of faders, by assigning the shifted faders to the encoders, as shown above.

# Advanced Features

This section covers functionality which is used less often than the basic features discussed in previous sections, but helps solve some of the issues arising in more advanced uses cases. We start out with a brief account on *MIDI feedback*, which is needed to properly implement bidirectional communication with some controllers. This often involves inputs which cannot be handled with the simple kinds of translations we've seen so far, so we introduce a more versatile kind of data translation, called *mod translations*, to deal with these. We also discuss some further uses of mod translations, including a simple macro facility which comes in handy if basic programming capabilities are needed.

## MIDI Feedback

Some MIDI controllers need a more elaborate setup than what we've seen so far, because they have motor faders, LEDs, etc. requiring feedback from the application. To accommodate these, you can use the `-o2` option of midizap, or the `JACK_PORTS 2` directive in the midizaprc file, to create a second pair of MIDI input and output ports, named `midi_in2` and `midi_out2`. Use of this option also activates a second MIDI default section in the midizaprc file, labeled `[MIDI2]`, which is used exclusively for translating MIDI input from the second input port and sending the resulting MIDI output to the second output port. Typically, the translations in the `[MIDI2]` section will be the inverse of those in the `[MIDI]` section, or whatever it takes to translate the MIDI feedback from the application back to MIDI data which the controller understands.

You then wire up midizap's `midi_in` and `midi_out` ports to controller and application as before, but in addition you also connect the application back to midizap's `midi_in2` port, and the `midi_out2` port to the controller. This reverse path is what is needed to translate the feedback from the application and send it back to the controller.

An in-depth discussion of controller feedback is beyond the scope of this manual, but we present a few useful tidbits below. Also, the distribution includes a full-blown example of this kind of setup for your perusal, please check examples/APCmini.midizaprc in the sources. It shows how to emulate a Mackie controller with AKAI's APCmini device, so that it readily works with DAW software such as Ardour.

## Mod Translations

Most of the time, MIDI feedback uses just the standard kinds of MIDI messages readily supported by midizap, such as note messages which make buttons light up in different colors, or control change messages which set the positions of motor faders. However, there are some encodings of feedback messages which combine different bits of information in a single message, making them difficult or even impossible to translate using the simple kinds of rules we've seen so far. midizap offers a special variation of data mode to help decoding such messages. We call them *mod translations*, because they involve calculations with integer moduli which enable you to not only calculate output from input values, but also modify the output messages themselves at the same time.

One important task, which we'll use as a running example below, is the decoding of meter (RMS level) data in the Mackie protocol. There, each meter value is represented as a key pressure message whose value consists of a mixer channel index 0..7 in the "high nibble" (bits 4..6) and the corresponding meter value in the "low nibble" (bits 0..3). Specifically, we will show how to map these values to notes indicating buttons on the AKAI APCmini; please check examples/APCmini.midizaprc in the sources for details about this device. However, mod translations aren't limited to this specific example; similar rules will apply to many other kinds of "scrambled" MIDI data.

In its simplest form, the translation looks as follows:

~~~
CP[16] C0
~~~

In contrast to standard data translations, there's no increment flag here, so the translation does *not* indicate an incremental change of the input value. Instead, the step size on the left-hand side is actually treated as a *modulus*, in order to decompose the input value into two separate quantities, *quotient* and *remainder*. Only the latter becomes the value of the output message, while the former is used as an *offset* to modify the output message. (Note that `CP` and `PB` messages don't have a modifiable offset, so if you use these on the output side of a mod translation, the offset part of the input value will be ignored. `PC` messages, on the other hand, lack the parameter value, so in this case the remainder value will be disregarded instead.)

In order to describe more precisely how this works, let's assume an input value *v* and a modulus *k*. We divide *v* by *k*, yielding the quotient (offset) *q* = *v* div *k* and the remainder (value) *r* =  *v* mod *k*. E.g., with *k* = 16 and *v* = 21, you'll get *q* = 1 and *r* = 5 (21 divided by 16 yields 1 with a remainder of 5). The calculated offset *q* is then applied to the note itself, and the remainder *r* becomes the velocity of that note. So in the example above the output would be the note `C#0` (`C0` offset by 1) with a velocity of 5. On the APCmini, this message will light up the second button in the bottom row of the 8x8 grid in yellow.

This simple kind of transformation is surprisingly versatile, and there are some variations of the syntax which make it even more flexible. The extended MIDI syntax being used here is described by the following grammar rules (please also refer to the beginning of Section *Translation Syntax* for the rest of the MIDI syntax):

~~~
token ::= msg [ steps ] [ "-" number] [ flag ]
steps ::= "{" list "}" | "[" number "]" "{" list "}"
list  ::= number { "," number | ":" number | "-" number }
flag  ::= "'"
~~~

There are two new elements in the syntax, the "transposition" flag `'`, and lists of numbers enclosed in curly braces.

- *Transposition*, denoted with the `'` (apostrophe) suffix on an output message, reverses the roles of *q* and *r*, so that the remainder becomes the offset and the quotient the value of the output message. We won't actually use this in the present example, but it's very convenient in some situations, and we'll get back to it in the following sections.

- *Value lists*, denoted as lists of numbers separated by commas and enclosed in curly braces, provide a way to describe *discrete mappings* of input to output values. To these ends, the remainder value *r* is used as an index into the list to give the corresponding output value, and the last value in the list will be used for any index which runs past the end of the list. There are also some convenient shortcuts which let you construct these lists more easily: repetition *a*`:`*b* (denoting *b* consecutive *a*'s) and enumeration *a*`-`*b* (denoting *a*`,`*a*±1`,`...`,`*b*, which ramps either up or down depending on whether *a*<=*b* or *a*>*b*, respectively).

Let's return to our example. As usual in data translations, you can also specify a step size to upscale the output value *r*:

~~~
CP[16] C0[2]
~~~

But in many cases the required transformations on *r* will be more complicated, so we specify them as value lists instead. E.g., the APCmini uses the velocities 0, 1, 3 and 5 to denote "off" and the colors green, red and yellow, respectively, so you can map the meter values to different colors, e.g., as follows:

~~~
CP[16] C0{0,1,1,1,1,1,1,1,1,5,5,5,3}
~~~

Using the shorthand for repetitions, this can be written more succinctly (which also helps readability):

~~~
CP[16] C0{0,1:8,5:3,3}
~~~

Thus 0 will be mapped to 0 (off), 1..8 to 1 (green), 9..11 to 5 (yellow), and 12 or more to 3 (red).

The quotient here is the mixer channel index in the high-nibble of the `CP` message, which will be used as an offset for the `C0` note on the right, so the above rule shows the meters as a single row of colored buttons at the bottom of the 8x8 grid on the APCmini (first value on `C0`, second value on `C#0`, etc.). To get a different layout, you can also scale the *offset* value, by adding a second step size to the left-hand side:

~~~
CP[16][8] C0{0,1:8,5:3,3}
~~~

With this rule, the buttons are now in the first *column* of the grid (first value on `C0`, second value on `G#0`, etc.). Instead of a single step size, it's also possible to specify a list of discrete offset values, so that you can achieve any regular or irregular output pattern that you want. E.g., the following rule places every other meter value in the second row:

~~~
CP[16]{0,9,2,11,4,13,6,15} C0{0,1:8,5:3,3}
~~~

You might also output several notes at once, in order to display a horizontal or vertical meter *strip* instead of just a single colored button for each mixer channel. For instance:

~~~
CP[16] C0{0,1} G#0{0:8,5} E1{0:11,3}
~~~

Note that each of the output notes will be offset by the same amount, so that the green, yellow and red buttons will always be lined up vertically in this example. (The APCmini.midizaprc example uses a similar, albeit more elaborate translation to handle meter data.)

Another example from the Mackie protocol is time feedback. The following rule (also from the APCmini.midizaprc example) decodes the least significant digit of the beat number in the time display (`CC69`) to count off time on some of the scene launch buttons of the APCmini. Note that the digits are actually encoded in ASCII, hence the copious amount of initial zeros in the value lists below with which we skip over all the non-digit characters at the beginning of the ASCII table.

~~~
CC69[128] F7{0:49,1,0} E7{0:50,1,0} Eb7{0:51,1,0} D7{0:52,1,0}
~~~

Also note the use of the modulus 128, which exceeds the controller value range so that we always have a zero offset here, and thus the output notes aren't modified. (In fact, we only use a mod translation here so that we can apply some value lists to the input value, because these are only available in mod translations.)

## More Uses of Mod Translations

While mod translations are often employed for MIDI feedback, they can also be used as a more capable replacement for "ordinary" (incremental) data translations in various contexts. We discuss some of these use cases below and show how they're implemented.

One common trick that we already encountered in the preceding section is to choose the modulus large enough (> 8192 for `PB`, > 127 for other messages) so that the offset becomes zero and thus inconsequential. We also call these mod translations "trivial". They are useful, in particular, if you want to preserve the parameter value in a data translation such as:

~~~
CC1[128] C5
~~~

This translates the `CC1` (modulation wheel) controller to  a `C5` (middle C) note message in such a way that the controller value becomes the velocity of the note. Note that this is different from both the key translation `CC1 C5` (which only preserves the "on"/"off" status but looses the actual parameter value) and the incremental data translation `CC1= C5` (which executes the translation in a step-wise fashion for each unit change). In contrast, a mod translation always maps messages one-to-one in a single step. While key and incremental data translations are typically used in conjunction with key and mouse output, for pure MIDI bindings like the one above a mod translation is often preferable.

You also need to use a mod translation if your binding involves discrete value lists, because these are not available in other kinds of translations. Value lists can represent *any* discrete mapping from input to output values, and thus offer much more flexibility than simple step sizes. For instance, here's how to map controller values to the first few Fibonacci numbers:

~~~
CC1[128] CC1{0,1,1,2,3,5,8,13,21,34,55,89}
~~~

Value lists offer some conveniences to facilitate their use, *repetitions* (which we've already discussed in the previous section) and *enumerations*. Enumerations are used to denote an ascending or descending range of values. E.g., to reverse the values of a controller you may write:

~~~
CC1[128] CC1{127-0}
~~~

The values in a list may be in any order, and you can throw in any combination of singleton values, enumerations and repetitions. E.g., the list `{0:2-5,7:5-0}` starts with two zeros, then ramps up to 5 followed by five 7s, before finally fading back to 0. That's much easier to read and also much less error-prone to write than `{0,0,1,2,3,4,5,7,7,7,7,7,6,5,4,3,2,1,0}`.

The mod translations above are all trivial translations with a zero offset. Conversely, you can also use a modulus of 1 to nullify the remainder instead, if you're only interested in the offset. This can be used, e.g., if you want to map controller values to note *numbers* (rather than velocities):

~~~
CC1[1] C0
~~~

This outputs the note with the same number as the controller value, `C0` for value 0, `C#0` for value 1, etc. Note that the remainder, which becomes the velocity of the output note, will always be zero here, so the above translation turns all notes off. To get a nonzero velocity, you specify it in a value list:

~~~
CC2[1] C0{127}
~~~

We also mention in passing here that a modulus 1 translation is actually the "dual" of a trivial mod translation, employing the `'` flag on the output message to transpose quotient and remainder. E.g., these two translations work exactly the same:

~~~
CC1[1] C0
CC1[128] C0'
~~~

Another important idiom is the following, which extracts the low nibble from a controller value. To these ends, we use a modulus of 16 and force the offset value to zero:

~~~
CC1[16]{0} CC1
~~~

Extracting the *high* nibble is just as easy (this is another case where the transposition flag comes in handy):

~~~
CC1[16]{0} CC2'
~~~

Note that this works because the output mapping (which in this case forces the offset to 0) is applied *after* the transposition. Thus you can also output *both* the low and high nibbles at the same time:

~~~
CC1[16]{0} CC1 CC2'
~~~

Using similar rules, you can extract any part of an input value, down to every single bit if needed (see the example at the end of the next section).

As you can see, mod translations in combination with discrete value lists are fairly powerful and let you implement pretty much any desired mapping with ease. There are some limitations, though. In particular, the reversal of the `CC1[1] C0` translation, i.e., *extracting* the note number from a note input, is rather tedious (it involves writing down rules for each and every single note). Also, there's no direct way to combine different kinds of translations of the same input, or to consolidate the values of multiple input messages into a single output message. (The macro facility in the following section lets you address the former, but doesn't help with the latter problem.)

## Macro Translations

There are some situations in which it is hard to construct a translation in a single step, but it may become much easier if we can recursively invoke other translations to construct some parts of the final result. midizap allows you to do this by "calling" the mod translation for a MIDI message on the right-hand side of a translation. This is done by prefixing the message to be expanded with the `$` character:

~~~
CC0= $CC1
~~~

Note that you can *only* call mod translations this way, so the message to be expanded (`CC1` in this example) must be bound in a mod translation somewhere; otherwise you'll get a warning about the message being undefined and no output will be generated. Also, you want to make sure that the message isn't also used as a "real" input, so that you are free to define it as needed. MIDI has 2048 distinct controllers, though (128 for each of the 16 MIDI channels), so in most cases it shouldn't be too hard to find a controller that's free to use for such internal translations.

Let's introduce a few terms which will make it easier to talk about these things. We refer to a mod translation being called in this manner as a *macro translation*, and we also call the left-hand side of the translation a *macro*, and the invocation of a macro using the dollar symbol a *macro call*.

For illustration purposes, let's define the `CC1` macro so that it outputs just a single note message:

~~~
CC0= $CC1    #1
CC1[128] C5  #2
~~~

On a conceptual level, the macro expansion process works pretty much like the production rules of a grammar, with the "dollar" tokens playing the role of the nonterminals. Thus, with the definitions above a `CC0` message will be processed as follows:

- Rule #1 is applied, constructing the output sequence with the `CC1` message as usual.

- Instead of outputting the resulting `CC1` message directly, the program now looks for a mod translation of that message which can be applied recursively.

- Rule #2 is applied, yielding a `C5` message which is substituted for the `$CC1` token in rule #1.

The end result is of course a `C5` message with the same velocity as the value of the `CC1` message, which in turn comes from the original `CC0` controller value.

Of course, we could also just have written `CC0= C5` here and be done with it. So let's try something slightly more complicated which really *needs* recursive translations to work. For instance, suppose that we'd like to output *two* messages instead: the note message `C5` as before, followed by a `CC1` message with just the low nibble of the controller value. Now each of these translations is easy to define:

~~~
CC0= C5
CC0[16]{0} CC1
~~~

But we can't write them this way, because we're not allowed to bind `CC0` in two different translations at once (if you try this, the parser will complain and just ignore the second rule). The single rule `CC0[16]{0} C0 CC1` won't work either since it only passes the low nibble to the `C0` message. However, we can massage those rules a bit to obtain the following:

~~~
CC0= C5 $CC1
CC1[16]{0} CC1
~~~

This works and the transformation we did here is actually quite straightforward. In the same vein, we can combine as many different mod translations as we like, even if they involve different moduli and offset transformations.

If you know C, you will have realized by now that macro translations work pretty much like parameter-less macros in the C programming language. The same caveats apply here, too. Specifically, the configuration language provides no way to break out of a recursive macro, so you do *not* want to have a macro invoke itself (either directly or indirectly), because that will always lead to an infinite recursion. E.g.:

~~~
CC0[128] $CC1
CC1[128] $CC0 # don't do this!
~~~

midizap *will* catch infinite recursion after a few iterations, so for educational purposes you can (and should) try the example above and see what happens. As you'll notice, the program prints an error message indicating the translation and message which caused the problem, so that you can correct your mistake.

So macro translations are too limited to make for a Turing-complete programming language, but there's still a lot that can be done with them. Here's another instructive example which spits out the individual bits of a controller value, using the approach that we discussed earlier in the context of nibble extraction. Input comes from `CC7` in the example, and bit #*i* of the controller value becomes `CC`*i* in the output, where *i* runs from 0 to 6. Note that each of these rules uses a successively smaller power of 2 as modulus and passes on the remainder to the next rule, while transposition is used to extract the topmost bit in the quotient.

~~~
CC7[64]{0} $CC6 CC6'
CC6[32]{0} $CC5 CC5'
CC5[16]{0} $CC4 CC4'
CC4[8]{0}  $CC3 CC3'
CC3[4]{0}  $CC2 CC2'
CC2[2]{0}  CC0  CC1'
~~~

Translations like these may actually be useful at times, but if your rules involve a lot of complicated macros, then you should ask yourself whether midizap is still the right tool for the job. midizap is designed to be simple, and consequently its macro facility is also fairly limited, so you may be better off using something which offers real programming capabilities when tackling more advanced use cases.

# Bugs

There probably are some. Please submit bug reports and pull requests at the midizap [git repository][agraef/midizap]. Contributions are also welcome. In particular, we're looking for interesting configurations to be included in the distribution.

The names of some of the debugging options are rather idiosyncratic. midizap inherited them from Eric Messick's ShuttlePRO program, and we decided to keep them for compatibility reasons.

midizap has only been tested on Linux so far, and its keyboard and mouse support is tailored to X11, i.e., it's pretty much tied to Unix/X11 systems right now. So there's no native Mac or Windows support, and there won't be until someone in the know about Mac and Windows equivalents for the X11 XTest extension comes along and ports it over.

midizap tries to keep things simple, which implies that it has its limitations. In particular, midizap lacks support for system messages and some more interesting ways of mapping, filtering and recombining MIDI data right now. There are other, more powerful utilities which do these things, but they are also more complicated and usually require at least some programming skills. Fortunately, midizap usually does the job reasonably well for simple mapping tasks (and even some rather complicated ones, such as the APCmini Mackie emulation included in the distribution). But if things start getting fiddly then you should consider using a more comprehensive tool like [Pd][] instead.

# See Also

midizap is based on a [fork][agraef/ShuttlePRO] of Eric Messick's [ShuttlePRO program][nanosyzygy/ShuttlePRO], which provides similar functionality for the Contour Design Shuttle devices.

Spencer Jackson's [osc2midi][] utility makes for a great companion to midizap if you also need to convert between MIDI and [Open Sound Control][OSC].

The [Bome MIDI Translator][] seems to be a popular MIDI and keystroke mapping tool for Mac and Windows. It is proprietary software and isn't available for Linux, but it should be worth a look if you need a midizap alternative which runs on these systems.

# Authors

midizap is free and open source software licensed under the GPLv3.

Copyright 2013 Eric Messick (FixedImagePhoto.com/Contact)  
Copyright 2018 Albert Graef (<aggraef@gmail.com>)

This is a version of Eric Messick's ShuttlePRO program which has been redesigned to work with Jack MIDI instead of the Contour Design Shuttle devices. ShuttlePRO was written in 2013 by Eric Messick, based on earlier code by Trammell Hudson and Arendt David. The MIDI support was added by Albert Gräf. All the key and mouse translation features of the original program still work as before, but it goes without saying that the configuration language and the translation code have undergone some substantial changes to accommodate the MIDI input and output facilities. The Jack MIDI backend is based on code from Spencer Jackson's osc2midi utility, and on the simple_session_client.c example available in the Jack git repository.

[MIDI]: https://www.midi.org/
[OSC]: http://opensoundcontrol.org/

[Jack]: http://jackaudio.org/
[a2jmidid]: http://repo.or.cz/a2jmidid.git
[QjackCtl]: https://qjackctl.sourceforge.io/
[FluidSynth]: http://www.fluidsynth.org/
[Qsynth]: https://qsynth.sourceforge.io/
[Pd]: http://puredata.info/

[agraef/midizap]: https://github.com/agraef/midizap
[agraef/ShuttlePRO]: https://github.com/agraef/ShuttlePRO
[nanosyzygy/ShuttlePRO]: https://github.com/nanosyzygy/ShuttlePRO
[osc2midi]: https://github.com/ssj71/OSC2MIDI
[jackaudio/example-clients]: https://github.com/jackaudio/example-clients
[Bome MIDI Translator]: https://www.bome.com/products/miditranslator
