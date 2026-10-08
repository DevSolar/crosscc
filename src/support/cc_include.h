#ifndef CROSSCC_SUPPORT_CC_INCLUDE_H
#define CROSSCC_SUPPORT_CC_INCLUDE_H

#if defined( __clang__ )
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wbad-function-cast"
#pragma clang diagnostic ignored "-Wc++98-compat"
#pragma clang diagnostic ignored "-Wc99-compat"
#pragma clang diagnostic ignored "-Wdeclaration-after-statement"
#pragma clang diagnostic ignored "-Wimplicit-void-ptr-cast"
#pragma clang diagnostic ignored "-Wextra-semi-stmt"
#pragma clang diagnostic ignored "-Wpadded"
#pragma clang diagnostic ignored "-Wswitch-default"
#endif

#include "cc.h"

typedef str( char ) string;

#if defined( __clang__ )
#pragma clang diagnostic pop
#endif

#ifndef cc_foreach
#define cc_foreach( ... ) for_each( __VA_ARGS__ )
#endif

#endif
