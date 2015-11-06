args = `pkg-config --cflags gtk+-3.0` `pkg-config --libs gtk+-3.0` -IC:/GTK/include/glib-2.0 -IC:/GTK/lib/glib-2.0/include -D _inline= -std=c99 -O0

all:					scopeui.exe
scopeui.exe:			config.o formatting.o drawing.o main.o measurement.o protocol.o scope.o scope_ui_handlers.o serial.o threads.o trace_math.o
						gcc -o runme.exe config.o drawing.o formatting.o main.o measurement.o protocol.o scope.o scope_ui_handlers.o serial.o threads.o trace_math.o $(args)
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
clean:
						rm *.o -f
						rm *.exe -f