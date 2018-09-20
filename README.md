% midizap(1)

# Name

midizap -- control your multimedia applications with MIDI

# Synopsis

midizap [-hkn] [-d[rskmj]] [-j *name*] [-ost[*n*]] [-P[*prio*]] [-r *rcfile*]

# Options

-h
:   Print a short help message and exit.

-d[rskmj]
:   Enable various debugging options: r = regex (print matched translation sections), s = strokes (print the parsed configuration file in a human-readable format), k = keys (print executed translations), m = midi (MIDI monitor, print all recognizable MIDI input), j = jack (print information about the Jack MIDI backend). Just `-d` enables all debugging options. See Section *Basic Usage*.

-j *name*
:   Set the Jack client name. This overrides the corresponding directive in the configuration file. Default: "midizap". See Section *Jack-Related Options*.

-k
:   Keep track of key (on/off) status. This may occasionally be useful to deal with quirky controllers sending repeated on or off messages. See Section *Key and Data Translations*.

-n
:   No automatic feedback. By default, midizap keeps track of controller feedback from the second input port if it is enabled (`-o2`). This option lets you disable this feature if the second port is being used for other purposes. See Section *Automatic Feedback*.

-o[*n*]
:   Enable MIDI output and set the number of output ports *n* (1 by default). Use *n* = 2 for a second pair of MIDI ports, e.g., for controller feedback, or *n* = 0 to disable MIDI output. This overrides the corresponding directive in the configuration file. See Section *Jack-Related Options*.

-P[*prio*]
:   Run with the given real-time priority (default: 90). See Section *Jack-Related Options*.

-r *rcfile*
:   Set the configuration file name. Default: taken from the MIDIZAP_CONFIG_FILE environment variable if it exists, or ~/.midizaprc if it exists, /etc/midizaprc otherwise. See Section *Configuration File*.

-s[*n*]
:   Pass through system messages from MIDI input to output; *n* optionally specifies the port (0 = none, 1 = first, 2 = second port only), default is pass-through on both ports (if available). This overrides the corresponding directive in the configuration file. See Section *Jack-Related Options*.

-t[*n*]
:   Pass through untranslated (non-system) messages from MIDI input to output; the meaning of the optional parameter *n* is the same as with the `-s` option. This overrides the corresponding directive in the configuration file. See Section *Jack-Related Options*.

# Description

midizap lets you control your multimedia applications using [MIDI][], the Musical Instrument Digital Interface. Modern MIDI controllers are often USB class devices which can be connected to computers without any ado. midizap makes it possible to use this gear with just about any X11-based application. To these ends, it translates Jack MIDI input into X keyboard and mouse events, and optionally MIDI output. It does this by matching the `WM_CLASS` and `WM_NAME` properties of the window that has the keyboard focus against the regular expressions for each application section in its configuration (midizaprc) file. If a regex matches, the corresponding set of translations is used. If a matching section cannot be found, or if it doesn't define a suitable translation, the program falls back to a set of translations in a default section at the end of the file, if available.

The midizaprc file is just an ordinary text file which you can edit to configure the program. The configuration language is fairly straightforward, basically the file is just a list of MIDI messages (denoted with familiar human-readable mnemonics, no hex numbers!) and their translations, which is divided into sections for different applications. An example.midizaprc file is included in the sources to get you started, and you can find some more examples of configuration files for various purposes in the examples directory.

midizap provides you with a way to hook up just about any MIDI controller to your applications. Even if your target application already supports MIDI, midizap's MIDI output option will be useful if your controller can't work directly with the application because of protocol incompatibilities. In particular, you can use midizap to turn pretty much any MIDI controller with enough faders and buttons into a Mackie-compatible mixing console ready to be used with most DAW (digital audio workstation) programs. Another common use case is video editing software, which rarely offers built-in MIDI support. midizap allows you to map the faders, encoders and buttons of your MIDI controller to keyboard commands of your video editor for cutting, marking, playback, scrolling, zooming, etc.

In other words, as long as the target application can be controlled with simple keyboard shortcuts and/or MIDI commands, chances are that midizap can make it work (at least to some extent) with your controller.

# Installation

First, make sure that you have the required dependencies installed. The program needs a few X11 libraries and Jack. And of course you need GNU make and gcc (the GNU C compiler). On Ubuntu and other Debian-based systems you should be able to get everything that's needed by running this command:

    sudo apt install build-essential libx11-dev libxtst-dev libjack-dev

Then just run `make` and `sudo make install`. This installs the example.midizaprc file as /etc/midizaprc, and the midizap program and the manual page in the default install location. Usually this will be under /usr/local, but the installation prefix can be changed with the `prefix` variable in the Makefile. Also, package maintainers can use the `DESTDIR` variable to install into a staging directory for packaging purposes.

# Configuration File

After installation the system-wide default configuration file will be in /etc/midizaprc, where the program will be able to find it. We recommend copying this file to your home directory, renaming it to .midizaprc:

    cp /etc/midizaprc ~/.midizaprc

The ~/.midizaprc file, if it exists, takes priority over /etc/midizaprc, so it becomes your personal default midizap configuration. The midizaprc file included in the distribution is really just an example; you're expected to edit this file to adjust the bindings for the MIDI controllers and the applications that you use.

It is also possible to specify the configuration file to be used, by invoking midizap with the `-r` option followed by the name of the midizaprc file on the command line. This is often used with more specialized configurations dealing with specific applications or MIDI controllers. E.g., to try one of sample configurations in the sources: `midizap -r examples/MPKmini2.midizaprc`

The program automatically reloads the midizaprc file whenever it notices that the file has been changed. Thus you can edit the file while the program keeps running, and have the changes take effect immediately without having to restart the program. When working on new translations, you may want to run the program in a terminal, and employ some or all of the debugging options explained below to see exactly how your translations are being processed.

# Basic Usage

The midizap program is a command line application, so you typically run it from the terminal. However, it is also possible to launch it from your Jack session manager (see *Jack-Related Options* below) or from your desktop environment's startup files once you've set up everything to your liking.

Try `midizap -h` for a brief summary of the available options with which the program can be invoked.

midizap uses [Jack][] for doing all its MIDI input and output, so you need to be able to run Jack and connect the Jack MIDI inputs and outputs of the program. We recommend using a Jack front-end and patchbay program like [QjackCtl][] for this purpose. In QjackCtl's setup, make sure that you have selected `seq` as the MIDI driver. This exposes the ALSA sequencer ports of your MIDI hardware and other non-Jack ALSA MIDI applications as Jack MIDI ports, so that they can easily be connected to midizap. (We're assuming that you're using Jack1 here. Jack2 works in a very similar way, but may require some more fiddling; in particular, you may have to use [a2jmidid][] as a separate ALSA-Jack MIDI bridge in order to have the ALSA MIDI devices show properly as Jack MIDI devices.)

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

You can invoke the program with various debugging options to get more verbose output. E.g., try running `midizap -drk` to have the program print the recognized configuration sections and translations as they are executed. Now press some of the keys and move the modulation wheel. You'll see something like:

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

The debugging output tells you pretty much everything you need to know about what's going on inside midizap, and helps you along when you start developing your own configurations. The `-d` option can be combined with various option characters to choose exactly which kinds of debugging output you want; `r` ("regex") prints the matched translation section (if any) along with the window name and class of the focused window; `s` ("strokes") prints the parsed contents of the configuration file in a human-readable form whenever the file is loaded; `k` ("keys") shows the recognized translations as the program executes them, in the same format as `s`; `m` ("MIDI") prints *any* received MIDI input, so that you can figure out which MIDI tokens to use for configuring the translations for your controller; and `j` adds some useful information about the Jack backend, so that you see when the Jack client is ready, and which MIDI ports it gets connected to. You can also just use `-d` to enable all debugging output. Moreover, most of these options are also available as directives in the midizaprc file, so that you can turn them on and off as needed without having to exit the program; please check the comments at the beginning of example.midizaprc for a list of these directives.

Most of the other translations in the distributed midizaprc file assume a Mackie-like device with standard playback controls and a jog wheel. There are also a few more generic examples, like the one above, which will work with almost any kind of MIDI keyboard. The examples are mostly for illustrative and testing purposes, though, to help you get started. You will want to edit them and add translations for your own controllers and favorite applications.

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

Note the `-10` suffix on the output messages in the above example, which indicates that output goes to MIDI channel 10. In midizaprc syntax, MIDI channels are 1-based, so they are numbered 1..16, and 10 denotes the GM (General MIDI) drum channel. E.g., the input note `C4` is mapped to `C3-10`, the note C in the third MIDI octave, which on channel 10 will produce the sound of a bass drum, at least on GM compatible synthesizers like Fluidsynth. The binding for the volume controller (`CC7`) at the end of the entry sends volume changes to the same drum channel (`CC7-10`), so that you can use the volume control on your keyboard to change the volume on the drum channel.

Besides MIDI notes and control change (`CC`) messages, the midizap program also recognizes key and channel pressure (`KP`, `CP`), program change (`PC`), and pitch bend (`PB`) messages, which should cover most common use cases. These are discussed in more detail in the *Translation Syntax* section below. In addition, unrecognized MIDI messages can be simply passed through with the `-t` option. Also, while midizap cannot translate system messages such as system exclusive, you can pass them through as well with the `-s` option, see the following section for details.

# Jack-Related Options

There are some additional directives (and corresponding command line options) to set midizap's Jack client name and the number of input and output ports it uses. If both the command line options and directives in the midizaprc file are used, the former take priority, so that it's always possible to override the configuration settings from the command line.

Firstly, there's the `-j` option and the `JACK_NAME` directive which change the Jack client name from the default (`midizap`) to whatever you want it to be. To use this option, simply invoke midizap as `midizap -j client-name`, or put the following directive into your midizaprc file:

~~~
JACK_NAME "client-name"
~~~

This option is useful, in particular, if you're running multiple instances of midizap with different configurations for different controllers and/or target applications, and you want to have the corresponding Jack clients named differently, so that they can be identified more easily.

Secondly, we've already seen the `-o` option which is used to equip the Jack client with an additional output port. This can also be achieved with the `JACK_PORTS` directive in the midizaprc file, as follows:

~~~
JACK_PORTS 1
~~~

The given number of output ports must be 0, 1 or 2. Zero means that MIDI output is disabled (which is the default). You may want to use `JACK_PORTS 1` if the configuration is primarily aimed at doing MIDI translations, so you'd like to have MIDI output enabled by default. `JACK_PORTS 2` or the `-o2` option indicates that *two* pairs of input and output ports are to be created. The second port is typically used to deal with controller feedback from the application, see the *MIDI Feedback* section for details.

Not very surprisingly, at least one output port is needed if you want to output any MIDI at all; otherwise MIDI messages on the right-hand side of translations will be silently ignored. Also note that midizap doesn't provide you with any options to actually connect the ports you created to other Jack MIDI clients. There already are plenty of excellent utilities specifically designed for this purpose, so there's no need to replicate that functionality in midizap. As already mentioned, we recommend using QjackCtl, which also offers a persistent MIDI patchbay, so that you can have the right connections automatically set up for you whenever you launch midizap. You then use the Jack client name option if you need to distinguish multiple midizap instances. (The examples folder in the sources contains a QjackCtl patchbay named midizap.xml which has been set up to work with any of the included examples. Use the "Load" button in QjackCtl's Patchbay dialog to load it, then hit the "Activate" button to have QjackCtl actually use it.)

If at least one output port is available then it also becomes possible to pass through MIDI messages from input to output unchanged. Two options are available for this: `-t` which passes through any ordinary (non-system) message for which there are no translations (not even in the default section), and `-s` which passes through all system messages. The former is convenient if the incoming MIDI data only needs to be modified in a few places to deal with slight variations in the protocol. The latter may be needed when the input data may contain system messages; midizap cannot translate these, but it can pass them on unchanged when necessary. You can find examples for both use cases in the examples folder in the sources.

The corresponding directives are named `PASSTHROUGH` and `SYSTEM_PASSTHROUGH`, respectively. In either case, you can optionally specify which port the pass-through should apply to (0 means none, 1 the first, 2 the second port; if no number is given, both ports are used). For instance, if you only need system pass-through on the feedback port, you might write `SYSTEM_PASSTHROUGH 2`, or use the `-s2` option on the command line; and to have unrecognized MIDI messages passed through in either direction, simply use `PASSTHROUGH`, or `-t`.

Note that all these options can only be set at program startup. If you later edit the corresponding directives in the configuration file, the changes won't take effect until you restart the program.

midizap also supports *Jack session management*, which makes it possible to record the options the program was invoked with, along with all the MIDI connections. QjackCtl has its own built-in Jack session manager which is available in its Session dialog. To use this, launch midizap and any other Jack applications you want to have in the session, use QjackCtl to set up all the connections as needed, and then hit the "Save" button in the Session dialog to have the session recorded. Now, at any later time you can rerun the recorded session with the "Load" button in the same dialog. Also, your most recent sessions are available in the "Recent" menu from where they can be launched quickly.

Finally, midizap also offers an option to run the program with *real-time priorities*. Jack itself usually does that anyway where needed, but midizap's main thread won't unless you run it with the `-P` option (`midizap -P`, or `midizap -P80` if you also want to specify the priority). Using this option, midizap should be able to get down to MIDI latencies in the 1 msec ballpark which should be good enough for most purposes. (Note that there's no need to use this option unless you actually notice high latencies or jitter in the MIDI output.)

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

Lines beginning with a `[`*name*`]` header are also special. Each such line introduces a translation class *name*, which may be followed by an extended regular expression *regex* (see the regex(7) manual page) to be matched against window class and title. Note that *everything* following the `[`*name*`]` header on the same line is taken verbatim; the *regex* part is the entire rest of the line, ignoring leading and trailing whitespace, but including embedded whitespace and `#` characters (so you can't place a comment on such lines).

To find a set of eligible translations, midizap matches class and title of the window with the keyboard focus against each section, in the order in which they are listed in the configuration file. For each section, midizap first tries to match the window class (the `WM_CLASS` property), then the window title (the `WM_NAME` property). The first section which matches determines the translations to be used for that window. An empty *regex* for the last class will always match, allowing default translations. If a translation cannot be found in the matched section, it will be loaded from the default section if possible. In addition, there are two special default sections labeled `[MIDI]` and `[MIDI2]` which are used specifically for MIDI translations, please see the *MIDI Output* and *MIDI Feedback* sections for details. If these sections are present, they should precede the main default section. All other sections, including the main default section, can be named any way you like; the given *name* is only used for debugging output and diagnostics, and needn't be unique.

This means that when you start writing a section for a new application, the first thing you'll have to do is determine its window class and title, so that you can figure out a regular expression to use in the corresponding section header. The easiest way to do this is to run midizap with the `-dr` option. Make sure that your controller is hooked up to midizap, click on the window and wiggle any control on your device. You'll get a message like the following, telling you both the title and the class name of the window (as well as the name of the translation class if the window is already recognized):

~~~
translation: Default for mysession - Ardour (class ardour_ardour)
~~~

Here, the class name is "ardour_ardour" and the window title "mysession - Ardour". Either can be used for the regular expression, but the class name usually provides the more specific clues for identifying an application. So we might write:

~~~
[Ardour] ^ardour_ardour$
~~~

The header is followed by a list of translations which define what output should be produced for the given MIDI input. Each translation must be on a line by itself. The left-hand side (first token) of the translation denotes the MIDI message to be translated. The corresponding right-hand side (the rest of the line) is a sequence of zero or more tokens, separated by whitespace, indicating MIDI and X11 keyboard and mouse events to be output. The output sequence may be empty, or just the special token `NOP` (which doesn't produce any output), to indicate that the translation outputs nothing at all; this suppresses the default translation for this input. Translation classes may be empty as well (i.e., not provide any translations), in which case *only* the default translations are active, even if a later non-default section matches the same window.

**NOTE:** Translations may be listed in any order, but they *must be determined uniquely*, i.e., each input message may be bound to at most one output sequence in each translation class. Otherwise, the parser will print an error message, and the extra translations will be ignored. This restriction makes it easier to detect inconsistencies, and it also ensures that midizap's operation is completely *deterministic*. That is, for each input sequence on a given window the program will always generate exactly the same output sequence.

Example:

~~~
[Terminal] ^.*-terminal.*|konsole|xterm$ 
 F5    XK_Up
 F#5   "pwd"
 G5    XK_Down
 G#5   "ls"
 A5    XK_Return
~~~

This binds a few keys in the middle octave to the Up, Down and Return keys as well as some frequently used shell commands, in a section named `Terminal` which matches some common types of terminal windows by their class names. The bindings in this translation class will let you operate the shell from your MIDI keyboard when the keyboard focus is on a terminal window.

## MIDI Message Notation
 
There's no real standard for symbolic designations of MIDI messages, but we hope that most users will find midizap's notation easy to understand and remember. Notes are specified using a format which musicians will find familiar: a note name `A`, `B`, ..., `G` is followed by an (optional) accidental (`#` or `b`), and a (mandatory) MIDI octave number. Note that all MIDI octaves start at the note C, so `B0` comes before `C1`. By default, `C5` denotes middle C (you can change this if you want, see *Octave Numbering* below). Enharmonic spellings are equivalent, so, e.g., `D#5` and `Eb5` denote exactly the same MIDI note.

The other messages are denoted using short mnemonics: `KP:`*note* (aftertouch a.k.a.\ key pressure for the given note); `CC`*n* (control change for the given controller number); `PC`*n* (program change for the given program number); `CP` (channel pressure); and `PB` (pitch bend). We will go into the other syntactic bits and pieces of MIDI message designations later, but it's good to have the following grammar in EBNF notation handy for reference. (To keep things simple, the grammar is somewhat abridged, but it covers all the frequently used notation. There is some additional syntax for special forms of translations which will be introduced later. Also, at the end of the manual you can find a complete grammar for the entire configuration language.)

~~~
token ::= msg [ "[" number "]" ] [ "-" number ] [ flag ]
msg   ::= ( note | other ) [ number ]
note  ::= ( "A" | ... | "G" ) [ "#" | "b" ]
other ::= "CH" | "PB" | "PC" | "CC" | "CP" | "KP:" note
flag  ::= "-" | "+" | "=" | "<" | ">" | "~"
~~~

Case is ignored here, so `CC`, `cc` or even `Cc` are considered to be exactly the same token by the parser, although by convention we usually write them in uppercase. Numbers are always integers in decimal. The meaning of the `msg` number depends on the context (octave number for notes and key pressure, controller or program number in the range 0..127 for other messages, MIDI channel number in the range 1..16 for the special `CH` token). This can optionally be followed by a number in brackets, denoting a nonzero step size. Also optionally, a suffix with a third number (after the dash) denotes the MIDI channel in the range 1..16; otherwise the default MIDI channel is used (which is always 1 on the left-hand side, but can be set on the right-hand side with the `CH` token). The optional "increment" flag at the end of a token indicates a "data" translation which responds to incremental (up/down) value changes rather than key presses, cf.\ *Key and Data Translations* below.

## Octave Numbering

A note on the octave numbers in MIDI note designations is in order here. There are various different standards for numbering octaves, and different programs use different standards, which can be rather confusing. E.g., there's the ASA (Acoustical Society of America) standard where middle C is C4, also known as "scientific" or "American standard" pitch notation. At least two other standards exist specifically for MIDI octave numbering, one in which middle C is C3 (so the lowest MIDI octave starts at C-2), and zero-based octave numbers, which start at C0 and have middle C at C5. There's not really a single "best" standard here, but the latter seems intuitive to mathematically inclined and computer-savvy people, and is also what is used by default in the midizaprc file.

However, you may want to change this, e.g., if you're working with documentation or MIDI monitoring software which uses a different numbering scheme. To do this, just specify the desired offset for the lowest MIDI octave with the special `MIDI_OCTAVE` directive in the configuration file. For instance:

~~~
MIDI_OCTAVE -1 # ASA pitches (middle C is C4)
~~~

Note that this transposes *all* existing notes in translations following the directive, so if you add this option to an existing configuration, you probably have to edit the note messages in it accordingly.

## Key and Data Translations

Translations come in two flavors or "modes", *key translations* and *data translations*, which differ in the way the extra data payload of the input message, called the *parameter value* (or just *value* for short), is processed. The parameter value depends on the type of MIDI message. Program changes (`PC`) have no value at all. For notes, as well as key and channel pressure messages (`CP`, `KP`), it is a velocity value; for control changes (`CC`), a controller value; and for pitch bend messages (`PB`), a pitch bend value. The latter is a 14 bit value composed of the two data bytes in the message, which is considered as a signed quantity in the range -8192..8191, where 0 denotes the center value. In all other cases, the parameter value consists of a single data byte, which denotes an unsigned 7 bit quantity in the range 0..127.

Note that since translations must be determined uniquely in each translation class, you can't have both key *and* data translations for the same input in the same section; it's either one or the other.

*Key mode* is the default mode and is available for all message types. In this mode, MIDI messages are considered as keys which can be "pressed" ("on") or "released" ("off"). Any nonzero data value means "pressed", zero "released". Two special cases need to be considered here:

- For pitch bends, any positive *or* negative value means "pressed", while 0 (the center value) means "released".

- Since program changes have no parameter value associated with them, they don't really have an "on" or "off" status. But they are treated in the same key-like fashion anyway, assuming that they are "pressed" and then "released" immediately afterwards.

Key mode can optionally keep track of the current key (on/off) status, so that a key translation is only triggered when its status actually changes. Normally this shouldn't be necessary and thus it is disabled by default. You usually want to keep it that way, unless you're dealing with a quirky controller or an unreliable transmission line. In such cases it may useful to enable this option with the `-k` option on the command line, so that, e.g., repeated note-ons or -offs are filtered out automatically.

*Data mode* is available for all messages with a parameter value, i.e., anything but `PC`. In this mode, the actual value of the message is processed rather than just the on/off state. Data mode is indicated with a special suffix on the message token which denotes a step size and/or the direction of the value change which the rule should apply to: increment (`+`), decrement (`-`), or both (`=`). The two parts are both optional, but at least one of them must be present (otherwise the rule is interpreted as a key translation).

In the following, we concentrate on *incremental* data mode messages, i.e., the kind which has an increment suffix. In this case, the optional step size in brackets indicates the amount of change required to trigger the translation, so its effect is to downscale the amount of change in the input value. The variant without an increment suffix is more complicated and mostly intended for more specialized uses, so we'll have a look at it later in the *Mod Translations* section.

Data mode usually tracks changes in the *absolute* value of a control. However, for `CC` messages there's also an alternative mode for so-called *relative* controllers which can found on some devices. These usually take the form of jog wheels or rotary encoders which can be turned endlessly in either direction, therefore we also just call them *encoders* for short in the following. There are various kinds of these which differ in the way they represent relative changes, but these days most encoders found on MIDI controllers employ the *sign bit* format; this is also the only kind supported by midizap in the present implementation. In the sign-bit representation, a value <64 denotes an increment (representing clockwise rotation), and a value >64 a decrement (counter-clockwise rotation); the actual amount of change is in the lower 6 bits of the value. In the message syntax, sign-bit values are indicated by using the suffixes `<`, `>` and `~` in lieu of `-`, `+` and `=`, respectively. These flags are only permitted with `CC` messages.

## Keyboard and Mouse Events

Keyboard and mouse output consists of X key codes with optional up/down indicators, or strings of printable characters enclosed in double quotes. The syntax of these items, as well as the special `RELEASE` and `SHIFT` tokens which will be discussed later, are described by the following grammar:

~~~
token   ::= "RELEASE" | "SHIFT" [ number ] |
	        keycode [ "/" keyflag ] | string
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

Translations are handled differently depending on the input mode (cf.\ *Key and Data Translations* above). In *key mode*, there are separate press and release sequences. The former is invoked when the input key goes "down" (i.e., when the "on" status is received), the latter when the input key goes "up" again ("off" status). More precisely, at the end of the press sequence, all down keys marked by `/D` will be released, and the last key not marked by `/D`, `/U`, or `/H` will remain pressed. The release sequence will begin by releasing the last held key. If keys are to be pressed as part of the release sequence, then any keys marked with `/D` will be repressed before continuing the sequence. Keycodes marked with `/H` remain held between the press and release sequences. For instance, let's take a look at one of the more conspicuous translations in the example above:

~~~
G5 XK_Alt_L/D "v" XK_Alt_L/U "x" RELEASE "q"
~~~

This translation has the following meaning: When the `G5` key is pressed on the MIDI keyboard, the key sequence `Alt+v x` is initiated, keeping the `x` key pressed (so it may start auto-repeating after a while). The program then sits there waiting (possibly executing other translations) until you release the `G5` key again, at which point the `x` key is released and the `q` key is pressed (and released).

One pitfall here is that character strings in double quotes are just a shorthand for the corresponding X key codes, ignoring case. Thus, e.g., `"abc"` actually denotes the keysym sequence `XK_a XK_b XK_c`, as does `"ABC"`. So in either case the *lowercase* string `abc` will be output. To output uppercase letters, it is always necessary to add one of the shift modifiers to the output sequence. E.g., `XK_Shift_L/D "abc"` will output `ABC` in uppercase.

In *data mode* only a single sequence is output whenever the message value changes. So there's no separate "release" sequence, and at the end of the sequence, *all* down keys will be released. Instead, data mode distinguishes between *increments* and *decrements* of the input value and outputs the corresponding sequence for each unit change. For instance, the following translations move the cursor left or right whenever the volume controller (`CC7`) decreases and increases, respectively. Also, the number of times one of these keys is output corresponds to the actual change in the value. Thus, if in the example `CC7` increases by 4, say, the program will press (and release) `XK_Right` four times in close succession, moving the cursor in the focused window four positions to the right.

~~~
CC7- XK_Left
CC7+ XK_Right
~~~

Sign-bit encoders are treated in an analogous fashion, but in this case the increment or decrement is determined directly by the input message. One example for this type of controller is the big jog wheel (`CC60`) on some Mackie devices, which can be processed as follows (using `<` and `>` in lieu of `-` and `+` as the increment flag of the `CC` message):

~~~
CC60< XK_Left
CC60> XK_Right
~~~

The corresponding "bidirectional" translations, which are indicated with the `=` and `~` flags, are mostly used with MIDI output; the same goes for the special `SHIFT` token. Thus we'll discuss these in later sections, see *MIDI Events* and *Shift State* below.

In data mode, input messages can also have a *step size* associated with them, which has the effect of downscaling changes in the parameter value. The default step size is 1 (no scaling). To change it, the desired step size is written in brackets immediately after the message token and before the increment flag. A step size *k* indicates that the translation is executed whenever the input value has changed by *k* units. For instance, to slow down the cursor movement in the example above by a factor of 4:

~~~
CC7[4]- XK_Left
CC7[4]+ XK_Right
~~~

The same goes for sign-bit encoders:

~~~
CC60[4]< XK_Left
CC60[4]> XK_Right
~~~

Note that since there's no persistent absolute controller state in this case, this simply scales down the actual increment value in the message itself.

## MIDI Events

Most of the notation for MIDI messages on the left-hand side of a translation rule also carries over to the output side. The only real difference is that the increment flags `+-=<>` aren't permitted here, as they are only used to determine the input mode (key or data) of the entire translation. The `~` flag *is* allowed, however, to indicate output in sign-bit encoder format in data translations, see below. Step sizes are permitted as well on the output side, in *both* key and data translations. Their meaning depends on the kind of translation, however. In key translations, they denote the (nonzero) value to be used for the "on" state in the press sequence; in data translations, they indicate the amount of change for each unit input change (which has the effect of *upscaling* the value change).

The output sequence can involve as many MIDI messages as you want, and these can be combined freely with keyboard and mouse events in any order. However, as already discussed in Section *MIDI Output* above, you also need to invoke the midizap program with the `-o` option to make MIDI output work. Otherwise, MIDI messages in the output translations will just be silently ignored.

There is one special MIDI token `CH` which can only be used on the output side. It is always followed by a MIDI channel number in the range 1..16. This token doesn't actually generate any MIDI message, but merely sets the default MIDI channel for subsequent MIDI messages in the same output sequence, which is convenient if multiple messages are output to the same MIDI channel. For instance, the sequence `C5-2 E5-2 G5-2`, which outputs a C major chord on MIDI channel 2, can also be abbreviated as `CH2 C5 E5 G5`.

For key mode inputs, the corresponding "on" or "off" event is generated for all MIDI messages in the output sequence, where the "on" value defaults to the maximum value (127 for controller values, 8191 for pitch bends). Thus, e.g., the following rule outputs a `CC64` (hold pedal) message with controller value 127 each time `C3` is pressed (and another `CC64` message with value 0 when the note is released again):

~~~
C3 CC64 # hold pedal on/off
~~~

The value for the "on" state can also be denoted explicitly with a step size:

~~~
C3 CC64[64] # hold pedal with an "on" value of 64
~~~

For pitch bends, the step size can also be negative. For instance, the following rules assign two keys to bend down and up by the maximum amount possible:

~~~
C2 PB[-8192] # bend down
D2 PB[8191]  # bend up
~~~

Rules similar to the ones above may be useful if your MIDI keyboard doesn't have a hold pedal or pitch bend wheel, as they let you set aside some keys to emulate those functions.

Let's now have a look at data mode. There are two additional flags `=` and `~` for data translations which are most useful with MIDI output, which is why we deferred their discussion until now. In pure MIDI translations not involving any key or mouse output, the increment and decrement sequences are usually identical, in which case the `=` suffix can be used to indicate that the output is to be used for *both* increments and decrements. For instance, to map the modulation wheel (`CC1`) to the volume controller (`CC7`):

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

The `~` flag can be used to denote encoders in output messages, too. E.g., to translate a standard (absolute) MIDI controller to a sign-bit encoder value, you might use a rule like:

~~~
CC48= CC16~
~~~

Of course, this won't magically turn a standard controller into a *real* encoder; its range will still be limited. One way to properly emulate the endless range of an encoder is to expend *two* absolute controllers, one for reducing and another one for increasing the value. For instance:

~~~
CC1- CC60~
CC2+ CC60~
~~~

Note that the "down" controller (`CC1` in this example) only reacts to negative, and the "up" controlller (`CC2`) only to positive changes. Therefore it's possible to "rewind" each control when getting to the end of its range, so that you can continue to change its value as much as you want. Admittedly, this solution is a bit quirky, but hey -- if you absolutely need to emulate a real encoder on a device which doesn't have one, that's one way to do it.

Step sizes work on the right-hand side of data translations as well. You might use these to scale up value changes, e.g., when translating from control changes to pitch bends:

~~~
CC1= PB[128]
~~~

The step size can also be negative, which allows you to reverse the direction of a controller if needed. E.g., the following will output values going down from 127 to 0 as input values go up from 0 to 127:

~~~
CC1= CC1[-1]
~~~

Note that you can also place step sizes on *both* the left-hand and right-hand side of a rule, in order to approximate a rational scaling factor:

~~~
CC1[3]= CC1[2]
~~~

The above translation will only be triggered when the input value changes by 3 units, and the change in the output value will then be doubled again, so that the net effect is to scale the input value by 2/3. (Note that for most ratios this will only give a rough approximation; the method works best if the input and output step sizes are reasonably small.)

**NOTE:** All data translations we've seen so far handle *incremental* value changes. In order to be able to detect these changes and, in the case of MIDI output, change the output values accordingly, midizap has to keep track of all the current parameter values of all messages on all MIDI channels, for both input and output. This is easy enough, but midizap usually has no way of knowing the *actual* state of your controllers and MIDI applications, so when the program starts up, it simply assumes all these values to be zero. This means that midizap's "shadow" values of controllers, pitch bends etc.\ may initially well be out of sync with your input devices and applications, and you may have to wiggle a control in order to "calibrate" it.

This becomes most apparent when using negative step sizes, as in the translation `CC1= CC1[-1]` from above. In this case, you will first have to move the control all the way up and then down again to get it working properly. There are some ways to mitigate these issues, however. In particular, midizap can utilize controller feedback from the application, please check the *MIDI Feedback* section below for details. Also, encoders need no calibration as they represent incremental changes anyway, and there's an alternative form of data translation, to be discussed in Section *Mod Translations*, which always works with absolute values and thus needs no calibration either.

## Shift State

Like the `CH` token, the special `SHIFT` token doesn't generate any output by itself; it merely toggles an internal shift state which can then be tested by other translations to generate alternative output sequences. The `^` prefix denotes a rule which is active only in shifted state, while unprefixed rules are active in *both* shifted and unshifted state. Also, a prefixed rule *always* overrides an unprefixed one (no matter in which order they appear in the configuration file), so if you specify both, the former will be used in shifted, the latter in unshifted state. That is, unprefixed rules act as defaults which can be overridden by prefixed ones.

Many DAW controllers have some designated shift keys which can be used for this purpose, but the following will actually work with any key-style MIDI message. E.g., to bind the shift key on an AKAI APCmini controller (`D8`):

~~~
D8 SHIFT
~~~

This rule doesn't have a prefix, so it is used to turn shift state both on and off, giving a "Caps Lock"-style of toggle key. If you'd rather have an ordinary shift key which turns on shift state when pressed and immediately turns it off when released again, you can do that as follows:

~~~
D8 SHIFT RELEASE SHIFT
~~~

Note that in either case `SHIFT` works as a *toggle*; when executed, it turns the shift status from off to on, and vice versa from on to off.

Having set up the translation for the shift key itself, we can now assign, depending on the shift state, different functions to buttons and faders. Here's a typical example which maps a control change to either Mackie-style fader values encoded as pitch bends, or sign-bit encoder values:

~~~
 CC48= PB[128]  # default: translate to pitch bend
^CC48= CC16~    # shifted: translate to encoder
~~~

It's also possible to explicitly denote a rule which is active *only* in unshifted state. Unshifted state is specified with the `0^` (zero-caret) prefix, so you can write:

~~~
0^CC56= PB[128]-9 # unshifted: translate to pitch bend
~~~

The syntax is a bit awkward, but the case arises rarely (usually, you'll just write an unprefixed rule instead).

midizap actually supports up to four different shift states, which are denoted `SHIFT1` to `SHIFT4`, with the corresponding prefixes being `1^` to `4^`. Unprefixed rules are enabled by default in all of these. The `SHIFT` token and `^` prefix we've seen above are in fact just shortcuts for `SHIFT1` and `1^`, respectively. So our first example above is equivalent to:

~~~
D8 SHIFT1 RELEASE SHIFT1
CC48= PB[128]
1^CC48= CC16~
~~~

We might add another shift key and use it to assign yet another function to the same input message:

~~~
F7 SHIFT2 RELEASE SHIFT2
2^CC48- XK_Left
2^CC48+ XK_Right
~~~

Another way to look at this is that translations are organized in *layers*. Layer 0 contains the unshifted translations, layer 1 to 4 the translations prefixed with the corresponding shift level. Unprefixed translations are available in all of these layers, unless they are overriden by translations specifically assigned to one of the layers. To keep things simple, only one layer can be active at any one time; if you press a shift key while another layer is still active, it will be deactivated automatically before activating the new layer.

Also note that the status of internal shift keys is *only* available to the midizap program; the host application never gets to see them. If your host software does its own handling of shift keys, it's usually more convenient to simply pass those keys on to the application. However, `SHIFT` comes in handy if your controller doesn't have enough buttons and faders, since it makes it possible to multiply the amount of controls available on the device. For instance, you can emulate a Mackie controller with both encoders and faders on a device which only has a single set of faders, simply by assigning the shifted faders to the encoders, as shown in the first example above.

# MIDI Feedback

Some MIDI controllers need a more elaborate setup than what we've seen so far, because they have motor faders, LEDs, etc., requiring feedback from the application. To accommodate these, you can use the `-o2` option of midizap, or the `JACK_PORTS 2` directive in the midizaprc file, to create a second pair of MIDI input and output ports, named `midi_in2` and `midi_out2`. Use of this option also activates a second MIDI default section in the midizaprc file, labeled `[MIDI2]`, which is used exclusively for translating MIDI input from the second input port and sending the resulting MIDI output to the second output port. Typically, the translations in the `[MIDI2]` section will be the inverse of those in the `[MIDI]` section, or whatever it takes to translate the MIDI feedback from the application back to MIDI data which the controller understands.

You then wire up midizap's `midi_in` and `midi_out` ports to controller and application as before, but in addition you also connect the application back to midizap's `midi_in2` port, and the `midi_out2` port to the controller. This reverse path is what is needed to translate the feedback from the application and send it back to the controller. (The `-s` option a.k.a.\ `SYSTEM_PASSTHROUGH` directive also works on the feedback port, passing through all system messages from the second input port to the second output port unchanged.)

The distribution includes a full-blown example of this kind of setup for your perusal, please check examples/APCmini.midizaprc in the sources. It shows how to emulate a Mackie controller with AKAI's APCmini device, so that it readily works with DAW software such as Ardour.

## Automatic Feedback

If done right, MIDI feedback will eliminate the problem of controls being out of sync with the application. midizap has some built-in logic to help with this. Specifically, the current state of controls received from the host application via the `midi_in2` port will be recorded, so that subsequent MIDI output for incremental data translations on the first output port will use the proper values for determining the required relative changes. (The same goes for the reverse path, recording the values from `midi_in` so that data translations feeding back to the controller will work correctly.)

We refer to this as *automatic feedback*. Some devices may provide you with sign-bit encoders which don't need any kind of feedback for themselves. In this case the automatic feedback will be all that's needed to keep controller and application in sync, and you don't even have to write any translation rules for the feedback; just enabling the second input port and hooking it up to the application will be enough. Other controllers may provide faders with motors or LEDs on them, however, in which case additional translation rules for the feedback will be needed.

**NOTE:** Automatic feedback is enabled automatically whenever you create a second pair of ports using `-o2`. If you're using the second pair for more esoteric purposes, you may want to disable this feature, which can be done with the `-n` option or the `NO_FEEDBACK` directive in the configuration file. Use this option *only* if feedback isn't needed with your application.

## Direct Feedback

Translations can also provide feedback themselves. To these ends, any MIDI message on the right-hand side of a translation can be prefixed with the `!` character. This outputs the message as usual, but flips the output ports, so that the message will go to port 2 in a forward translation destined for port 1, and vice versa to port 1 in a feedback translation (in the `[MIDI2]` section) destined for port 2.

We call this *direct feedback*. For instance, suppose that on our MIDI controller we have a fader taking the form of a touchstrip `CC1` that has some LEDs for conveying its current value. We might like to translate that message to a `CC7` message for the application, while also providing feedback to the controller. The following translation will do this:

~~~
CC1= CC7 !CC1
~~~

Now, whenever you touch that fader, the corresponding value will be sent as `CC7` to the application, while the same value is sent back as `CC1` to the controller (which presumably will light the appropriate LEDs on the fader).

Another example are internal shift buttons (cf.\ *Shift State* above). The host application never gets to see these, so chances are that we'll have to provide suitable feedback ourselves in order to light the buttons. E.g., the following should usually turn on the LED of the button when pressed, and turn it off again when released:

~~~
D8 SHIFT !D8 RELEASE SHIFT !D8
~~~

This will work for simple cases involving only a single shift key whose state always matches the button state. However, for more complicated setups possibly involving multiple shift keys, it's better to use the `^` prefix instead:

~~~
D8 SHIFT ^D8 RELEASE SHIFT ^D8
~~~

This variation of direct feedback is tailored to shift keys, and it *only* works with key translations. It also operates under the following two assumptions:

- Feedback messages come *after* the corresponding `SHIFT` token in the translation, so that midizap knows which shift state the message belongs to.

- The shift keys can be turned off by sending them a zero parameter value.

midizap has some built-in machinery to deal with shift key feedback which handles direct feedback for shift keys in an automatic fashion, provided that these conditions are met. In particular, midizap ensures that the feedback message reflects the *shift* (rather than the button) state, which is needed to make "Caps Lock"-style toggle keys like the following work correctly:

~~~
D8 SHIFT ^D8
~~~

This turns the key on when pushed, and toggles it off again when pushed a second time. Note that you can't get this behavior with the basic direct feedback facility, since there's no way to keep track of the required status information across different translations. Moreover, midizap also maintains the current status of all shift keys and automatically turns them off when switching from one shift status to another, so that the keys will be lit properly even if you use multiple shift keys in your configuration.

midizap still has a few more tricks up its sleeves, but they require the use of a special kind of data translation. These are a separate topic in their own right, so we'll introduce them in the next section.

# Mod Translations

Most of the time, MIDI feedback uses just the standard kinds of MIDI messages readily supported by midizap, such as note messages which make buttons light up in different colors, or control change messages which set the positions of motor faders. However, there are some encodings of feedback messages which combine different bits of information in a single message, making them difficult or even impossible to translate using the simple kinds of rules we've seen so far. midizap offers a special variation of data mode to help decoding such messages. We call them *mod translations* (a.k.a.\ "modulus" or "modifier" translations), because they involve operations with integer moduli which enable you to both calculate output from input values in a direct fashion, *and* modify the output messages themselves along the way.

One important task, which we'll use as a running example below, is the decoding of meter (RMS level) data in the Mackie protocol. There, each meter value is represented as a channel pressure message whose value consists of a mixer channel index 0..7 in the "high nibble" (bits 4..6) and the corresponding meter value in the "low nibble" (bits 0..3). We will show how to map these values to notes indicating buttons on the AKAI APCmini (please check examples/APCmini.midizaprc in the sources for details about this device). Mod translations aren't limited to this specific use case, however; similar rules will apply to other kinds of "scrambled" MIDI data.

In its simplest form, the translation looks as follows:

~~~
CP[16] C0
~~~

In contrast to standard data translations, there's no increment flag here, so the translation does *not* indicate an incremental change of the input value. Instead, mod translations always work with *absolute* values, and the step size on the left-hand side is treated as a *modulus* to decompose the input value into two separate quantities, *quotient* and *remainder*. Only the latter becomes the value of the output message, while the former is used as an *offset* to modify the output message. (Note that `CP` and `PB` messages don't have a modifiable offset, so if you use these on the output side of a mod translation, the offset part of the input value will be ignored. `PC` messages, on the other hand, lack the parameter value, so in this case the remainder value will be disregarded instead.)

In order to describe more precisely how this works, let's assume an input value *v* and a modulus *k*. We divide *v* by *k*, yielding the quotient (offset) *q* = *v* div *k* and the remainder (value) *r* =  *v* mod *k*. E.g., with *k* = 16 and *v* = 21, you'll get *q* = 1 and *r* = 5 (21 divided by 16 yields 1 with a remainder of 5). The calculated offset *q* is then applied to the note itself, and the remainder *r* becomes the velocity of that note. So in the example above the output would be the note `C#0` (`C0` offset by 1) with a velocity of 5. On the APCmini, this message will light up the second button in the bottom row of the 8x8 grid in yellow.

This transformation is surprisingly versatile, and there are some extensions of the MIDI syntax which make it even more flexible. These extensions are only available in mod translations. They are described by the following grammar rules (please also refer to Section *MIDI Message Notation* for the rest of the grammar rules for the MIDI syntax):

~~~
token ::= msg [ mod ] [ steps ] [ "-" number] [ flag ]
mod   ::= "[" [ number ] "]"
steps ::= "[" number "]" | "{" list "}"
list  ::= number { "," number | ":" number | "-" number }
flag  ::= "'" | "?" | "'?" | "?'"
~~~

There are a couple of new elements in the syntax: an empty modulus bracket `[]`, the transposition flag `'`, the change flag `?`, and lists of numbers enclosed in curly braces. They have the following meaning:

- The *empty modulus* bracket, denoted `[]` on the left-hand side of a mod translation, indicates a default modulus large enough (16384 for `PB`, 128 for other messages) so that the offset *q* always becomes zero and the translation passes on the entire input value as is.

- The *transposition* flag, denoted with the `'` (apostrophe) suffix on an output message, reverses the roles of *q* and *r*, so that the remainder becomes the offset and the quotient the value of the output message.

- The *change* flag, denoted with the `?` suffix on an output message, only outputs the message if there are any changes in offset or value.

- *Value lists*, denoted as lists of numbers separated by commas and enclosed in curly braces, provide a way to describe *discrete mappings* of input to output values. The input value is used as an index into the list to give the corresponding output value, and the last value in the list will be used for any index which runs past the end of the list. There are also some convenient shortcuts which let you construct these lists more easily: repetition *a*`:`*b* (denoting *b* consecutive *a*'s) and enumeration *a*`-`*b* (denoting *a*`,`*a*±1`,`...`,`*b*, which ramps either up or down depending on whether *a*<=*b* or *a*>*b*, respectively).

These are often used in concert. We will introduce value lists in a moment, and cover the other options in due course.

**NOTE:** In the context of mod translations, pitch bend values are interpreted as *unsigned* quantities in the range 0..16383 (with 8192 denoting the center value), which corresponds to the way they are actually encoded in MIDI. This makes the modular arithmetic work consistently across all types of MIDI messages, and also facilitates conversions between the different types of absolute parameter values. Normally you shouldn't have to worry about this, but the change in representation needs to be taken into account when transforming pitch bend values with value lists.
 
Let's return to our example. As usual in data translations, you can also specify a step size on the right-hand side to upscale the output value (which is the remainder *r* here):

~~~
CP[16] C0[2]
~~~

More complicated transformations can be specified as value lists instead. E.g., the APCmini uses the velocities 0, 1, 3 and 5 to denote "off" and the colors green, red and yellow, respectively, so we can map the meter values to different colors as follows:

~~~
CP[16] C0{0,1,1,1,1,1,1,1,1,5,5,5,3}
~~~

Using the shorthand for repetitions, this can be written more succinctly (which also helps readability):

~~~
CP[16] C0{0,1:8,5:3,3}
~~~

Thus 0 will be mapped to 0 (off), 1..8 to 1 (green), 9..11 to 5 (yellow), and 12 or more to 3 (red). (These values appear to be in line with the MCP feedback spit out by Ardour, and what little technical documentation about the Mackie protocol is available on the web. Your mileage may vary, though, so you may have to experiment with these values to make this rule work with your DAW.)

The quotient here is the mixer channel index in the high-nibble of the `CP` message, which will be used as an offset for the `C0` note on the right, so the above rule shows the meters as a single row of colored buttons at the bottom of the 8x8 grid on the APCmini (first value on `C0`, second value on `C#0`, etc.). To get a different layout, you can also scale the *offset* value, by adding a second step size to the left-hand side:

~~~
CP[16][8] C0{0,1:8,5:3,3}
~~~

Even though the extra step size appears on the left-hand side of a mod translation, it is applied to all messages on the *right-hand side*, scaling *up* the offsets of all output messages by the same factor. Thus, in the example above, the buttons are now in the first *column* of the grid (first value on `C0`, second value on `G#0`, etc.). Instead of a single step size, it's also possible to specify a list of discrete offset values, so that you can achieve any regular or irregular output pattern that you want. E.g., the following rule places every other meter value in the second row:

~~~
CP[16]{0,9,2,11,4,13,6,15} C0{0,1:8,5:3,3}
~~~

You might also output several notes at once, in order to display a horizontal or vertical meter *strip* instead of just a single colored button for each value. For instance:

~~~
CP[16] C0{0,1} G#0{0:8,5} E1{0:11,3}
~~~

Note that each of the output notes will be offset by the same amount, so that the green, yellow and red buttons will always be lined up vertically in this example. (The APCmini.midizaprc example actually uses a very similar scheme mapping the meter values to the five topmost button rows.)

Another example from the Mackie protocol is time feedback. The following rule (also from the APCmini.midizaprc example) decodes the least significant digit of the beat number in the time display (`CC69`) to count off time on some of the scene launch buttons of the APCmini. Note that the digits are actually encoded in ASCII, hence the copious amount of initial zeros in the value lists below with which we skip over all the non-digit characters at the beginning of the ASCII table.

~~~
CC69[] F7{0:49,1,0} E7{0:50,1,0} Eb7{0:51,1,0} D7{0:52,1,0}
~~~

Also note the use of an empty modulus bracket on the left-hand side, which means that we always have a zero offset here and thus the output notes aren't modified in this example.

## Basic Mod Translations

While mod translations are often employed for MIDI feedback, they can also be used as an alternative to "ordinary" (incremental) data translations in various contexts. We discuss some of these use cases below and show how they're implemented.

For more basic uses we often want to simply pass on (and possibly transform) the input value *without* using an offset, so we employ a default modulus in most of the following examples to ensure that the offset becomes zero and thus inconsequential. We also call such translations *basic* mod translations. They are useful, in particular, if we want to simply preserve the parameter value in a data translation. For instance:

~~~
CC1[] C5
~~~

This translates the `CC1` (modulation wheel) controller to  a `C5` (middle C) note message in such a way that the controller value becomes the velocity of the note. Note that this is different from both the key translation `CC1 C5` (which only preserves the "on"/"off" status but looses the actual parameter value) and the incremental data translation `CC1= C5` (which usually preserves the value, but executes the translation in a step-wise fashion). A mod translation always maps messages in a single step, which reproduces leaps in the input values on the output side (and, as a side effect, also reduces the amount of data traffic compared to incremental data translations). While key and incremental data translations are tailored to key and mouse output, for pure MIDI bindings like the one above a basic mod translation is often preferable.

You also need to use a mod translation if your binding involves discrete value lists, because these are not available in other kinds of translations. Value lists can represent *any* discrete mapping from input to output values, and thus offer much more flexibility than simple step sizes. For instance, here's how to map controller values to the first few Fibonacci numbers:

~~~
CC1[] CC1{0,1,1,2,3,5,8,13,21,34,55,89}
~~~

Value lists offer some conveniences to facilitate their use, *repetitions* (which we've already discussed above) and *enumerations*. Enumerations are used to denote an ascending or descending range of values. E.g., to reverse the values of a controller you may write:

~~~
CC1[] CC1{127-0}
~~~

Contrast this with the *incremental* reversed controller rule that we've seen earlier:

~~~
CC1= CC1[-1]
~~~

As mentioned, this rule requires that you first move the controller up to its maximum position to make it work. The corresponding mod translation uses absolute values and thus doesn't have this defect; it just works without jumping through any such hoops.

The values in a list may be in any order, and you can throw in any combination of singleton values, enumerations and repetitions. For instance:

~~~
CC1[] CC1{0:2-5,7:5-0}
~~~

The value list in this example starts with two zeros, then ramps up to 5 followed by five 7s, before finally fading back to 0. It goes without saying that this is much easier to read and also much less error-prone to write than `{0,0,1,2,3,4,5,7,7,7,7,7,6,5,4,3,2,1,0}`.

Values in lists may also be negative. In general, if mapping any input value using a value list results in an output value that is out of range for the type of MIDI message at hand, the message will be silently ignored. We can use this, e.g., to suppress note-off messages in the output:

~~~
C0[] C0{-1,1-127}
~~~

This rule will translate a zero velocity to -1, which isn't in the valid range, so the output message will be dropped. For all other velocities, the input message will be output unchanged, because the `1-127` enumeration maps the positive velocities to themselves. If you'd rather output a key-style fixed velocity for the note-ons instead, you can do that as follows:

~~~
C0[] C0{-1,127}
~~~

This translation may look a bit odd, but can be useful at times if the application interprets note inputs, e.g., as radio or toggle buttons, and may get confused by note-off messages. Note that it's impossible to do this kind of mapping with key or incremental data translations, because these don't allow you to suppress the note-off messages.

Last but not least, you can also use a modulus of 1 to cancel the *remainder* instead, if you want to use the input value solely as an offset. For instance, here's how you can map controller values to note *numbers* (rather than velocities):

~~~
CC1[1] C0
~~~

This outputs the note with the same number as the controller value, `C0` for value 0, `C#0` for value 1, `D0` for value 2, etc. In fact, this is just a basic mod translation in disguise, because employing the `'` flag on the output message to transpose quotient and remainder, we can also write it as:

~~~
CC1[] C0'
~~~

Note that the quotient, which becomes the velocity of the output note, will always be zero here, so the above translation turns all notes off. To get a nonzero velocity, you specify it in a value list:

~~~
CC2[] C0{127}'
~~~

## Extracting Sub-Values

Another important idiom is the following, which extracts the low nibble from a controller value. To these ends, we use a modulus of 16 and force the offset value to zero:

~~~
CC1[16]{0} CC1
~~~

Extracting the *high* nibble is just as easy (this is another case where the transposition flag comes in handy):

~~~
CC1[16]{0} CC2'
~~~

Note that this works because the output mapping `{0}` (which forces the offset to 0) is in fact applied *after* the transposition. Thus the quotient becomes the value of the `CC2` message, while the remainder is canceled out and becomes a zero offset. You can also output *both* the low and high nibbles at the same time that way:

~~~
CC1[16]{0} CC1 CC2'
~~~

Using similar rules, you can extract almost any part of an input value, down to every single bit if needed (see the *Macro Translations* section below for another example).

## Detecting Changes

Let's have another look at the high-nibble extraction rule from above:

~~~
CC1[16]{0} CC2'
~~~

Note that if the input value changes gradually then many output values will be identical. E.g., if the input values are 0, 10, 19, 32, 64 then the first four high nibbles are all zero, so the output will be 0, 0, 0, 0, 1, with the zero value repeated four times. If this is not desired, you can add the `?` flag to indicate that the message should be output only if the value has changed:

~~~
CC1[16]{0} CC2'?
~~~

Now, repeated values are suppressed, so with the same inputs the output will be just 0, 1. Note that we used the `?` flag in combination with transposition and the offset forced to zero here, but of course it will work for any kind of change (offset or value, transposed or not). Also note that change detection always considers the "post-transform" values as they would be output, i.e., changes are detected *after* transposition and all mappings of input and output values have been performed.

Change detection is often useful when input values are projected, as in the above example, but also in many other situations in which you simply want to prevent repeated values. For instance, suppose that we'd like to turn the modulation wheel (`CC1`) into a kind of on/off switch. Using a basic mod translation with a value list and change detection, this can be done quite easily:

~~~
CC1[] CC1{0,127}?
~~~

This emits a single 127 value as soon as the input value becomes nonzero, and a single 0 value when it drops to zero again. Note that without the `?` flag, the 127 value might be repeated any number of times while you keep turning the modulation wheel, which isn't the behavior we want here.

## Macro Translations

There are some situations in which it is hard or even impossible to construct a translation in a single step, but it may become much easier if we can recursively invoke other translations. midizap allows you to do this by "calling" the mod translation for a MIDI message on the right-hand side of a translation. This is done by prefixing the message to be expanded with the `$` character:

~~~
CC0[] $CC1
~~~

Note that you can *only* call mod translations this way, so the message to be expanded (`CC1` in this example) must be bound in a mod translation somewhere; otherwise you'll get a warning about the message being undefined and no output will be generated. On the other hand, the translation *containing* the call may also be a key or incremental data translation instead, so we might just as well have written, e.g.:

~~~
CC0= $CC1
~~~

Before we proceed, let's introduce a few terms which will make it easier to talk about these things. We refer to a mod translation being called in this manner as a *macro translation*, and we also call the left-hand side of the translation a *macro*, and the invocation of a macro using the dollar symbol a *macro call*.

To continue our example, let's define the `CC1` macro so that it outputs just a single note message:

~~~
CC0[] $CC1  #1
CC1[] C5    #2
~~~

On a conceptual level, the macro expansion process works pretty much like the production rules of a grammar, with the "dollar" tokens playing the role of the nonterminals. Thus, with the definitions above a `CC0` message will be processed as follows:

- Rule #1 is applied, constructing the output sequence with the `CC1` message as usual.

- Instead of outputting the resulting `CC1` message directly, the program now looks for a mod translation of that message which can be applied recursively.

- Rule #2 is applied, yielding a `C5` message which is substituted for the `$CC1` token in rule #1.

Unsurprisingly, the end result is a `C5` message with the same velocity as the value of the `CC1` message, which in turn comes from the original `CC0` message being translated.

Of course, we could also just have written `CC0[] C5` here and be done with it. So let's try something slightly more complicated which really *needs* recursive translations to work. For instance, suppose that we'd like to output *two* messages instead: the note message `C5` as before, followed by a `CC1` message with just the low nibble of the controller value. Now each of these translations is easy to define:

~~~
CC0[] C5
CC0[16]{0} CC1
~~~

But we can't just put these two rules into the same section, because we're not allowed to bind `CC0` in two different translations at once (if you try this, the parser will complain and just ignore the second rule). And the single rule `CC0[16]{0} C5 CC1` won't work either, because it only passes the low nibble to the `C5` message. However, using a macro call we can massage those rules into an eligible form:

~~~
CC0[] C5 $CC1
CC1[16]{0} CC1
~~~

This does exactly what we set out to do, and the transformation of the original rules we applied here is actually quite straightforward. In the same vein, we can combine as many different mod translations as we like, even if they involve different moduli and offset transformations.

If you know C, you will have realized by now that macro translations work pretty much like macros in the C programming language. The same caveats apply here, too. Specifically, you usually do *not* want to have a macro invoke itself (either directly or indirectly), because that will almost certainly lead to infinite recursion. E.g.:

~~~
CC0[128] $CC1
CC1[128] $CC0 # don't do this!
~~~

midizap *will* catch such mishaps after a few iterations, but it's better to avoid them in the first place. We mention in passing that in theory, recursive macro calls in conjunction with value lists and change detection make the configuration language Turing-complete. However, there's a quite stringent limit on the number of recursive calls, and there are no variables and no iteration constructs, so these facilities aren't really suitable for general-purpose programming.

But there's still a lot of fun to be had with macros despite their limitations. Here's another instructive example which spits out the individual bits of a controller value, using the approach that we discussed earlier in the context of nibble extraction. Input comes from `CC7` in the example, and bit #*i* of the controller value becomes `CC`*i* in the output, where *i* runs from 0 to 6. Note that each of these rules uses a successively smaller power of 2 as modulus and passes on the remainder to the next rule, while transposition is used to extract and output the topmost bit in the quotient.

~~~
CC7[64]{0} $CC6 CC6'
CC6[32]{0} $CC5 CC5'
CC5[16]{0} $CC4 CC4'
CC4[8]{0}  $CC3 CC3'
CC3[4]{0}  $CC2 CC2'
CC2[2]{0}  CC0  CC1'
~~~

You may want to run this example with debugging enabled to see what exactly is going on there.

The "naming" of macros is another issue worth discussing here. In principle, any message which can occur on the left-hand side of a mod translation (i.e., everything but `PC`) can also be used as a macro. Unfortunately, in general you can't be sure which messages might show up in *real* MIDI input. For instance, in the example above the macro translations for `CC2` to `CC6` might also be triggered by real MIDI input instead of macro calls. While this may be useful at times, e.g., for testing purposes, it is most likely going to confuse unsuspecting end users. As a remedy, midizap also provides a special kind of *macro event*, denoted `M0` to `M127`. These "synthetic" messages work exactly like `CC` messages, but they are guaranteed to never occur as real input, and they can *only* be used in macro calls and on the left-hand side of mod translations. We can rewrite the previous example using macro events as follows:

~~~
CC7[64]{0} $M6 CC6'
M6[32]{0}  $M5 CC5'
M5[16]{0}  $M4 CC4'
M4[8]{0}   $M3 CC3'
M3[4]{0}   $M2 CC2'
M2[2]{0}   CC0 CC1'
~~~

Let's conclude with another, slightly more practical example for the use of macros, which turns the pitch wheel of a MIDI keyboard into a simple kind of "shuttle control". To illustrate how this works, let's emit an `XK_Left` key event when the pitch wheel is pushed down, and an `XK_Right` event when it's pushed up. This can be done as follows:

~~~
PB[] $M0{0:8192,1,2}?
M0[] $M1{1,-1} $M2{-1:2,1,-1}
M1[] XK_Left
M2[] XK_Right
~~~

Note that the `M0` macro will be invoked with a value of 0, 1 and 2 if the pitch wheel is down, centered, and up, respectively. Also, we use the change flag here, so the `M0` macro is only invoked when this value actually changes. The value lists in the definition of `M0` are then used to filter these values and call the appropriate macro which handles the key output: `M1` for value 0, `M2` for value 2.

Kicking the pitch wheel around just to move the cursor left and right isn't much fun after a while, but it's easy to adjust the `M1` and `M2` macros for more sensible purposes. E.g., we might output the keyboard shortcuts for "Rewind" and "Fast Forward" for a video editor like Kdenlive or Shotcut:

~~~
M1[] "j"
M2[] "l"
~~~

We could also substitute the corresponding MIDI messages of the Mackie protocol to control a DAW program like Ardour:

~~~
M1[] G7[127]  # Rewind
M2[] G#7[127] # Fast Forward
~~~

# Configuration Language Grammar

The following EBNF grammar summarizes the syntax of the configuration language. The character set is 7 bit ASCII (arbitrary UTF-8 characters are permitted in comments, however). The language is line-oriented; each directive, section header, and translation must be on a separate line. Empty lines and lines containing nothing but whitespace are generally ignored, as are comments, which are introduced with `#` at the beginning of a line or after whitespace, and continue until the end of the line. The only exception are header lines which are always taken verbatim, so whitespace and `#` have no special meaning there.

Section names may contain any character but `]` and newline, regular expressions any character but newline. The latter must follow the usual syntax for extended regular expressions, see regex(7) for details. In a directive or translation line, tokens are delimited by whitespace. Strings are delimited by double quotes and may contain any printable ASCII character except newline and double quotes. Numbers are always decimal integers.

~~~
config      ::= { directive | header | translation }
header      ::= "[" name "]" regex
translation ::= midi-token { key-token | midi-token }

directive   ::= "DEBUG_REGEX" | "DEBUG_STROKES" | "DEBUG_KEYS" |
                "DEBUG_MIDI" | "MIDI_OCTAVE" number |
				"JACK_NAME" string | "JACK_PORTS" number |
				"SYSTEM_PASSTHROUGH" [ number ]

midi-token  ::= msg [ mod ] [ steps ] [ "-" number] [ flag ]
msg         ::= ( note | other | "M" ) [ number ]
note        ::= ( "A" | ... | "G" ) [ "#" | "b" ]
other       ::= "CH" | "PB" | "PC" | "CC" | "CP" | "KP:" note
mod         ::= "[" [ number ] "]"
steps       ::= "[" number "]" | "{" list "}"
list        ::= number { "," number | ":" number | "-" number }
flag        ::= "-" | "+" | "=" | "<" | ">" | "~" |
                "'" | "?" | "'?" | "?'"

key-token   ::= "RELEASE" | "SHIFT" [ number ] |
	            keycode [ "/" keyflag ] | string
keycode     ::= "XK_Button_1" | "XK_Button_2" | "XK_Button_3" |
                "XK_Scroll_Up" | "XK_Scroll_Down" |
                "XK_..." (see /usr/include/X11/keysymdef.h)
keyflag     ::= "U" | "D" | "H"
string      ::= '"' { character } '"'
~~~

# Bugs

There probably are some. Please submit bug reports and pull requests at the midizap [git repository][agraef/midizap]. Contributions are also welcome. In particular, we're looking for interesting configurations to be included in the distribution.

The names of some of the debugging options are rather idiosyncratic. midizap inherited them from Eric Messick's ShuttlePRO program, and we decided to keep them for backward compatibility.

midizap tries to keep things simple, which implies that it has its limitations. In particular, midizap lacks support for translating system messages and some more interesting ways of mapping, filtering and recombining MIDI data right now. There are other, more powerful utilities which do these things, but they are also more complicated and usually require programming skills. Fortunately, midizap often does the job reasonably well for simple mapping tasks (and even some rather complicated ones, such as the APCmini Mackie emulation included in the distribution). But if things start getting too fiddly then you should consider using a more comprehensive tool with real programming capabilities such as [Pd][] instead.

midizap has only been tested on Linux so far, and its keyboard and mouse support is tailored to X11, i.e., it's pretty much tied to Unix/X11 systems right now. Native Mac or Windows support certainly seems possible, but it's not going to happen until someone steps in who's in the know about suitable Mac and Windows replacements for the X11 XTest extension.

# See Also

midizap is based on a [fork][agraef/ShuttlePRO] of Eric Messick's [ShuttlePRO program][nanosyzygy/ShuttlePRO], which provides similar functionality for the Contour Design Shuttle devices.

Spencer Jackson's [osc2midi utility][osc2midi] makes for a great companion to midizap if you also need to convert between MIDI and [Open Sound Control][OSC].

The [Bome MIDI Translator][] seems to be a popular MIDI and keystroke mapping tool for Mac and Windows. It is proprietary software and isn't available for Linux, but it should be worth a look if you need a midizap alternative which runs on these systems.

# Authors, License and Credits

midizap is free and open source software licensed under the GPLv3, please see the LICENSE file in the distribution for details.

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
