#ifndef _

#define V void
#define C char
typedef char* S;
typedef const char* KS;
typedef unsigned char B;
#define I int
typedef unsigned int U;
typedef unsigned long long L;
typedef double D;
#define K const
#define O static /* own */
#define E extern
#define OUT /* output parameter */

#define __ {
#define _  }

#define R return
#define CASE break; case
#define DO(_i,_n) for(I _i=0;_i<(_n);++_i)
#define DIM(a) (sizeof(a)/sizeof(a[0]))
// changes in ...\Vim\vim80\syntax\c.vim as well

#endif
