#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <glib-2.0\glib.h>

#include "common.h"

void config_open();

// caller is responsible for freeing the list returned
GList* config_get_int_list(const char* group, const char* key);

char* config_get_string(const char* group, const char* key);

int config_get_int(const char* group, const char* key);

void config_close();

#endif