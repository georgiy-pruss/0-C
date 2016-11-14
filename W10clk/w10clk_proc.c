// Part of w10clk

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/timeb.h>
#include <unistd.h>

typedef unsigned long long ULL;
typedef unsigned int uint;
typedef int bool;
#define false 0
#define true 1
extern bool g_seconds;
extern void help_on_error_input();
extern uint calculate( char* src, double* res ); // ret. 0 if ok

extern uint ymd2n( uint y, uint m, uint d );
extern void n2ymd( uint n, /* OUT */ uint* y, uint* m, uint* d );
extern uint n2wd( uint n );

char WD[7][4] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};

int
process_char( int c, char* s, int mn, int n ) // mn - max length
{
  time_t t; struct tm* tmptr;  uint y,m,d, w, x;
  static time_t t_start;       static int t_start_ms;
  static int    t_idle;        static int t_idle_ms;
  static time_t t_break_start; static int t_break_start_ms;
  if( '0'<=c && c<='9' || 'A'<=c && c<='F' || strchr( "+-*/%^o._:()#", c ) )
  {
    s[n]=(char)c; if(n<mn) ++n; s[n]=0;
  }
  else if( c==32 || c==27 ) // space escape
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
  else if( c=='u' || c=='d' || c=='t' || c=='z' || c=='y' )
  {
    t = time(NULL);
    tmptr = localtime(&t); // assume it can't be NULL
    if( n==0 )
    {
      if( c=='u' )
        n = strftime( s, mn, "%s", tmptr );
      else if( c=='d' )
        n = strftime( s, mn, "%Y/%m/%d %a", tmptr );
      else if( c=='y' )
        n = strftime( s, mn, "%j", tmptr );
      else if( c=='z' )
        n = strftime( s, mn, "%z", tmptr );
      else // 't'
        n = sprintf( s, "%d", tmptr->tm_hour*3600 + tmptr->tm_min*60 + tmptr->tm_sec );
    }
    else
    {
      if( c=='u' ) // NNNNNNNu
      {
        ULL xx; int k = sscanf( s, "%llu", &xx );
        if( k==1 && xx<0x100000000ull )
        {
          time_t t1 = (time_t)xx;
          tmptr = localtime(&t1);
          n = strftime( s, mn, "%Y/%m/%d %H:%M:%S %a", tmptr );
        }
      }
      else if( c=='d' ) // NNNNNNd or YYYY/MM/DDd
      {
        if( strchr( s, '/' ) ) // YYYY/MM/DD
        {
          int k = sscanf( s, "%u/%u/%u", &y, &m, &d );
          if( k==3 && 0<y && y<=10000 && 1<=m && m<=12 && 1<=d && d<=31 )
          {
            uint x = ymd2n( y, m, d );
            n = sprintf( s, "%d %s", x, WD[n2wd(x)] );
          }
        }
        else // NNNNN
        {
          if( sscanf( s, "%u", &x )==1 && x>0 ) // don't do it for 0
          {
            n2ymd( x, &y,&m,&d );
            n = sprintf( s, "%d/%d/%d %s", y,m,d, WD[n2wd(x)] );
          }
        }
      }
      else if( c=='y' )
        n = strftime( s, mn, "%j", tmptr );
      else if( c=='z' )
        n = strftime( s, mn, "%z", tmptr );
      else // 't'
        n = sprintf( s, "%d", tmptr->tm_hour*3600 + tmptr->tm_min*60 + tmptr->tm_sec );
    }
  }
  else if( c=='f' ) // F --> C
  {
    double x; int k=sscanf( s, "%lf", &x );
    if( k==1 ) { x = (x-32.0)/1.8; n = sprintf( s, "%.1f", x ); }
  }
  else if( c=='c' ) // C --> F
  {
    double x; int k=sscanf( s, "%lf", &x );
    if( k==1 ) { x = x*1.8+32.0; n = sprintf( s, "%.1f", x ); }
  }
  else if( c=='h' ) // help | decimal --> XXXX (hexadecimal)
  {
    ULL x; int k=sscanf( s, "%llu", &x );
    if( k==1 ) n = sprintf( s, "#%llX", x );
  }
  else if( c=='=' || c==13 ) // = or enter -- calculate expression
  {
    double res; if( calculate( s, &res ) == 0 ) n = sprintf( s, "%.15g", res );
  }
  else if( c=='s' ) // start stopwatch
  {
    struct timeb tb; ftime(&tb);
    t_start = tb.time; t_start_ms = (int)tb.millitm;
    t_break_start = 0;
    t_idle = 0; t_idle_ms = 0;
    n = sprintf( s, "%s", "started" );
  }
  else if( c=='e' ) // elapsed (can be finish)
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
  else if( c=='b' ) // break, start idle time
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
  else if( c=='g' ) // go on, stop break, return to action time
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
  else // unrecognized keypress
  {
    help_on_error_input();
  }
  return n;
}
