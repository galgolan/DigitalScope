#ifndef _DRAWING_H_
#define _DRAWING_H_

#include <gtk-3.0\gtk\gtk.h>

void drawing_resize(int width, int height);

DWORD WINAPI drawing_worker_thread(LPVOID param);

// paints the dest context from the buffer
void drawing_copy_from_buffer(cairo_t* dest);

// fires a draw event if needed
void drawing_request_redraw();

// fires a draw event
void drawing_redraw();

// draws to the internal buffer
void drawing_update_buffer();

#endif