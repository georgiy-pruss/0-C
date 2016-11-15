// Part of w10clk

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/timeb.h>
#include <unistd.h>
#include "../_.h"

typedef unsigned int uint;
typedef int bool;
#define false 0
#define true 1
extern bool g_seconds; // show seconds hand
extern bool help_on_error_input(); // true if OK pressed
extern uint calculate( char* src, double* res ); // returns 0 if ok

extern uint ymd2n( uint y, uint m, uint d );
extern void n2ymd( uint n, /* OUT */ uint* y, uint* m, uint* d );
extern uint n2wd( uint n );

char WD[7][4] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};

uint
process_char( uint c, char* s, uint mn, uint n ) __ // mn - max length
  time_t t; struct tm* tmptr;  uint y,m,d, w, x; int k,a,z; L xx; double b; // temp. vars.
  struct timeb tb;
  static time_t t_start;       static int t_start_ms;
  static int    t_idle;        static int t_idle_ms;
  static time_t t_break_start; static int t_break_start_ms;
  if( '0'<=c && c<='9' || 'A'<=c && c<='F' || strchr( "+-*/%^o._:()#", c ) ) __
    s[n]=(char)c; if(n<mn) ++n; s[n]=0; _
  else if( c==32 || c==27 ) __ // space escape
    n=0; s[n]=0; _
  else if( c=='\b' ) __ // bksp
    if( n>0 ) --n; s[n]=0; _
  else if( c=='\t' ) __ // tab
    g_seconds = ! g_seconds; _
  else if( c=='u' || c=='d' || c=='t' || c=='z' || c=='y' ) __
    t = time(NULL);
    tmptr = localtime(&t); // assume it can't be NULL
    uint today = ymd2n( tmptr->tm_year+1900, tmptr->tm_mon+1, tmptr->tm_mday );
    if( n==0 ) __ // current time in many forms
      if( c=='u' )
        n = strftime( s, mn, "%s", tmptr );
      else if( c=='d' )
        n = strftime( s, mn, "%Y/%m/%d %a", tmptr );
      else if( c=='y' )
        n = strftime( s, mn, "%j", tmptr );
      else if( c=='z' )
        n = strftime( s, mn, "%z", tmptr );
      else // 't'
        n = sprintf( s, "%d", tmptr->tm_hour*3600 + tmptr->tm_min*60 + tmptr->tm_sec ); _
    else __
      if( c=='u' ) __ // unixtime
        // TODO if( strchr( s, '/' ) ) __ // YYYY/MM/DD or MM/DD + HH:MM:SS
        if( sscanf( s, "%llu", &xx )==1 && xx<0x100000000ull ) __ // NNNNNNNu
          time_t t1 = (time_t)xx;
          tmptr = localtime(&t1);
          n = strftime( s, mn, "%Y/%m/%d %H:%M:%S %a", tmptr ); _ _
      else if( c=='d' ) __ // NNNNNNd or YYYY/MM/DDd
        char diff[20] = "";
        if( strchr( s, '/' ) ) __ // YYYY/MM/DD or MM/DD
          k = sscanf( s, "%u/%u/%u", &y, &m, &d );
          if( k==3 && 0<y && y<=10000 && 1<=m && m<=12 && 1<=d && d<=31 )
            x = ymd2n( y, m, d );
          else if( k==2 && 1<=y && y<=12 && 1<=m && m<=31 ) // y is m, m is d
            x = ymd2n( tmptr->tm_year+1900, y, m );
          else
            return n; // for now, the only return from inside
          if( x != today ) sprintf( diff, " %+d", (int)x - (int)today );
          n = sprintf( s, "%d %s%s", x, WD[n2wd(x)], diff ); _
        else if( s[0]=='+' || s[0]=='-' ) __ int xd;
          if( sscanf( s, "%d", &xd )==1 && -xd<(int)today ) __
            x = (uint)((int)today + xd);
            n2ymd( x, &y,&m,&d );
            if( x != today ) sprintf( diff, " %+d", (int)x - (int)today );
            n = sprintf( s, "%d/%d/%d %s%s", y,m,d, WD[n2wd(x)], diff ); _ _
        else __ // NNNNN
          if( sscanf( s, "%u", &x )==1 && x>0 ) __ // don't do it for 0
            n2ymd( x, &y,&m,&d );
            if( x != today ) sprintf( diff, " %+d", (int)x - (int)today );
            n = sprintf( s, "%d/%d/%d %s%s", y,m,d, WD[n2wd(x)], diff ); _ _ _
      else if( c=='t' ) __ // time
        if( strchr( s, ':' ) ) __ // D:H:M:S or H:M:S or M:S
        _ _
      else if( c=='y' ) // year day
        n = strftime( s, mn, "%j", tmptr );
      else if( c=='z' ) // timezone
        n = strftime( s, mn, "%z", tmptr );
      else // 't'
        n = sprintf( s, "%d", tmptr->tm_hour*3600 + tmptr->tm_min*60 + tmptr->tm_sec ); _ _
  else if( c=='f' ) __ // F --> C
    if( sscanf( s, "%lf", &b )==1 ) { b = (b-32.0)/1.8; n = sprintf( s, "%.1f", b ); } _
  else if( c=='c' ) __ // C --> F
    if( sscanf( s, "%lf", &b )==1 ) { b = b*1.8+32.0; n = sprintf( s, "%.1f", b ); } _
  else if( c=='h' ) __ // help | decimal --> XXXX (hexadecimal)
    if( sscanf( s, "%llu", &xx )==1 ) n = sprintf( s, "#%llX", xx ); _
  else if( c=='=' || c==13 ) __ // = or enter -- calculate expression
    if( calculate( s, &b ) == 0 ) n = sprintf( s, "%.15g", b ); _
  else if( c=='s' ) __ // start stopwatch
    ftime(&tb); t_start = tb.time; t_start_ms = (int)tb.millitm;
    t_break_start = 0; t_idle = 0; t_idle_ms = 0;
    n = sprintf( s, "%s", "started" ); _
  else if( c=='e' ) __ // elapsed (can be finish)
    if( t_start == 0 ) n = sprintf( s, "%s", "not started" );
    else __ ftime(&tb);
      int elapsed = tb.time - t_start; int elapsed_ms = tb.millitm - t_start_ms;
      if( elapsed_ms<0 ) { elapsed_ms += 1000; elapsed -= 1; }
      n = sprintf( s, "%d.%03d", elapsed, elapsed_ms );
      if( t_idle > 0 ) __
        int w = elapsed - t_idle; int w_ms = elapsed_ms - t_idle_ms;
        if( w_ms<0 ) { w_ms += 1000; w -= 1; }
        n += sprintf( s+n, ", idle:%d.%03d, work:%d.%03d", t_idle, t_idle_ms, w, w_ms ); _ _ _
  else if( c=='b' ) __ // break, start idle time
    if( t_start == 0 ) n = sprintf( s, "%s", "not started" );
    else if( t_break_start == 0 ) __ ftime(&tb);
      t_break_start = tb.time; t_break_start_ms = tb.millitm;
      n = sprintf( s, "%s", "break" ); _
    else n = sprintf( s, "%s", "alrady break" ); _
  else if( c=='g' ) __ // go on, stop break, return to action time
    if( t_start == 0 ) n = sprintf( s, "%s", "not started" );
    if( t_break_start != 0 ) __ ftime(&tb);
      int br = tb.time - t_break_start; int br_ms = tb.millitm - t_break_start_ms;
      if( br_ms<0 ) { br_ms += 1000; br -= 1; }
      t_idle += br; t_idle_ms += br_ms;
      if( t_idle_ms >= 1000 ) { t_idle += 1; t_idle_ms -= 1000; }
      t_break_start = 0;
      n = sprintf( s, "%d.%03d was on break, idle:%d.%03d", br, br_ms, t_idle, t_idle_ms ); _
    else n = sprintf( s, "%s", "not on break" ); _
  else __ // unrecognized keypress
    static bool to_show_help = true;
    if( to_show_help )
      if( !help_on_error_input() )
         to_show_help = false; _
  return n; _
