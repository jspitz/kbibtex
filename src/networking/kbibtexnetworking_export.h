#ifndef KBIBTEXNETWORKING_EXPORT_H
#define KBIBTEXNETWORKING_EXPORT_H

#include <kdemacros.h>

#ifndef KBIBTEXNETWORKING_EXPORT
# if defined(MAKE_NETWORKING_LIB)
/* We are building this library */
#  define KBIBTEXNETWORKING_EXPORT KDE_EXPORT
# else // MAKE_NETWORKING_LIB
/* We are using this library */
#  define KBIBTEXNETWORKING_EXPORT KDE_IMPORT
# endif // MAKE_NETWORKING_LIB
#endif // KBIBTEXNETWORKING_EXPORT

#endif // KBIBTEXNETWORKING_EXPORT_H