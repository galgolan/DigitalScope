# DigitalScope

Specifications (subject to change):
-----------------------------------

* Analog Channels		2
* Input Voltage		-24v to +24v
* Input Impedance		1MOhm, 10pF (compatible with standard probes)
* Analog Bandwidth	1MHz
* Sample Rate		1 MSamples/sec per channel
* Coupling			AC/DC
* Trigger				Raising Edge, Falling Edge or Both, with selectable level (-12v to +10v) and channel 1 or 2 as source
* Connection to PC	Virtual COM Port over USB connection

Overview
--------
This is a DSO project based on a the Tiva™ C Series TM4C1294 Connected LaunchPad dev board.
The project contains:
* embedded C code for the dev board - for CodeComposer Studio
* schematics for the electronic circuits
* code for a GUI based on GTK+3.0 written in C for Windows - Visual Studio 2013 project and a makefile. 

This project is still under work and is expected to be completed at the end of 2015 - check back soon !