#ifndef __SCOPEAPP_H
#define __SCOPEAPP_H

#include <gtk/gtk.h>

#define SCOPE_APP_TYPE (scope_app_get_type ())
#define SCOPE_APP(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SCOPE_APP_TYPE, ScopeApp))

typedef struct _ScopeApp       ScopeApp;
typedef struct _ScopeAppClass  ScopeAppClass;

GType           scope_app_get_type(void);
ScopeApp     *scope_app_new(void);

#endif /* __EXAMPLEAPP_H */