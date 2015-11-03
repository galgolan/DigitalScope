#ifndef _DRAWING_H_
#define _DRAWING_H_

#include <gtk-3.0\gtk\gtk.h>
#include "scope.h"

void drawing_resize(int width, int height);

DWORD WINAPI drawing_worker_thread(LPVOID param);

// paints the dest context from the buffer
void drawing_copy_from_buffer(cairo_t* dest);

// fires a draw event
void drawing_redraw();

// draws to the internal buffer
bool drawing_update_buffer();

float inverse_translate(int y, const Trace* trace);

int translate(float value, const Trace* trace);

// returns value1-value2
float inverse_translate_diff(int value1, int value2, const Trace* trace, int gridLines, int height);

bool drawing_save_screen_as_image(char* filename);

#endif