% midizap(1)

# Synopsis

midizap [-h] [-k] [-o[2]] [-j *name*] [-r *rcfile*] [-d[rskmj]]

# Options

-h
:   Print a short help message.

-k
:   Keep track of key (on/off) status of MIDI notes and control switches. This isn't generally recommended, but may occasionally be useful to deal with quirky controllers sending note- or control-ons without corresponding off messages.

-o[2]
:   Enable MIDI output. Add "2" for a second pair of MIDI ports to be used, e.g., for controller feedback.

-j *name*
:   Set the Jack client name. Default: "midizap".

-r *rcfile*
:   Set the configuration file name. Default: Taken from the MIDIZAP_CONFIG_FILE environment variable if it exists, or ~/.midizaprc if it exists, /etc/midizaprc otherwise.

-d[rskmj]
:   Enable various debugging options: r = regex (print matched translation sections), s = strokes (print the parsed configuration file in a human-readable format), k = keys (print executed translations), m = midi (MIDI monitor, print all recognizable MIDI input), j = jack (additional Jack debugging output). Just `-d` enables all debugging options. (NB: The names of these options aren't really very mnemonic in some cases, but they provide compatibility with Eric Messick's ShuttlePRO program on which midizap is based.)

# Description

The midizap program translates Jack MIDI input into X keystrokes, mouse button presses, scroll wheel events, or, as an option, MIDI output. It does this by matching the `WM_CLASS` and `WM_NAME` properties of the window that has the keyboard focus against the regular expressions for each application section in its configuration (midizaprc) file. If a regex matches, the corresponding set of translations is used. Otherwise the program falls back to a set of translations in a default section at the end of the file, if available.

The midizaprc file is just an ordinary text file which you can edit to configure the program for use with any kind of application taking keyboard, mouse or MIDI input. An example.midizaprc file containing sample configurations for some applications is included in the sources. Also, in the examples directory you can find some more examples of configuration files for various purposes.

midizap provides you with a way to hook up just about any MIDI controller to your favorite multimedia applications, like digital audio workstation (DAW) programs, as well as audio and video editors. The MIDI output option is useful if the target application supports MIDI, but can't work directly with your controller because of protocol incompatibilities. In particular, you can use midizap to turn pretty much any MIDI controller with enough faders and buttons into a Mackie-compatible mixing device for DAW programs. Another common use case is video editing software, which rarely offers built-in MIDI controller support. midizap allows you to map the faders, encoders and buttons of your MIDI controller to corresponding keyboard commands of your video software for cutting, marking, playback, scrolling and zooming.

In other words, as long as the target application can be controlled with simple keyboard shortcuts and/or MIDI commands, midizap should be able to make it work with your controller.

# Installation

First, make sure that you have the required dependencies installed. The program needs a few X11 libraries and Jack. And of course you need GNU make and gcc (the GNU C compiler). On Ubuntu and other Debian-based systems you should be able to get everything that's needed by running this command:

    sudo apt install build-essential libx11-dev libxtst-dev libjack-dev

Then just run `make` and `sudo make install`. This installs the example.midizaprc file as /etc/midizaprc, and the midizap program and the manual page in the default install location. Usually this will be under /usr/local, but the installation prefix can be changed with the `prefix` variable in the Makefile. Also, package maintainers can use the `DESTDIR` variable as usual to install into a staging directory for packaging purposes.

# Configuration File

After installation the system-wide default configuration file will be in /etc/midizaprc, where the program will be able to find it. We recommend copying this file to your home directory, renaming it to .midizaprc:

    cp /etc/midizaprc ~/.midizaprc

The ~/.midizaprc file, if it exists, takes priority over /etc/midizaprc, so it becomes your personal default midizap configuration. You can edit this file as you see fit, in order to customize existing or add your own application configurations, adjust the bindings for the MIDI controllers that you have, etc. (If you create any new configurations which might be useful for others, please consider submitting them so that they can be included in future releases.)

It is also possible to specify the configuration file to be used, by invoking midizap with the `-r` option on the command line, e.g.: `midizap -r myconfig.midizaprc`. This is often used with more specialized configurations dealing with specific applications or MIDI controllers.

**NOTE:** The program automatically reloads the midizaprc file whenever it notices that the file has been changed. Thus you can edit the file while the program keeps running, and have the changes take effect immediately without having to restart the program. When working on new translations, you may want to run the program in a terminal, and employ some or all of the debugging options explained below to see exactly how your translations are being processed.

# Basic Usage

The midizap program is a command line application, so you typically run it from the terminal, but of course it is also possible to invoke it from your desktop environment's startup files once you've set up everything to your liking.

Try `midizap -h` for a brief summary of the available options with which the program can be invoked.

midizap uses [Jack][] for doing all its MIDI input and output, so you need to be able to run Jack and connect the Jack MIDI inputs and outputs of the program. While it's possible to do all of that from the command line as well, we recommend using a Jack front-end and patchbay program like [QjackCtl][] to manage Jack and to set up the MIDI connections. In QJackCtl's setup, make sure that you have selected `seq` as the MIDI driver. This exposes the ALSA sequencer ports of your MIDI hardware and other non-Jack ALSA MIDI applications as Jack MIDI ports, so that they can easily be connected to midizap.

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

It goes without saying that these debugging options will be very helpful when you start developing your own bindings. The `-d` option can be combined with various option characters to choose exactly which kinds of debugging output you want; `r` ("regex") prints the matched translation section (if any) along with the window name and class of the focused window; `s` ("strokes") prints the parsed contents of the configuration file in a human-readable form whenever the file is loaded; `k` ("keys") shows the recognized translations as the program executes them, in the same format as `s`; `m` ("MIDI") prints *any* MIDI input, so that you can figure out which MIDI tokens to use for configuring the translations for your controller; and `j` adds some debugging output from the Jack driver. You can also just use `-d` to enable all debugging output. (Most of these options are also available as directives in the midizaprc file; please check the distributed example.midizaprc for details.)

Have a look at the distributed midizaprc file for more examples. Most of the other translations in the file assume a Mackie-like device with standard playback controls and a jog wheel. Any standard DAW controller which can be switched into Mackie mode should work with these. Otherwise, editing the configuration to make the translations work with your controller should be a piece of cake.

More information about the available configurations and on how to actually create your own configurations can be found in the example.midizaprc file. This also contains a brief explanation of the syntax used to denote the MIDI messages to be translated. You may also want to look at the comments at the top of readconfig.c for further technical details.

# MIDI Output

As already mentioned, the midizap program can also be made to function as a MIDI mapper which translates MIDI input to MIDI output. MIDI output is enabled by running the program as `midizap -o`. This equips the Jack client with an additional MIDI output port named `midi_out` (visible on the left side of QJackCtl's Connection window). 

The example.midizaprc file comes with a sample configuration in the special `[MIDI]` default section for illustration purposes. This section is only active if the program is run with the `-o` option. It allows MIDI output to be sent to any connected applications, no matter which window currently has the keyboard focus. This is probably the most common way to use this feature, but of course it is also possible to have application-specific MIDI translations, in the same way as with X11 key bindings. In fact, you can freely mix mouse actions, key presses and MIDI messages in all translations.

You can try it and test that it works by running `midizap -o`, firing up a MIDI synthesizer such as [FluidSynth][] or its graphical front-end [Qsynth][], and employing QjackCtl to connect its input it to midizap's output port. In the sample configuration, the notes `C4` thru `F4` in the small octave have been set up so that you can operate a little drumkit, and a binding for the volume controller (`CC7`) has been added as well. The relevant portion from the configuration entry looks as follows:

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

Besides MIDI notes and control change (`CC`) messages, the midizap program also supports receiving and sending program change (`PC`) and pitch bend (`PB`) messages. This should cover most common use cases. Other messages (in particular, aftertouch and system messages) are not supported right now, but may be added in the future. Again, please refer to the example.midizaprc file and the comments in readconfig.c for further details.

# Octave Numbering

A note on the octave numbers in MIDI note designations is in order here. There are various different standards for numbering octaves, and different programs use different standards, which can be rather confusing. E.g., there's the ASA (Acoustical Society of America) standard where middle C is C4, also known as "scientific" or "American standard" pitch notation. At least two other standards exist specifically for MIDI octave numbering, one in which middle C is C3 (so the lowest MIDI octave starts at C-2), and zero-based octave numbers, which start at C0 and have middle C at C5. There's not really a single "best" standard here, but the latter tends to appeal to mathematically inclined and computer-savvy people, and is also what is used by default in the midizaprc file.

However, if you prefer a different numbering scheme then you can easily change this by specifying the desired offset for the lowest MIDI octave with the special `MIDI_OCTAVE` directive in the configuration file. For instance:

~~~
MIDI_OCTAVE -1 # ASA pitches (middle C is C4)
~~~

This is useful, in particular, if you use some external MIDI monitoring software to figure out which notes to put into your midizaprc file. To these ends, just check how the program prints middle C, and adjust the `MIDI_OCTAVE` offset in your midizaprc file accordingly. (Note that midizap's built-in MIDI monitoring facility always prints out MIDI notes using the `MIDI_OCTAVE` offset that is in effect. Thus in this case the printed note tokens will always be in exactly the form that is to be used in the midizaprc file, no matter what the `MIDI_OCTAVE` offset happens to be.)

# Shift State

The special `SHIFT` token toggles an internal shift state, which can be used to generate alternative output for certain MIDI messages. Please note that the `SHIFT` token doesn't generate any output by itself; it merely toggles the internal shift bit which can then be queried in other translations to distinguish between shifted and unshifted bindings for the same input message.

To these ends, there are two additional prefixes which indicate the shift status in which a translation is active. Unprefixed translations are active only in unshifted state. The `^` prefix denotes a translation which is active only in shifted state, while the `?` prefix indicates a translation which is active in *both* shifted and unshifted state.

Many DAW controllers have some designated shift keys which can be used for this purpose, but the following will actually work with any key-style MIDI message. E.g., to bind the shift key (`A#5`) on a Mackie controller:

~~~
?A#5 SHIFT
~~~

Note the `?` prefix indicating that this translation is active in both unshifted and shifted state, so it is used to turn shift state both on and off, giving a "Caps Lock"-style of toggle key. If you'd rather have an ordinary shift key which turns on shift state when pressed and immediately turns it off when released again, you can do that as follows:

~~~
?A#5 SHIFT RELEASE SHIFT
~~~

Having set up the translation for the shift key itself, we can now indicate that a translation should be valid only in shifted state with the `^` prefix. This makes it possible to assign different functions, e.g., to buttons and faders which depend on the shift state. Here's a typical example which maps a control change to either Mackie-style fader values encoded as pitch bends, or incremental encoder values:

~~~
CC48=  PB[129]-1 # translate controller to pitch bend when unshifted
^CC48= CC16~     # translate controller to encoder when shifted
~~~

**NOTE:** To keep things simple, only one shift status is available in the present implementation. Also, when using a shift key in the manner described above, then its status is *only* available internally to the midizap program; the host application never gets to see it. If your host software does its own handling of shift keys (as most Mackie-compatible DAW software does), then it's usually more convenient to simply pass those keys on to the application and have it take care of them.

However, midizap's internal shift status feature may come in handy if your controller simply doesn't have enough buttons and faders to control all the essential features of your target application. In this case the internal shift feature makes it possible to (almost) double the amount of controls available on the device. For instance, you can emulate a Mackie controller with both encoders and faders on a device which only has a single set of faders, by assigning the shifted faders to the encoders, as shown above.

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

You may want to place this directive directly into a configuration file if the configuration is primarily aimed at doing MIDI translations, so you'd like to have the MIDI output enabled by default. Typically, such configurations will include just a default `[MIDI]` section and little else. As explained below, it's also possible to have *two* pairs of input and output ports, in order to deal with controller feedback from the application. This is achieved by either invoking midizap with the `-o2` option, or by employing the `JACK_PORTS 2` directive in the configuration file.

Last but not least, midizap also supports Jack session management, which makes it possible to record the options the program was invoked with, along with all the MIDI connections. This feature can be used with any Jack session management software. Specifically, QjackCtl has its own built-in Jack session manager which is available in its Session dialog. To use this, launch midizap and any other Jack applications you want to have in the session, use QjackCtl to set up all the connections as needed, and then the "Save" (or "Save and Quit") option in the Session dialog to have the session recorded. Now, at any later time you can relaunch the same session with the "Load" (or "Recent") option in the same dialog.

# Secondary MIDI Ports

Some MIDI controllers need a more elaborate setup than what we've seen so far, because they have motor faders, LEDs, etc. requiring feedback from the application. To accommodate these, you can use the `-o2` option of midizap, or the `JACK_PORTS 2` directive in the midizaprc file, to create a second pair of MIDI input and output ports, named `midi_input2` and `midi_output2`. Use of this option also activates a second MIDI default section in the midizaprc file, labeled `[MIDI2]`, which is used exclusively for translating MIDI from the second input port and sending the resulting MIDI data to the second output port. Typically, the translations in the `[MIDI2]` section will be the inverse of those in the `[MIDI]` section, or whatever it takes to translate the MIDI feedback from the application back to MIDI data which the controller understands.

You then wire up midizap's `midi_input` and `midi_output` ports to controller and application as before, but in addition you also connect the application back to midizap's `midi_input2` port, and the `midi_output2` port to the controller. This reverse path is what is needed to translate the feedback from the application and send it back to the controller. A full-blown example for this kind of setup can be found in examples/APCmini.midizaprc in the sources, which shows how to emulate a Mackie controller with AKAI's APCmini device, so that it readily works with DAW software such as Ardour and Reaper.

You can also use examples/APCmini.midizaprc as a blueprint for your own Mackie emulations. If your controller has enough buttons and faders to serve as a mixing device, you just need to figure out the MIDI messages which the device generates, and which MIDI messages can be sent back to the device for controller feedback (if the device supports it). This information can hopefully be gleaned from your controller's manual or found on the web somewhere, or you can figure it out on your own by running midizap with its MIDI monitoring option (`-dm`).

# Notes

midizap is licensed under the GPLv3, please check the accompanying LICENSE file for details.

Copyright 2013 Eric Messick (FixedImagePhoto.com/Contact)  
Copyright 2018 Albert Graef (<aggraef@gmail.com>)

This is a version of Eric Messick's ShuttlePRO program which has been redesigned to use Jack MIDI instead of the Contour Design Shuttle devices that the original program was written for.

[ShuttlePRO][nanosyzygy/ShuttlePRO] was originally written in 2013 by Eric Messick, based on earlier code by Trammell Hudson (<hudson@osresearch.net>) and Arendt David (<admin@prnet.org>). The present version of the program is based on Albert Graef's [fork][agraef/ShuttlePRO] of the program. All the translation features of Eric's version are still there (in particular, key and mouse translations work exactly the same), but of course the code has undergone quite some significant changes to accommodate the MIDI input and output facilities. The Jack MIDI driver code is based on code from Spencer Jackson's [osc2midi][] utility, and on the simple_session_client.c example available in the Jack [git repository][jackaudio/example-clients].

[nanosyzygy/ShuttlePRO]: https://github.com/nanosyzygy/ShuttlePRO
[agraef/ShuttlePRO]: https://github.com/agraef/ShuttlePRO
[osc2midi]: https://github.com/ssj71/OSC2MIDI
[jackaudio/example-clients]: https://github.com/jackaudio/example-clients
