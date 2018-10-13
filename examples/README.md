
# Examples

This folder contains a couple of sample midizap configurations for different controllers:

- [APCmini.midizaprc](APCmini.midizaprc): Mackie emulation for the AKAI APCmini

- [Maschine.midizaprc](Maschine.midizaprc): Mackie emulation for the NI Maschine Mk3 (this requires Harry v. Haaren's Ctlra software to make the device work in Linux, please check the config for detailed instructions)

- [MPKmini2.midizaprc](MPKmini2.midizaprc): minimal Mackie emulation for the AKAI MPKmini Mk2 keyboard

- [nanoKONTROL2.midizaprc](nanoKONTROL2.midizaprc): fix up the marker keys of the Korg nanoKONTROL2 in Cubase mode for use in Ardour

- [XTouchMini.midizaprc](XTouchMini.midizaprc): adds a bunch of Mackie control functions to make the device more useful in MC mode

- [XTouchONE.midizaprc](XTouchONE.midizaprc): adds a bunch of Mackie control functions to make the device more useful in MC mode

Other interesting items:

- [x-touch-one.device](x-touch-one.device): X-Touch ONE device description for Ardour 5.12 (this is basically the X-Touch description with a bank size of 1, so that all tracks become accessible)

	Installation: Copy the x-touch-one.device file to your ~/.config/ardour5/mcp/ directory and select "Behringer X-Touch ONE" as your Mackie control surface in Ardour's preferences dialog.

	Note: You can also use my patched-up version of Ardour 5.12 instead, which fixes up the single-channel bank changes so that the X-Touch ONE will work nicely with Ardour *without* having to set the bank size to 1 (which will be problematic if you use the ONE in combination with other Mackie controllers which require a larger bank size). You can find the sources here: https://github.com/agraef/ardour/tree/5.12-ag-mcpfixes. Make sure to check out the 5.12-ag-mcpfixes branch. This also has a few other bugfixes in the MCP code which will become available with Ardour 6.0, but aren't in the upstream 5.12 tree.
