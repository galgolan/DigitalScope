#include <gtk/gtk.h>

#include "app.h"

int main(int argc, char *argv[])
{
	return g_application_run(G_APPLICATION(scope_app_new()), argc, argv);
}