% midizap(1)

# Name

midizap -- control your multimedia applications with MIDI

# Synopsis

midizap [-h] [-k] [-o[2]] [-j *name*] [-r *rcfile*] [-d[rskmj]]

# Options

-h
:   Print a short help message.

-k
:   Keep track of key (on/off) status of MIDI notes and control switches. This isn't generally recommended, but may occasionally be useful to deal with quirky controllers sending note- or control-ons without corresponding off messages.

-o[2]
:   Enable MIDI output. Add "2" for a second pair of MIDI ports to be used, e.g., for controller feedback. See Sections *MIDI Output* and *MIDI Feedback*.

-j *name*
:   Set the Jack client name. Default: "midizap". See Section *Jack-Related Options*.

-r *rcfile*
:   Set the configuration file name. Default: taken from the MIDIZAP_CONFIG_FILE environment variable if it exists, or ~/.midizaprc if it exists, /etc/midizaprc otherwise. See Section *Configuration File*.

-d[rskmj]
:   Enable various debugging options: r = regex (print matched translation sections), s = strokes (print the parsed configuration file in a human-readable format), k = keys (print executed translations), m = midi (MIDI monitor, print all recognizable MIDI input), j = jack (additional Jack debugging output). Just `-d` enables all debugging options. See Section *Basic Usage*.

# Description

midizap lets you control your favorite multimedia applications using [MIDI][]. To these ends, it translates Jack MIDI input into X keystrokes, mouse button presses, scroll wheel events, and, as an option, MIDI output. It does this by matching the `WM_CLASS` and `WM_NAME` properties of the window that has the keyboard focus against the regular expressions for each application section in its configuration (midizaprc) file. If a regex matches, the corresponding set of translations is used. If a matching section cannot be found, or if it doesn't define a suitable translation, the program falls back to a set of translations in a default section at the end of the file, if available.

The midizaprc file is just an ordinary text file which you can edit to configure the program. An example.midizaprc file containing sample configurations for some applications is included in the sources. Also, in the examples directory you can find some more examples of configuration files for various purposes.

midizap provides you with a way to hook up just about any MIDI controller to your applications. MIDI controllers are readily supported by all major operating systems, and are often cheaper and more versatile than special-purpose input devices, so they're always an option to consider, especially if you already have one lying around that you'd like to put to good use again. Even if your target application already supports MIDI, midizap's MIDI output option will be useful if your controller can't work directly with the application because of protocol incompatibilities. In particular, you can use midizap to turn pretty much any MIDI controller with enough faders and buttons into a Mackie-compatible mixing device ready to be used with most DAW (digital audio workstation) programs. Another common use case is video editing software, which rarely offers built-in MIDI support. midizap allows you to map the faders, encoders and buttons of your MIDI controller to keyboard commands of your video editor for cutting, marking, playback, scrolling, zooming, etc.

In other words, as long as the target application can be controlled with simple keyboard shortcuts and/or MIDI commands, chances are that midizap can make it work (at least to some extent) with your controller.

# Installation

First, make sure that you have the required dependencies installed. The program needs a few X11 libraries and Jack. And of course you need GNU make and gcc (the GNU C compiler). On Ubuntu and other Debian-based systems you should be able to get everything that's needed by running this command:

    sudo apt install build-essential libx11-dev libxtst-dev libjack-dev

Then just run `make` and `sudo make install`. This installs the example.midizaprc file as /etc/midizaprc, and the midizap program and the manual page in the default install location. Usually this will be under /usr/local, but the installation prefix can be changed with the `prefix` variable in the Makefile. Also, package maintainers can use the `DESTDIR` variable as usual to install into a staging directory for packaging purposes.

# Configuration File

After installation the system-wide default configuration file will be in /etc/midizaprc, where the program will be able to find it. We recommend copying this file to your home directory, renaming it to .midizaprc:

    cp /etc/midizaprc ~/.midizaprc

The ~/.midizaprc file, if it exists, takes priority over /etc/midizaprc, so it becomes your personal default midizap configuration. The midizaprc file included in the distribution is really just an example; you're expected to edit this file to adjust the bindings for the MIDI controllers and the applications that you use. (If you create new configurations which might be useful for others, please consider submitting them so that they can be included in future releases.)

It is also possible to specify the configuration file to be used, by invoking midizap with the `-r` option on the command line, e.g.: `midizap -r myconfig.midizaprc`. This is often used with more specialized configurations dealing with specific applications or MIDI controllers.

The program automatically reloads the midizaprc file whenever it notices that the file has been changed. Thus you can edit the file while the program keeps running, and have the changes take effect immediately without having to restart the program. When working on new translations, you may want to run the program in a terminal, and employ some or all of the debugging options explained below to see exactly how your translations are being processed.

# Basic Usage

The midizap program is a command line application, so you typically run it from the terminal. However, it is also possible to launch it from your Jack session manager (see *Jack-Related Options* below) or from your desktop environment's startup files once you've set up everything to your liking.

Try `midizap -h` for a brief summary of the available options with which the program can be invoked.

midizap uses [Jack][] for doing all its MIDI input and output, so you need to be able to run Jack and connect the Jack MIDI inputs and outputs of the program. While it's possible to do all of that from the command line as well, we recommend using a Jack front-end and patchbay program like [QjackCtl][] to manage Jack and to set up the MIDI connections. In QjackCtl's setup, make sure that you have selected `seq` as the MIDI driver. This exposes the ALSA sequencer ports of your MIDI hardware and other non-Jack ALSA MIDI applications as Jack MIDI ports, so that they can easily be connected to midizap. (Here and in the following, we're assuming that you're using Jack1. Jack2 works in a very similar way, but may require some more fiddling; in particular, you may have to use [a2jmidid][] as a separate ALSA-Jack MIDI bridge in order to have the ALSA MIDI devices show properly as Jack MIDI devices.) 

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

You can try it and test that it works by running `midizap -o`, firing up a MIDI synthesizer such as [FluidSynth][] or its graphical front-end [Qsynth][], and employing QjackCtl to connect its input it to midizap's output port. In the sample configuration, the notes `C4` thru `F4` in the small octave have been set up so that you can operate a little drumkit, and a binding for the volume controller (`CC7`) has been added as well. The relevant portion from the configuration entry looks as follows:

~~~
[MIDI]

 C4    C3-10
 D4    C#3-10
 E4    D3-10
 F4    D#3-10

 CC7=  CC7-10
~~~

Note the `-10` suffix on the output messages in the above example, which indicates that output goes to MIDI channel 10. In midizaprc syntax, MIDI channels are 1-based, so they are numbered 1..16, and 10 denotes the GM (General MIDI) drum channel.

E.g., the input note `C4` is mapped to `C3-10`, the note C in the third MIDI octave, which on channel 10 will produce the sound of a bass drum, at least on GM compatible synthesizers like Fluidsynth. The binding for the volume controller (`CC7`) at the end of the entry sends volume changes to the same drum channel (`CC7-10`), so that you can use the volume control on your keyboard to dial in the volume on the drum channel that you want. The program keeps track of the values of both input and output controllers on all MIDI channels internally, so with the translations above all that happens automagically.

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

Last but not least, midizap also supports Jack session management, which makes it possible to record the options the program was invoked with, along with all the MIDI connections. This feature can be used with any Jack session management software. Specifically, QjackCtl has its own built-in Jack session manager which is available in its Session dialog. To use this, launch midizap and any other Jack applications you want to have in the session, use QjackCtl to set up all the connections as needed, and then hit the "Save" (or "Save and Quit") button in the Session dialog to have the session recorded. Now, at any later time you can relaunch the same session with the "Load" (or "Recent") button in the same dialog.

# Translation Syntax

The midizap configuration file consists of sections defining translation classes. Each section generally looks like this, specifying the name of a translation class, optionally a regular expression to be matched against the window class or title, and a list of translations:

~~~
[name] regex
<A..G><#b><0..12> <output>  # note
KP:<note> <output>          # key pressure (aftertouch)
PC<0..127> <output>         # program change
CC<0..127> <output>         # control change
CP <output>                 # channel pressure
PB <output>                 # pitch bend
~~~

The `#` character at the beginning of a line and after whitespace is special; it indicates that the rest of the line is a comment, which is skipped by the parser. Empty lines and lines containing nothing but whitespace are also ignored.

Each `[name] regex` line introduces the list of MIDI message translations for the named translation class. The name is only used for debugging output, and needn't be unique. When focus is on a window whose class or title matches the regular expression `regex`, the corresponding translations are in effect. An empty regex for the last class will always match, allowing default translations. Any output sequences not bound in a matched section will be loaded from the default section if they are bound there.

The translations define what output should be produced for the given MIDI input. Each translation must be on a line by itself. The left-hand side (first token) of each translation denotes the MIDI message to be translated. MIDI messages are on channel 1 by default; a suffix of the form `-<1..16>` can be used to specify a MIDI channel. E.g., `C3-10` denotes note `C3` on MIDI channel 10.

Note messages are specified using the customary notation (note name `A..G`, optionally followed by an accidental, `#` or `b`, followed by the MIDI octave number). The same notation is used for key pressure (`KP`) messages. Note that all MIDI octaves start at the note C, so `B0` comes before `C1`. By default, `C5` denotes middle C. Enharmonic spellings are equivalent, so, e.g., `D#` and `Eb` denote exactly the same MIDI note.

We will go into most of the other syntactic bits and pieces of MIDI message designations later, but it's good to have the following grammar in EBNF notation handy for reference. (To keep things simple, the grammar is somewhat abridged, but it covers all the frequently used notation. There is some additional syntax for some special forms of translations which will be introduced later.)

~~~
token ::= msg [ "[" number "]" ] [ "-" number ] [ incr ]
msg   ::= ( note | other ) [ number ]
note  ::= ( "A" | ... | "G" ) [ "#" | "b" ]
other ::= "CH" | "PB" | "PC" | "CC" | "CP" | "KP:" note
incr  ::= "-" | "+" | "=" | "<" | ">" | "~"
~~~

Case is ignored here, so `CC`, `cc` or even `Cc` are considered to be exactly the same token by the parser, although by convention we usually write them in uppercase. Numbers are always integers in decimal. The meaning of the first number depends on the context (octave number for notes and key pressure, controller or program number in the range 0..127 for other messages). This can optionally be followed by a number in brackets, denoting a nonzero step size. Also optionally, the suffix with the third number (after the dash) denotes the MIDI channel in the range 1..16; otherwise the default MIDI channel is used (which is always 1 on the left-hand side, but can be set on the right-hand side with `CH`). The optional incr (increment) flag at the end of a token indicates a "data" translation which responds to incremental (up/down) value changes rather than key presses, cf. *Key and Data Input* below.

## Octave Numbering

A note on the octave numbers in MIDI note designations is in order here. There are various different standards for numbering octaves, and different programs use different standards, which can be rather confusing. E.g., there's the ASA (Acoustical Society of America) standard where middle C is C4, also known as "scientific" or "American standard" pitch notation. At least two other standards exist specifically for MIDI octave numbering, one in which middle C is C3 (so the lowest MIDI octave starts at C-2), and zero-based octave numbers, which start at C0 and have middle C at C5. There's not really a single "best" standard here, but the latter tends to appeal to mathematically inclined and computer-savvy people, and is also what is used by default in the midizaprc file.

However, you may want to change this, e.g., if you're working with documentation or MIDI monitoring software which uses a different numbering scheme. To do this, just specify the desired offset for the lowest MIDI octave with the special `MIDI_OCTAVE` directive in the configuration file. For instance:

~~~
MIDI_OCTAVE -1 # ASA pitches (middle C is C4)
~~~

Note that this transposes *all* existing notes in translations following the directive, so if you add this option to an existing configuration, you probably have to edit the note messages in it accordingly.

## Key and Data Input

Input messages can be processed in two different ways, "key mode" and "data mode". Depending on the mode, the extra data payload of the message, which we refer to as the *parameter value* (or just *value* for short), is interpreted in different ways. The parameter value corresponds to the type of MIDI message. Program changes have no value at all. For notes, as well as key and channel pressure messages, it is the velocity value; for control changes, the controller value; and for pitch bend messages, the pitch bend value. Note that the latter is actually a 14 bit value which is considered as a signed quantity in the range -8192..8191, where 0 denotes the center value. In all other cases, the parameter value is an unsigned 7 bit quantity in the range 0..127. (MIDI aficionados will notice that what we call the parameter value here, is actually the second data byte, or, in case of pitch bends, the combined first and second data byte of the MIDI message.)

*Key mode* is the default mode and is available for all message types. In this mode, MIDI messages are considered as keys which can be "pressed" ("on") or "released" ("off"). Any nonzero data value means "pressed", zero "released". Two special cases need to be considered here:

- For pitch bends, any positive *or* negative value means "pressed", while 0 (the center value) means "released".

- Since program changes have no parameter value associated with them, they don't really have an "on" or "off" status. But they are treated in the same key-like fashion anyway, assuming that they are "pressed" and then "released" immediately afterwards.

*Data mode* is available for all messages whose parameter value may continuously change over time, i.e., key and channel pressure, control changes, and pitch bends. In this mode, the actual *amount* of change in the value of the message (increment or decrement, a.k.a. "up" or "down") is processed rather than the on/off state. Data mode is indicated with a special suffix on the message token which indicates the direction of the change which the rule should apply to: increment (`+`), decrement (`-`), or both (`=`).

Data mode usually tracks changes in the *absolute* value of a control. However, for `CC` messages there's also an alternative mode for so-called *incremental* controllers, or *encoders* for short, which can found on some DAW controllers. (These usually take the form of jog wheels or rotary encoders which can be turned endlessly in either direction. In contrast, absolute-valued controllers are usually faders or knobs which are confined to a range between minimum and maximum values.)

Encoders emit a special *sign bit* value indicating a *relative* change, where a value < 64 usually denotes an increment (representing clockwise rotation), and a value > 64 a decrement (counter-clockwise rotation). The actual amount of change is in the lower 6 bits of the value. In the message syntax, these kinds of controls are indicated by using the suffixes `<`, `>` and `~` in lieu of `-`, `+` and `=`, respectively. These suffixes are only permitted with `CC` messages.

Translations must be determined uniquely in each translation class. That is, there must at most be one translation for each MIDI token in each translation section. Note, however, that the MIDI channel is part of the token, so tokens with different MIDI channels count as different messages here. The same goes for the mode of a message (key or data), so messages can have both key and data translations associated with them (which is rarely used in practice, though).

## Keyboard and Mouse Translations

The right-hand side of a translation (i.e., everything following the first token) is a sequence of one or more tokens, separated by whitespace, indicating either MIDI messages or X11 keyboard and mouse events to be output.

Let's look at keyboard and mouse output first. It consists of X key codes with optional up/down indicators, or strings of printable characters enclosed in double quotes. The syntax of these items, as well as the special `RELEASE` and `SHIFT` tokens which will be discussed later, are described by the following grammar:

~~~
token   ::= "RELEASE" | "SHIFT" | keycode [ "/" flag ] | string
keycode ::= "XK_Button_1" | "XK_Button_2" | "XK_Button_3" |
            "XK_Scroll_Up" | "XK_Scroll_Down" |
            "XK_..." (X keysyms, see /usr/include/X11/keysymdef.h)
flag    ::= "U" | "D" | "H"
string  ::= '"' { character } '"'
~~~

Here, case *is* significant (except in character strings, see the remarks below), so the special `RELEASE` and `SHIFT` tokens must be in all caps, and the `XK` symbols need to be written in mixed case exactly as they appear in the /usr/include/X11/keysymdef.h file. Besides the key codes from the keysymdef.h file, there are also some special additional key codes to denote mouse button (`XK_Button_1`, `XK_Button_2`, `XK_Button_3`) and scroll wheel (`XK_Scroll_Up`, `XK_Scroll_Down`) events.

Any keycode can be followed by an optional `/D`, `/U`, or `/H` flag, indicating that the key is just going down (without being released), going up, or going down and being held until the "off" event is received. So, in general, modifier key codes will be followed by `/D`, and precede the keycodes they are intended to modify. If a sequence requires different sets of modifiers for different keycodes, `/U` can be used to release a modifier that was previously pressed with `/D`. Sequences may also have separate press and release sequences, separated by the special word `RELEASE`. Examples:

~~~
C5 "qwer"
D5 XK_Right
E5 XK_Alt_L/D XK_Right
F5 "V" XK_Left XK_Page_Up "v"
G5 XK_Alt_L/D "v" XK_Alt_L/U "x" RELEASE "q"
~~~

One pitfall for beginners is that character strings in double quotes are just a shorthand for the corresponding X key codes, ignoring case. Thus, e.g., `"abc"` actually denotes the keysym sequence `XK_a XK_b XK_c`, as does `"ABC"`. So in either case the *lowercase* string `abc` will be output. To output uppercase letters, it is always necessary to add one of the shift modifiers to the output sequence. E.g., `XK_Shift_L/D "abc"` will output `ABC` in uppercase.

Translations are handled differently depending on the input mode (cf. *Key and Data Input* above). In *key mode*, there are separate press and release sequences. The former is invoked when the input key goes "down" (i.e., when the "on" status is received), the latter when the input key goes "up" again ("off" status). More precisely, at the end of the press sequence, all down keys marked by `/D` will be released, and the last key not marked by `/D`, `/U`, or `/H` will remain pressed. The release sequence will begin by releasing the last held key. If keys are to be pressed as part of the release sequence, then any keys marked with `/D` will be repressed before continuing the sequence. Keycodes marked with `/H` remain held between the press and release sequences. For instance, let's take a look at one of the more conspicuous translations in the example above:

~~~
G5 XK_Alt_L/D "v" XK_Alt_L/U "x" RELEASE "q"
~~~

When the `G5` key is pressed on the MIDI keyboard, the key sequence `Alt+v x` is initiated, keeping the `x` key pressed (so it may start auto-repeating after a while). The program then sits there waiting (possibly executing other translations) until you release the `G5` key again, at which point the `x` key is released and the `q` key is pressed (and released).

In contrast, in *data mode* only a single sequence is output whenever the message value increases or decreases. At the end of the sequence, all down keys will be released. For instance, the following translations move the cursor left or right whenever the volume controller (`CC7`) decreases and increases, respectively. Also, the number of times one of these keys is output corresponds to the actual change in the value. Thus, if in the example `CC7` increases by 4, say, the program will press (and release) `XK_Right` four times, moving the cursor 4 positions to the right.

~~~
CC7- XK_Left
CC7+ XK_Right
~~~

Incremental `CC` messages are treated in an analogous fashion, but in this case the increment or decrement is determined directly by the input message. One example for this type of controller is the jog wheel on the Mackie MCU, which can be processed as follows (using `<` and `>` in lieu of `-` and `+` as the suffix of the `CC` message):

~~~
CC60< XK_Left
CC60> XK_Right
~~~

(The corresponding "bidirectional" translations, which are indicated with the `=` and `~` suffixes, are rarely used with keyboard and mouse translations. Same goes for the special `SHIFT` token. Thus we'll discuss these in later sections, see *MIDI Translations* and *Shift State* below.)

In data mode, input messages can also have a *step size* associated with them, which enables you to scale down changes in the parameter value. The default step size is 1 (no scaling). To change it, the desired step size is written in brackets immediately after the message token to be translated, but before the increment suffix. A step size *k* indicates that the translation is executed whenever the input value has changed by *k* units. For instance, to slow down the cursor movement in the example above by a factor of 4:

~~~
CC7[4]- XK_Left
CC7[4]+ XK_Right
~~~

Note that there is no step size with input messages in (standard) key mode. The message is either just "on" or "off", with any nonzero value denoting off. There is a special variation of key mode which has a step size associated with it, but it functions differently. As these translations are somewhat esoteric and mostly used with MIDI feedback generated by the host application, we'll have a look at them later, see the *MIDI Feedback* section near the end of the manual.

## MIDI Translations

Most of the notation for MIDI messages on the left-hand side of a translation rule also carry over to the output side. The only real difference is that the increment suffixes `+-=<>` aren't permitted here, as they are only used to determine the input mode (key or data) of the entire translation. (The `~` suffix *is* allowed, however, to indicate output in incremental bit-sign format in data translations, see below.)

The output sequence can involve as many MIDI messages as you want, and these can be combined freely with keyboard and mouse events in any order. However, as already discussed in Section *MIDI Output* above, you need to invoke the midizap program with the `-o` option to make MIDI output work. Otherwise, MIDI messages in the output translations will just be silently ignored.

There is one special MIDI token `CH` which can only be used on the output side. The `CH` token, which is followed by a MIDI channel number in the range 1..16, doesn't actually generate any MIDI message, but merely sets the default MIDI channel for subsequent MIDI messages in the same output sequence. This is convenient if multiple messages are output to the same MIDI channel. For instance, the sequence `C5-2 E5-2 G5-2`, which outputs a C major chord on MIDI channel 2, can also be abbreviated as `CH2 C5 E5 G5`.

For key-mode inputs, the corresponding "on" or "off" event is generated for all MIDI messages in the output sequence, where the "on" value defaults to the maximum value (127 for controller values, 8191 for pitch bends). Thus, e.g., the following rule outputs a `CC80` message with controller value 127 each time middle C (`C5`) is pressed (and another `CC80` message with value 0 when the note is released again):

~~~
C5 CC80
~~~

The value for the "on" state can also be denoted explicitly with a step size:

~~~
C5 CC80[64]
~~~

For pitch bends, the step size can also be negative. For instance, the following rules assign two keys to bend down and up by the maximum amount possible:

~~~
C2 PB[-8192] # bend down
D2 PB[8191]  # bend up
~~~

There are two additional suffixes `=` and `~` for data translations which are most useful with pure MIDI translations, which is why we deferred their discussion until now. If the increment and decrement sequences for a given translation are the same, the `=` suffix can be used to indicate that this sequence should be output for *both* increments and decrements. For instance, to map the modulation wheel (`CC1`) to the volume controller (`CC7`):

~~~
CC1= CC7
~~~

Which is exactly the same as the two translations:

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

The `~` suffix can be used to denote incremental controllers in output messages, too. E.g., to translate a standard (absolute) MIDI controller to an incremental encoder value, you might use a rule like:

~~~
CC48= CC16~
~~~

Step sizes also work on the right-hand side of data translations. You might use these to scale up value changes, e.g., when translating from control changes to pitch bends:

~~~
CC1= PB[128]
~~~

The step size can also be negative, which allows you to reverse the direction of a controller if needed. E.g., the following will output values going down from 127 to 0 as input values go up from 0 to 127:

~~~
CC1= CC1[-1]
~~~

## Shift State

The special `SHIFT` token toggles an internal shift state, which can be used to generate alternative output for certain MIDI messages. Please note that, like the `CH` token, the `SHIFT` token doesn't generate any output by itself; it merely toggles the internal shift bit which can then be queried in other translations to distinguish between shifted and unshifted bindings for the same input message.

To these ends, there are two additional prefixes which indicate the shift status in which a translation is active. Unprefixed translations are active only in unshifted state. The `^` prefix denotes a translation which is active only in shifted state, while the `?` prefix indicates a translation which is active in *both* shifted and unshifted state.

Many DAW controllers have some designated shift keys which can be used for this purpose, but the following will actually work with any key-style MIDI message. E.g., to bind the shift key (`A#5`) on a Mackie controller:

~~~
?A#5 SHIFT
~~~

Note the `?` prefix indicating that this translation is active in both unshifted and shifted state, so it is used to turn shift state both on and off, giving a "Caps Lock"-style of toggle key. If you'd rather have an ordinary shift key which turns on shift state when pressed and immediately turns it off when released again, you can do that as follows:

~~~
?A#5 SHIFT RELEASE SHIFT
~~~

Having set up the translation for the shift key itself, we can now indicate that a translation should be valid only in shifted state with the `^` prefix. This makes it possible to assign, depending on the shift state, different functions to buttons and faders. Here's a typical example which maps a control change to either Mackie-style fader values encoded as pitch bends, or incremental encoder values:

~~~
 CC48= PB[128]  # translate to pitch bend when unshifted
^CC48= CC16~    # translate to encoder when shifted
~~~

To keep things simple, only one shift status is available in the present implementation. Also note that when using a shift key in the manner described above, its status is *only* available internally to the midizap program; the host application never gets to see it. If your host software does its own handling of shift keys (as most Mackie-compatible DAWs do), it's usually more convenient to simply pass those keys on to the application. However, `SHIFT` comes in handy if your controller simply doesn't have enough buttons and faders to control all the essential features of your target application. In this case the internal shift feature makes it possible to double the amount of controls available on the device. For instance, you can emulate a Mackie controller with both encoders and faders on a device which only has a single set of faders, by assigning the shifted faders to the encoders, as shown above.

# Advanced Features

This section covers some more advanced functionality which isn't used nearly as often as the basic features discussed in previous sections, but will come in handy in some situations. Specifically, we'll discuss *MIDI feedback*, which is needed to properly implement bidirectional communication with some controllers, as well as a special kind of key translations which helps implementing some types of feedback, but also has its uses in "normal" processing.

## MIDI Feedback

Some MIDI controllers need a more elaborate setup than what we've seen so far, because they have motor faders, LEDs, etc. requiring feedback from the application. To accommodate these, you can use the `-o2` option of midizap, or the `JACK_PORTS 2` directive in the midizaprc file, to create a second pair of MIDI input and output ports, named `midi_in2` and `midi_out2`. Use of this option also activates a second MIDI default section in the midizaprc file, labeled `[MIDI2]`, which is used exclusively for translating MIDI input from the second input port and sending the resulting MIDI output to the second output port. Typically, the translations in the `[MIDI2]` section will be the inverse of those in the `[MIDI]` section, or whatever it takes to translate the MIDI feedback from the application back to MIDI data which the controller understands.

You then wire up midizap's `midi_in` and `midi_out` ports to controller and application as before, but in addition you also connect the application back to midizap's `midi_in2` port, and the `midi_out2` port to the controller. This reverse path is what is needed to translate the feedback from the application and send it back to the controller. A full-blown example for this kind of setup can be found in examples/APCmini.midizaprc in the sources, which shows how to emulate a Mackie controller with AKAI's APCmini device, so that it readily works with DAW software such as Ardour.

## Specialized Key Translations

Most of the time, MIDI feedback uses just the standard kinds of MIDI messages readily supported by midizap, such as note messages which make buttons light up in different colors, or control change messages which set the positions of motor faders. However, there are some encodings of MIDI messages employed in feedback, such as time and meter displays, which combine different bits of information in a single message, making them difficult or even impossible to translate using the simple kinds of rules we've seen so far.

midizap offers a special variation of key mode to help decoding at least some of these special messages. For reasons which will become obvious in a moment, we also call these *mod key translations*, or just *mod translations* for short. The extended MIDI syntax being used here is described by the following grammar rules (please refer to the beginning of Section *Translation Syntax* for the parts of the syntax not explicitly defined here):

~~~
token ::= msg [ steps ] [ "-" number]
steps ::= "[" list "]" | "[" number "]" "[" list "]"
list  ::= number { "," number | ":" number }
~~~

To explain the meaning of these translations, we take the mapping of channel pressure to notes as a running example here. But the same works with any kind of message having a parameter value (i.e., anything but `PC`). In its most basic form, the translation looks as follows:

~~~
CP[16] C0
~~~

This looks like a key translation with a step size, but is treated differently. Firstly, there are no separate press and release sequences like in other key translations, only a single output sequence. Thus, like in data mode, all down keys will be released immediately (there aren't any in this example, but in general, all kinds of output events are allowed, like in other translations).

Secondly, there's no "on"/"off" status here to determine the output values either. Rather, the output messages are constructed directly from the input value by some arithmetic calculations. To these ends, the step size is actually being used as a *modulus* in order to decompose the input value into two separate quantities, *quotient* and *remainder*. Only the latter becomes the value of the output message, while the former is used as an *offset* to modify the output message.

More precisely, the input value *v*, say, is divided by the given step size *k*, yielding the offset *p* = *v/k* (rounded to the nearest integer towards zero), and the remainder *q* of that division. E.g., with *k* = 16 and *v* = 21, you'll get *p* = 1 and *q* = 5 (21 divided by 16 yields 1 with remainder 5, because 16×1 + 5 = 21). The offset *p* is then applied to the note itself, and the remainder *q* becomes the velocity of that note. So in the example the output would be the note `C#0` (`C0` offset by 1) with a velocity of 5.

(Note that `CP` and `PB` messages don't have a modifiable offset, so if you use these on the output side of a mod translation, the offset part of the input value will be ignored. The same goes for the `PC` message, which doesn't have a parameter value either, so that the remainder value will be ignored, too.)
 
The above might seem a bit esoteric, but this is in fact exactly how you decode meter information in the Mackie control protocol, which consists of a mixer channel index 0..7 in the high nibble (bits 4..6) and the corresponding meter value in the low nibble (bits 0..3) of a key pressure message, which is why we used 16 as the modulus in this example. There are also some variations of the syntax which make this kind of translation more flexible. In particular, on the right-hand side of the rule you can specify a step size if the remainder value needs to be scaled:

~~~
CP[16] C0[2]
~~~

Or you can specify a *list* of discrete velocity values instead. E.g., the APCmini uses the velocities 1, 3 and 5 to denote the colors green, red and yellow, respectively, and a zero velocity denotes "off", so you can map the meter value to different colors as follows:

~~~
CP[16] C0[0,1,1,1,1,5,5,5,3]
~~~

The remainder of the input value will then be used as an index into the list to give the translated value. E.g., in our example 0 will be mapped to 0 (off), 1..4 to 1 (green), 5..7 to 5 (yellow), and 8 to 3 (red). Also, the last value in the list will be used for any index which runs past the end of the list. Thus in the example, if for some reason you'd receive a meter value of 10, say, the output will still be 3, since it's the last value in the list.

Note that there are a lot of repeated values in this example. For convenience, it's possible to abbreviate these using the notation *value*`:`*count*, which also helps readability. The following denotes exactly the same list as above:

~~~
CP[16] C0[0,1:4,5:3,3]
~~~

You can also scale the *offset* value, by adding a second step size to the left-hand side:

~~~
CP[16][8] C0[0,1:4,5:3,3]
~~~

Now, a channel pressure value of 24 (denoting a meter value of 8 on the second mixer channel) will output the note `A0` (`C0` offset by 8) with velocity 3, which on the APCmini will light up the first LED in the second row in red. Instead of a single step size, it's also possible to specify a list of discrete offset values, so that you can achieve any regular or irregular output pattern that you like:

~~~
CP[16][1,8,17,24] C0[0,1:4,5:3,3]
~~~

You might also output several notes at once, to show a horizontal or vertical strip of LEDs for each mixer channel. For instance, suppose that we'd like to use an LED in the bottom row of the APCmini for the green, and the LEDs in the two rows above it for the yellow and red values, respectively. You can do that as follows:

~~~
CP[16] C0[0,1] G#0[0:5,5] E1[0:8,3]
~~~

Note that in this case each of the output notes will be offset by the same amount, so that an input value of 24 will cause the second LED in the bottom row to light up in green, and the two LEDs above it in yellow and red, respectively. For more examples, please have a look at the APCmini.midizaprc file in the sources which has a collection of similar rules to implement the meter display.

It goes without saying that this is not a universal solution, but it covers at least one important real-world use case, and should work with almost any other kind of "scrambled" MIDI feedback which packs two separate values together. Although we try to keep midizap lean and mean, we might add some more special-case key translations like these in the future, as the need arises.

## Other Uses

Mod translations work with all kinds of output, so that you can also output X11 key and mouse events along with the transformed MIDI data if needed. While mod translations are most commonly employed for MIDI feedback, they also have their uses in ordinary (forward) translations. The input message may be anything which has a parameter value, i.e., any MIDI message but `PC`, and you can choose the modulus large enough (8192 for `PB`, 128 for other messages) so that the offset is always zero, if you just want to employ the discrete value lists for your translations. These offer a great deal of flexibility, much more than can be achieved with simple step sizes. In fact, they can be used to realize *any* conceivable mapping between input and output values. For instance, suppose that we'd like to map controller values to the the first few Fibonacci numbers:

~~~
CC1[128] CC1[0,1,1,2,3,5,8,13,21,34,55,89]
~~~

The output values don't have to be increasing either; they might be in any order you like:

~~~
CC1[128] CC1[0,2,1,4,3,6,5,8,7,10,9,...]
~~~

You can also use a modulus of 1 if you'd like to map, say, controller values to note *numbers* (rather than velocities):

~~~
CC1[1] C0
~~~

This will output the note with the same number as the controller value, `C0` for value 0, `C#0` for value 1, etc. Note that the remainder value, which becomes the velocity of the output note, will always be zero here, so the above translation turns all notes off. If we want a nonzero velocity, we have to specify it in a value list:

~~~
CC2[1] C0[127:1]
~~~

Now we can turn notes on with `CC2` and turn them off again with `CC1`. Note the little bit of trickery there on the right-hand side. Just `[127]` would be interpreted as a simple step size, which wouldn't do us much good here since the remainder value to be scaled is always zero. Thus we need to write `[127:1]` instead to make sure that the parser properly recognizes this as a value list. The list we used here will map any input to 127, which is exactly what we want here.

For the sake of a more practical example, let's have another look at MIDI feedback in the Mackie protocol. The following rule decodes the lowest digit in the time display (`CC69`) to count off time on the 4 bottommost scene launch buttons on the AKAI APCmini. Note that the digits are actually encoded in ASCII, therefore the copious amount of initial zeros in the value lists below to skip over all the non-digit characters at the beginning of the ASCII table.

~~~
CC69[128] F7[0:49,1,0] E7[0:50,1,0] Eb7[0:51,1,0] D7[0:52,1,0]
~~~

As you can see, mod key translations in combination with discrete value lists are really very powerful and let you implement pretty much any desired 1-1 mapping with ease. There *are* some limitations, though. In particular, mappings involving multiple different translations of the same input aren't possible right now, because translations must be unique. Also, there's no way to combine the values of several input messages into a single output message.

# Bugs

There probably are some. Please submit bug reports and pull requests at the midizap [git repository][agraef/midizap]. Contributions are also welcome. In particular, we're looking for interesting configurations to be included in the distribution.

The names of some of the debugging options are rather peculiar. midizap inherited them from Eric Messick's ShuttlePRO program on which midizap is based, so they'll probably last until someone comes up with something better.

There's no Mac or Windows support (yet). midizap has only been tested on Linux so far, and its keyboard and mouse support is tailored to X11, i.e., it's pretty much tied to Unix/X11 systems right now.

midizap tries to keep things simple, which implies that it has its limitations. In particular, system messages are not supported right now, and midizap lacks some more interesting ways of mapping, filtering and recombining MIDI data. There are other, more powerful utilities which do these things, but they are also more complicated and usually require at least some programming skills. midizap often does the job reasonably well for simple mapping tasks, but if things start getting fiddly then you should consider using a more comprehensive tool like [Pd][] instead.

# See Also

midizap is based on a [fork][agraef/ShuttlePRO] of Eric Messick's [ShuttlePRO program][nanosyzygy/ShuttlePRO], which provides pretty much the same functionality for the Contour Design Shuttle devices.

Spencer Jackson's [osc2midi][] utility makes for a great companion to midizap if you also need to convert between MIDI and [Open Sound Control][OSC].

The [Bome MIDI Translator][] seems to be a popular MIDI and keystroke mapping tool for Mac and Windows. It is proprietary software and isn't available for Linux, but it should be worth a look if you need a midizap alternative which runs on these systems.

# Authors

midizap is free and open source software licensed under the GPLv3, please check the accompanying LICENSE file for details.

Copyright 2013 Eric Messick (FixedImagePhoto.com/Contact)  
Copyright 2018 Albert Graef (<aggraef@gmail.com>)

This is a version of Eric Messick's ShuttlePRO program which has been redesigned to work with Jack MIDI instead of the Contour Design Shuttle devices. ShuttlePRO was written in 2013 by Eric Messick, based on earlier code by Trammell Hudson and Arendt David. The MIDI support was added by Albert Graef. All the key and mouse translation features of the original program still work as before, but it goes without saying that the configuration language and the translation code have undergone some substantial changes to accommodate the MIDI input and output facilities. The Jack MIDI backend is based on code from Spencer Jackson's osc2midi utility, and on the simple_session_client.c example available in the Jack git repository.

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
