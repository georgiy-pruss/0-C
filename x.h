/* Simple dynamic string class */

#ifndef X0 /* well, instead of artificial __X__ etc */

#include <_.h>

typedef struct X { union { S p; C s[8]; }; U len; U n; } X;

#define X0 {0,0,0} /* initializer for X x=X0; for statics done automatically */

// initialization; undefined state -> defined
#define XI(x)
#define XIN(x,n)
#define XIS(x,s)
#define XIM(x,m,l)
// finalization=free; defined state -> undefined
#define XF(x)
// copy (with underscores - no check if fits)
#define XCS(x,s)
#define XCM(x,m,l)
#define XCS_(x,s)
#define XCM_(x,m,l)
// append
#define XAS(x,s)
#define XAM(x,m,l)
#define XAS_(x,s)
#define XAM_(x,m,l)
// truncate
#define XT(x,l)
// reallocate to current size
#define XR(x)

#endif
