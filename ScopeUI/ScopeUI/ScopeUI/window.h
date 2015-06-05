#ifndef __SCOPEAPPWIN_H
#define __SCOPEAPPWIN_H

#include <gtk/gtk.h>

#include "app.h"

#define SCOPE_APP_WINDOW_TYPE (scope_app_window_get_type ())
#define SCOPE_APP_WINDOW(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SCOPE_APP_WINDOW_TYPE, ScopeAppWindow))

typedef struct _ScopeAppWindow         ScopeAppWindow;
typedef struct _ScopeAppWindowClass    ScopeAppWindowClass;

GType scope_app_window_get_type(void);
ScopeAppWindow* scope_app_window_new(ScopeApp *app);
void scope_app_window_open(ScopeAppWindow* win);

#endif /* __EXAMPLEAPPWIN_H */