#include <glib-2.0\glib.h>

#include "config.h"

#define SETTINGS_FILENAME	"settings.ini"

static GKeyFile* keyfile = NULL;
static GError* error = NULL;

void config_open()
{
	keyfile = g_key_file_new();

	if (!g_key_file_load_from_file(keyfile, SETTINGS_FILENAME, G_KEY_FILE_NONE, &error))
	{
		// TODO: return error code
		g_debug("%s", error->message);
		return;
	}
}

GList* config_get_int_list(const char* group, const char* key)
{
	int count,i;
	GList* values = NULL;// g_list_alloc();
	int* valuesArray = g_key_file_get_integer_list(keyfile, group, key, &count, &error);
	if (valuesArray == NULL)
		return values;

	for (i = 0; i < count; ++i)
	{
		int value = valuesArray[i];
		values = g_list_append(values, value);
	}

	g_free(valuesArray);

	return values;
}

char* config_get_string(const char* group, const char* key)
{
	// todo: should free string after use?
	return g_key_file_get_string(keyfile, group, key, &error);
}

int config_get_int(const char* group, const char* key)
{
	return g_key_file_get_integer(keyfile, group, key, &error);
}

void config_close()
{
	if (keyfile != NULL)
	{
		keyfile = NULL;
		g_key_file_free(keyfile);
	}
}

GList* config_get_keys(const char* group)
{
	int count, i;
	GList* keys = NULL;

	char** keysArray = g_key_file_get_keys(keyfile, group, &count, &error);
	if (keysArray == NULL)
		return keys;

	for (i = 0; i < count; ++i)
	{
		keys = g_list_append(keys, keysArray[i]);
	}

	//g_strfreev(keysArray);
	return keys;
}