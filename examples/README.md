
# Examples

This folder contains a couple of sample midizap configurations for different controllers:

- [APCmini.midizaprc](APCmini.midizaprc): Mackie emulation for the AKAI APCmini

- [Maschine.midizaprc](Maschine.midizaprc): Mackie emulation for the NI Maschine Mk3 (this requires Harry v. Haaren's Ctlra software to make the device work in Linux, please check the config for detailed instructions)

- [MPKmini2.midizaprc](MPKmini2.midizaprc): minimal Mackie emulation for the AKAI MPKmini Mk2 keyboard

- [nanoKONTROL2.midizaprc](nanoKONTROL2.midizaprc): fix up the marker keys of the Korg nanoKONTROL2 in Cubase mode for use in Ardour

- [XTouchONE.midizaprc](XTouchONE.midizaprc): map the SCRUB key to the more important SHIFT key which the X-Touch ONE lacks

- [XTouchONE2.midizaprc](XTouchONE2.midizaprc): an improved config for the X-Touch ONE which turns the SCRUB key into an internal shift key, showing how to assign arbitrary MCP key combinations to the shifted keys

Other interesting items:

- [midizap.xml](midizap.xml): ready-to-use QjackCtl patchbay for all the sample configurations, auto-connects to Ardour

	Installation: Open QjackCtl's Patchbay dialog, load the midizap.xml file and activate the patchbay.

- [x-touch-one.device](x-touch-one.device): X-Touch ONE device description for Ardour 5.12 (this is basically the X-Touch description with a bank size of 1, so that all tracks become accessible)

	Installation: Copy the x-touch-one.device file to your ~/.config/ardour5/mcp/ directory and select "Behringer X-Touch ONE" as your Mackie control surface in Ardour's preferences dialog.
