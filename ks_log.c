#include <stdio.h>
#include <time.h>
#include <sys/timeb.h>
#include <unistd.h> /* not in VC */
/* Include all this code in c/c++ file */
/* Use kss(s,s) ksn(s,n) ksx(s,hex) ksp(s,ptr) */
static const char* tmstrng() { static char tr[64]; char tf[32];
  struct timeb tb; struct tm ts; ftime(&tb); localtime_r(&tb.time,&ts);
  strftime(tf,sizeof(tf),"%H:%M:%S.%%03d",&ts);
  sprintf(tr,tf,(int)tb.millitm); return tr; }
#define KSFNDEF( fnm, typ, fmt ) \
static void fnm( const char* msg, typ arg ) { FILE* f; \
  f = fopen("/var/tmp/WHATEVERDIR/ks.log","at"); if( !f ) return; \
  fprintf(f, "%s %5d %s " fmt "\n", tmstrng(), (int)getpid(), msg, arg ); fclose(f); }
KSFNDEF( ksx, unsigned long, "%016lX" )
KSFNDEF( ksp, void*, "%016lX" )
KSFNDEF( ksn, long, "%ld" )
KSFNDEF( kss, const char*, "[%s]" )
