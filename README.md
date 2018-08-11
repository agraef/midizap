# midizap

Copyright 2013 Eric Messick (FixedImagePhoto.com/Contact)  
Copyright 2018 Albert Graef <<aggraef@gmail.com>>

This is a version of Eric Messick's [ShuttlePRO][nanosyzygy/ShuttlePRO] program which has been redesigned to use Jack MIDI input instead of the Contour Design Shuttle devices that ShuttlePRO was written for.

ShuttlePRO was originally written in 2013 by Eric Messick. This version of the program is based on Albert Graef's [fork][agraef/ShuttlePRO] of the program, so it has all of the translation features of the original program, but also offers Jack MIDI support and various other useful improvements, such as additional command line options and the ability to detect applications using their `WM_CLASS` property (in addition to window titles).

midizap provides you with a way to hook up just about any MIDI controller and use it to translate MIDI input to X keyboard and mouse events in order to control your favorite multimedia applications, such as audio and video editors, digital audio workstation (DAW) programs and the like. Moreover, midizap can also be used to output translated MIDI data, which is useful, e.g., if the target application supports MIDI, but can't work directly with your controller because of protocol incompatibilities. In other words, as long as the target application can be controlled with simple keyboard shortcuts and/or MIDI commands, `midizap` should be able to make it work with your controller.

[nanosyzygy/ShuttlePRO]: https://github.com/nanosyzygy/ShuttlePRO
[agraef/ShuttlePRO]: https://github.com/agraef/ShuttlePRO

## Description

The `midizap` program translates Jack MIDI input into X keystrokes, mouse button presses, scroll wheel events, or, as an option, MIDI output. It does this by matching the (name and class of the) focused window against the regular expressions for each application section in its configuration (midizaprc) file, and using a matching set of translations if available. Default translations for applications not explicitly matching any of the sections in the configuration file can be given as well.

By these means incoming MIDI messages can be translated to sequences of multiple mouse actions and keystrokes, including the pressing and releasing of modifier keys. In addition, MIDI messages can be generated and output using Jack MIDI.

The midizaprc file is just an ordinary text file which you can edit to configure the program for use with any kind of application taking keyboard, mouse or MIDI input. An example.midizaprc file containing sample configurations for some  applications is included in the sources.

## Installation

First, make sure that you have the required dependencies installed. The program needs a few X11 libraries and Jack. And of course you need GNU make and gcc (the GNU C compiler). On Ubuntu and other Debian-based systems you should be able to get everything that's needed by running this command:

    sudo apt install build-essential libx11-dev libxtst-dev libjack-dev

Then just run `make` and `sudo make install`. This installs the example.midizaprc file as /etc/midizaprc, and the `midizap` program in the default install location. Usually this will be /usr/local/bin, but the installation prefix can be changed with the `prefix` variable in the Makefile. Also, package maintainers can use the `DESTDIR` variable as usual to install into a staging directory for packaging purposes.

## Configuration File

After installation the system-wide default configuration file will be in /etc/midizaprc, where the program will be able to find it. We recommend copying this file to your home directory, renaming it to .midizaprc:

    cp /etc/midizaprc ~/.midizaprc

The ~/.midizaprc file, if it exists, takes priority over /etc/midizaprc, so it becomes your personal `midizap` configuration. You can edit this file as you see fit, in order to customize existing or add your own application configurations for the MIDI controller that you have. (If you create any new entries which might be useful to other users of this program, please consider submitting them so that they can be included in future releases.)

**NOTE:** The program automatically reloads the midizaprc file whenever it notices that the file has been changed. Thus you can edit the file while the program keeps running, and have the changes take effect immediately without having to restart the program. When working on new translations, you may want to run the program in a terminal, and employ some or all of the debugging options explained below to see exactly how your translations are being processed.

## Basic Usage

The `midizap` program is a command line application, so you typically run it from the terminal, but of course it is also possible to invoke it from your desktop environment's startup files once you've set up everything to your liking.

`midizap` uses [Jack][] for doing all its MIDI input and output, so you need to be able to run Jack and connect the Jack MIDI inputs and outputs of the program. While it's possible to do all of that from the command line as well, we recommend using a Jack front-end and patchbay program like [QjackCtl][] to manage Jack and to set up the MIDI connections. In QJackCtl's setup, make sure that you have selected `seq` as the MIDI driver. This exposes the ALSA sequencer ports of your MIDI hardware and other non-Jack ALSA MIDI applications as Jack MIDI ports, so that they can easily be connected using QjackCtl.

(Here and in the following, we're assuming that you're using Jack1. Jack2 works in a very similar way, but may require some more fiddling; in particular, you may have to use [a2jmidid][] as a separate ALSA-Jack MIDI bridge in order to have the ALSA MIDI devices show properly as Jack MIDI devices.) 

[Jack]: http://jackaudio.org/
[QjackCtl]: https://qjackctl.sourceforge.io/
[a2jmidid]: http://repo.or.cz/a2jmidid.git

Having that set up, start Jack, make sure that your MIDI controller is connected, and try running `midizap` from the command line (without any arguments). In QJackCtl, open the Connections dialog and activate the second tab named "MIDI", which shows all available Jack MIDI inputs and outputs. On the right side of the MIDI tab, you should now see a client named `midizap` with one MIDI input port named `midi_in`. That's the one you need to connect to your MIDI controller, whose output port should be visible under the `alsa_midi` client on the left side of the dialog.

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

It should be fairly obvious that these translations map the white keys of the middle octave (MIDI notes `C5` thru `B5`) to some mouse buttons and cursor commands. Switch the keyboard focus to some window with text in it, such as a terminal or an editor window. Pressing the keys C, D and E should click the mouse buttons, while F thru B should perform various cursor movements. Also, moving the modulation wheel (`CC1`) on your keyboard should scroll the window contents up and down.

One useful feature is that you can invoke the program with various debugging options to get more verbose output as the program recognizes events from the device and translates them to corresponding mouse actions or key presses. E.g., try running `midizap -drk` to have the program print the recognized configuration sections and translations as they are executed. Now press some of the keys and move the modulation wheel. You should see something like:

~~~
$ midizap -drk
Loading configuration: /home/user/.midizaprc
translation: Default for emacs@hostname (class emacs)
CC1-1-[]: XK_Scroll_Down/D XK_Scroll_Down/U 
CC1-1-[]: XK_Scroll_Down/D XK_Scroll_Down/U 
G5-1[D]: XK_Up/D 
G5-1[U]: XK_Up/U 
A5-1[D]: XK_Down/D 
A5-1[U]: XK_Down/U 
~~~

It goes without saying that these debugging options will be very helpful when you start developing your own bindings. The `-d` option can be combined with various option characters to choose exactly which kinds of debugging output you want; `r` ("regex") prints the matched translation section along with the window name and class of the focused window; `s` ("strokes") prints the parsed contents of the configuration file in a human-readable form whenever the file is loaded; `k` ("keys") shows the recognized translations as the program executes them, in the same format as `s`; and `j` adds some debugging output from the Jack driver. You can also just use `-d` to enable all debugging output.

It's also possible to use alternative configuration files, by specifying the midizaprc file to be used with the `-r` option. Also, try `midizap -h` which prints a short help message with the available options and a brief description.

Most of the other translations in the distributed midizaprc file assume a Mackie-like device with standard playback controls and a jog wheel. Any standard DAW controller which can be switched into Mackie mode should work with these. Otherwise, you'll have to edit the configuration file to make them work.

More information about the available configurations and on how to actually create your own configurations can be found in the example.midizaprc file. This also contains a brief explanation of the syntax used to denote the MIDI messages to be translated. You may also want to look at the comments at the top of readconfig.c for further technical details.

## MIDI Output

As already mentioned, the `midizap` program can also be used to translate MIDI input to MIDI output. To these ends, MIDI messages can be translated to sequences of other MIDI messages.

You enable MIDI output by running the program as `midizap -o`. This will equip the `midizap` Jack MIDI client with an additional output port named `midi_out` (visible on the left side of QJackCtl's Connection window). 

The example.midizaprc file comes with a sample configuration in the special `[MIDI]` default section for illustration purposes. This section is only active if the program is run with the `-o` option. It allows MIDI output to be sent to any connected applications, no matter which window currently has the keyboard focus. This is probably the most common way to use this feature, but of course it is also possible to have application-specific MIDI translations, in the same way as with X11 key bindings. In fact, you can freely mix mouse actions, key presses and MIDI messages in all translations.

You can try it and test that it works by running `midizap -o`, firing up a MIDI synthesizer such as [FluidSynth][] or its graphical front-end [Qsynth][], and employing QjackCtl to connect its input it to midizap's output port. In the sample configuration, the notes `C4` thru `F4` in the small octave have been set up so that they play some MIDI notes, and the modulation wheel (`CC1`) can be used as a MIDI volume controller (`CC7`). The relevant excerpt from the configuration entry looks as follows:

[FluidSynth]: http://www.fluidsynth.org/
[Qsynth]: https://qsynth.sourceforge.io/

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

A note on the MIDI octave numbers is in order here. There are various different standards for numbering octaves, and different programs use different standards, which can be rather confusing. E.g., there's the ASA (Acoustical Society of America) standard where middle C is C4, also known as "scientific" or "American standard" pitch notation. At least two standards exist for MIDI octave numbering, one in which middle C is C3 (so the lowest MIDI octave starts at C-2), and zero-based octave numbers, which start at C0 and have middle C at C5. There's not really a single "best" standard here, but the latter tends to appeal to mathematically inclined and computer-savvy people, and is also what is used by default in the midizaprc file.

However, you can easily change this with a special `MIDI_OCTAVE` directive in the configuration file, please check example.midizaprc for details. Also, if you use some MIDI monitoring software to figure out which notes to put into your midizaprc file, first check how the program prints middle C, so that you know how to adjust the octave numbers reported by the monitoring program (or which `MIDI_OCTAVE` offset to use in your midizaprc file).

Besides MIDI notes and control change (`CC`) messages, the `midizap` program also supports receiving and sending program change (`PC`) and pitch bend (`PB`) messages. This should cover most common use cases. Other messages (in particular, aftertouch and system messages) are not supported right now, but may be added in the future. Again, please refer to the example.midizaprc file and the comments in the readconfig.c for further details.

## Secondary MIDI Ports

Some MIDI controllers need a more elaborate setup than what we've seen so far, because they have motor faders, LEDs and similar controls requiring feedback from the application. To accommodate these, you can use the `-o2` option of `midizap` to create a second pair of MIDI input and output ports, named `midi_input2` and `midi_output2`. Use of this option also activates a second MIDI default section in the midizaprc file, labeled `[MIDI2]`, which is used exclusively for translating MIDI from the second input port and sending the resulting MIDI data to the second output port. Typically, the translations in the `[MIDI2]` section will be the inverse of those in the `[MIDI]` section, or whatever it takes to translate the MIDI feedback from the application back to MIDI data which the controller understands.

You then wire up the controller to the `midi_input` port of midizap and the `midi_output` port to the application as before, but in addition you also connect the application back to midizap's `midi_input2` port, and the `midi_output2` port to the controller. This second path is what is needed to translate the feedback from the application and send it back to the controller. Please check the example.midizaprc file for a simple example illustrating this kind of setup.
