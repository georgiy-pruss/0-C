/* Simple dynamic string class */

#ifndef X0 /* well, it serves well instead of artificial __X_H__ etc */

#include <string.h> // strlen,memcpy,etc
#include <stdlib.h> // malloc,free
#include "_.h"

typedef struct X { union { S p; C s[8]; }; U l; U n; } X;

#define X0 {0,0,0} /* initializer for X x=X0; for statics done automatically */

// initialization; undefined state -> defined
// -- be sure to use only XI* in undefined state and other ops only in defined!
#define XI(x)       xi(&(x)) //((x).p=0,(x).n=(x).l=0)
#define XIN(x,n)    ((x).p=(S)malloc(n+1),(x).n=n,(x).len=0,*p='\0')
#define XIS(x,s)    (XIM((x),(s),(strlen(s))))
#define XIM(x,m,k)  ((x).l=(k),((x).l>8)?((x).n=(x).l,(x).p=(S)malloc((x).l+1),memcpy((x).p,(m),(x).l+1)):((x).n=0,memcpy((x).s,(m),(x).l+1)))
#define XIX(x,x2)   (0)
// finalization=free; defined state -> undefined
#define XF(x)       ((x).n>0?(void)free((x).p):(void)0)
// copy (with underscores - no check if fits)
#define XCS(x,s)    (0)
#define XCM(x,m,k)  (0)
#define XCX(x,x2)   (0)
#define XCS_(x,s)   (0)
#define XCM_(x,m,k) (0)
#define XCX_(x,x2)  (0)
// append
#define XAS(x,s)    (0)
#define XAM(x,m,k)  (0)
#define XAX(x,x2)   (0)
#define XAS_(x,s)   (0)
#define XAM_(x,m,k) (0)
#define XAX_(x,x2)  (0)
// truncate
#define XT(x,k)     ((k)>(x).l?((x).l>8?((x).p[k]='\0'):((x).s[k]='\0')):(void)0)
// reallocate to current size
#define XR(x)
// access to data
#define XP(x) ((x).n>8?(x).p:(x).s)
#define XL(x) ((x).l)
#define XN(x) ((x).n)

#endif
