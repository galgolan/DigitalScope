# GCC options:
#		-std=c99 Sets the C99 standatd which allows vard to be declared in the for loop
#		-D _inline=  defines the inline directive as empty. workaround to compile gtk i think.
#		-O0 disables optimizations.
#		-ggdb creates GDB debug symbols.
ROOT_DIR = $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
kissfft_dir = $(ROOT_DIR)/../../../kiss_fft130
GTKDIR = C:/GTK
args = `pkg-config --cflags gtk+-3.0` `pkg-config --libs gtk+-3.0` -I$(GTKDIR)/include/glib-2.0 -I$(GTKDIR)/lib/glib-2.0/include -I$(kissfft_dir)/../ -I$(kissfft_dir) -I$(kissfft_dir)/tools -D _inline= -std=c99 -O0 -ggdb

all:					scopeui.exe
scopeui.exe:			config.o formatting.o drawing.o main.o measurement.o protocol.o scope.o scope_ui_handlers.o serial.o threads.o trace_math.o kiss_fft.o kiss_fftr.o
						gcc -o $@ $^ $(args)
config.o:				config.c
						gcc -c $< $(args)
drawing.o:				drawing.c
						gcc -c $< $(args)
main.o:					main.c
						gcc -c $< $(args)
measurement.o:			measurement.c
						gcc -c $< $(args)
protocol.o:				protocol.c
						gcc -c $< $(args)
scope.o:				scope.c
						gcc -c $< $(args)
scope_ui_handlers.o:	scope_ui_handlers.c
						gcc -c $< $(args)
serial.o:				serial.c
						gcc -c $< $(args)
threads.o:				threads.c
						gcc -c $< $(args)
trace_math.o:			trace_math.c
						gcc -c $< $(args)
formatting.o:			formatting.c
						gcc -c $< $(args)
kiss_fft.o:				$(kissfft_dir)/kiss_fft.c $(kissfft_dir)/kiss_fft.h $(kissfft_dir)/_kiss_fft_guts.h
						gcc -c $^ $(args)
kiss_fftr.o:			kiss_fft.o $(kissfft_dir)/tools/kiss_fftr.c $(kissfft_dir)/tools/kiss_fftr.h
						gcc -c $^ $(args)
clean:
						rm *.o -f
						rm *.exe -f