# DigitalScope

Specifications (subject to change):
-----------------------------------

* Analog Channels	2
* Input Voltage		-24v to +24v
* Input Impedance	1MOhm, 10pF (compatible with standard probes)
* Analog Bandwidth	1MHz
* Sample Rate		1 MSamples/sec per channel
* Coupling			DC
* Trigger			Mode: Auto, Single or None.
					Type: Rising Edge, Falling Edge or Both.
					Level: Selectable in the range [-24v,10.5v] with 1.5v increments.
					Source: Channel1 or Channel2.
* Cursors
* Math				FFT (linear or dB)
* Measurements		Frequency, DC Mean, Vpp, Vrms, Max, Min, Duty Cycle, Rise Time, Fall Time
* Connection to PC	Virtual COM Port over USB connection

Overview
--------
This is a DSO project based on a the Tiva™ C Series TM4C1294 Connected LaunchPad dev board.
The project contains:
* embedded C code for the dev board - for CodeComposer Studio
* schematics for the electronic circuits
* code for a GUI based on GTK+3.0 written in C for Windows. Can be compiled with the included VS2013 project or the included makefile to compile with gcc.

This project is still under work and is expected to be completed at the end of 2015 - check back soon !