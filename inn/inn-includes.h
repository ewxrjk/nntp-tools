#ifndef INN_INCLUDES_H
#define INN_INCLUDES_H

/* INN headers produce different structure layouts depending on whether it was
 * configured with SSL support, so we must jump through an stupid hoop to get a
 * usable header file. */
#if INN_HAVE_SSL
#define HAVE_SSL 1
#endif

#include <inn/history.h>
#include <inn/innconf.h>
#include <inn/libinn.h>
#include <inn/paths.h>
#include <inn/wire.h>

#if INN_HAVE_SSL
#undef HAVE_SSL
#endif

#endif /* INN_INCLUDES_H */
