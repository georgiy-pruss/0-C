// See w10clk.c

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/timeb.h>
#include <unistd.h>

typedef int bool;
#define true 1
#define false 0
extern bool g_seconds;
extern void help_on_error_input();

int
process_char( int c, char* s, int m, int n ) // m - max length
{
  time_t t;
  struct tm* tmptr;
  static time_t t_start;       static int t_start_ms;
  static int    t_idle;        static int t_idle_ms;
  static time_t t_break_start; static int t_break_start_ms;
  if( '0'<=c && c<='9' || 'A'<=c && c<='F' || strchr( "+-/._:", c ) )
  {
    s[n]=(char)c; if(n<m) ++n; s[n]=0;
  }
  else if( c==' ' )
  {
    n=0; s[n]=0;
  }
  else if( c=='\b' ) // bksp
  {
    if( n>0 ) --n; s[n]=0;
  }
  else if( c=='\t' ) // tab
  {
    g_seconds = ! g_seconds;
  }
  else if( c=='u' || c=='d' || c=='n' )
  {
    t = time(NULL);
    tmptr = localtime(&t); // assume it can't be NULL
    if( c=='u' )
      n = strftime( s, m, "%s", tmptr );
    else if( c=='d' )
      n = strftime( s, m, "%Y/%m/%d %a", tmptr );
    else
    {
      int x = tmptr->tm_hour*3600 + tmptr->tm_min*60 + tmptr->tm_sec;
      n = sprintf( s, "%d", x );
    }
  }
  else if( c=='c' )
  {
    int x; int k=sscanf( s, "%d", &x );
    if( k==1 ) { x = (x-32)*5/9; n = sprintf( s, "%d", x ); }
  }
  else if( c=='f' )
  {
    int x; int k=sscanf( s, "%d", &x );
    if( k==1 ) { x = x*9/5+32; n = sprintf( s, "%d", x ); }
  }
  else if( c=='h' )
  {
    int x; int k=sscanf( s, "%X", &x );
    if( k==1 ) { n = sprintf( s, "%d", x ); }
  }
  else if( c=='#' )
  {
    int x; int k=sscanf( s, "%d", &x );
    if( k==1 ) { n = sprintf( s, "%X", x ); }
  }
  else if( c=='s' )
  {
    struct timeb tb; ftime(&tb);
    t_start = tb.time; t_start_ms = (int)tb.millitm;
    t_break_start = 0;
    t_idle = 0; t_idle_ms = 0;
    n = sprintf( s, "%s", "started" );
  }
  else if( c=='e' )
  {
    if( t_start == 0 ) n = sprintf( s, "%s", "not started" );
    else
    {
      struct timeb tb; ftime(&tb);
      int elapsed = tb.time - t_start; int elapsed_ms = tb.millitm - t_start_ms;
      if( elapsed_ms<0 ) { elapsed_ms += 1000; elapsed -= 1; }
      n = sprintf( s, "%d.%03d", elapsed, elapsed_ms );
      if( t_idle > 0 )
      {
        int w = elapsed - t_idle; int w_ms = elapsed_ms - t_idle_ms;
        if( w_ms<0 ) { w_ms += 1000; w -= 1; }
        n += sprintf( s+n, ", idle:%d.%03d, work:%d.%03d", t_idle, t_idle_ms, w, w_ms );
      }
    }
  }
  else if( c=='b' )
  {
    if( t_start == 0 ) n = sprintf( s, "%s", "not started" );
    else if( t_break_start == 0 )
    {
      struct timeb tb; ftime(&tb);
      t_break_start = tb.time; t_break_start_ms = tb.millitm;
      n = sprintf( s, "%s", "break" );
    }
    else n = sprintf( s, "%s", "alrady break" );
  }
  else if( c=='g' )
  {
    if( t_start == 0 ) n = sprintf( s, "%s", "not started" );
    if( t_break_start != 0 )
    {
      struct timeb tb; ftime(&tb);
      int b = tb.time - t_break_start; int b_ms = tb.millitm - t_break_start_ms;
      if( b_ms<0 ) { b_ms += 1000; b -= 1; }
      t_idle += b; t_idle_ms += b_ms;
      if( t_idle_ms>=1000 ) { t_idle += 1; t_idle_ms -= 1000; }
      t_break_start = 0;
      n = sprintf( s, "%d.%03d was on break, idle:%d.%03d", b, b_ms, t_idle, t_idle_ms );
    }
    else n = sprintf( s, "%s", "not on break" );
  }
  else
  {
    help_on_error_input();
  }
  return n;
}
