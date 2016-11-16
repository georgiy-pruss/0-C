// Part of w10clk -- process character/command

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
extern bool g_seconds; // show second hand
extern bool help_on_error_input(); // true if OK pressed
extern uint calculate( char* src, double* res ); // returns 0 if ok

extern uint ymd2n( uint y, uint m, uint d );
extern void n2ymd( uint n, /* OUT */ uint* y, uint* m, uint* d );
extern uint n2wd( uint n );
extern int gettz( int* h, uint* m );

char WD[7][4] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};

bool tz_is_set = false;
int tz_offset; int tz_h; uint tz_m;

uint
process_char( uint c, char* s, uint mn, uint n ) __ // mn - max length
  time_t t; struct tm* tmptr;  uint y,m,d, w,h, x; int k,a,z; L xx; double b; // temp. vars.
  struct timeb tb;
  static time_t t_start;       static int t_start_ms;
  static int    t_idle;        static int t_idle_ms;
  static time_t t_break_start; static int t_break_start_ms;
  if( '0'<=c && c<='9' || 'A'<=c && c<='F' || strchr( "+-*/%^o.:()#", c ) ) __
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
        n = strftime( s, mn, "%s", tmptr ); // %s -- unix seconds
      else if( c=='d' )
        n = strftime( s, mn, "%Y/%m/%d %a", tmptr );
      else if( c=='y' )
        n = strftime( s, mn, "%j", tmptr ); // yday
      else if( c=='z' ) __
        if( !tz_is_set ) { tz_offset = gettz( &tz_h, &tz_m ); tz_is_set = true; }
        n = tz_m==0 ? sprintf( s, "%+d", tz_h ) : sprintf( s, "%+d%02u", tz_h, tz_m ); _
      else // 't'
        n = sprintf( s, "%u:%02u:%02u", tmptr->tm_hour, tmptr->tm_min, tmptr->tm_sec ); _
    else __
      if( c=='u' ) __ // unixtime
        if( sscanf( s, "%u/%u/%u.%u:%u:%u", &y,&m,&d, &h,&w,&x )==6 ) __ // YYYY/MM/DD.HH:MM:SS
          if( 1970<=y && 1<=m && m<=12 && 1<=d && d<=31 && h<24 && w<60 && x<60 &&
              (y<2106 || y==2106 && (m<2 || m==2 && d<=7)) ) __
            tmptr->tm_year = y-1900; tmptr->tm_mon = m-1; tmptr->tm_mday = d;
            tmptr->tm_hour = h; tmptr->tm_min = w; tmptr->tm_sec = x;
            t = mktime( tmptr );
            if( (int)t != -1 ) n = sprintf( s, "%u", (uint)t ); _ _
        else if( sscanf( s, "%llu", &xx )==1 && xx<0x100000000ull ) __ // NNNNNNN
          time_t t1 = (time_t)xx;
          tmptr = localtime(&t1);
          n = strftime( s, mn, "%Y/%m/%d.%H:%M:%S %a", tmptr ); _ _
      else if( c=='d' ) __ // NNNNNNd or YYYY/MM/DDd
        char diff[20] = "";
        if( strchr( s, '/' ) ) __ // YYYY/MM/DD or MM/DD
          k = sscanf( s, "%u/%u/%u", &y, &m, &d );
          if( k==3 && 0<y && y<=10000 && 1<=m && m<=12 && 1<=d && d<=31 )
            x = ymd2n( y, m, d );
          else if( k==2 && 1<=y && y<=12 && 1<=m && m<=31 ) // y is m, m is d
            x = ymd2n( tmptr->tm_year+1900, y, m );
          else
            return n; // for now, the only return from inside . . . . . . . . . . . . . . .
          if( x != today ) sprintf( diff, " %+d", (int)x - (int)today );
          n = sprintf( s, "%u %s%s", x, WD[n2wd(x)], diff ); _
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
      else if( c=='t' ) __ // time conversions
        const char* s_abs = (s[0]=='+' || s[0]=='-') ? s+1 : s;
        const char* sign = (s[0]=='-') ? "-" : "";
        k = sscanf( s_abs, "%u:%u:%u:%u", &d,&h,&m,&x );
        if( k>1 ) __
          if( k==2 ) { x=h; m=d; h=0; d=0; }        // M:S
          else if( k==3 ) { x=m; m=h; h=d; d=0; }   // H:M:S
          int secs = 86400*d + 3600*h + 60*m + x;   // D:H:M:S
          n = sprintf( s, "%s%d", sign, secs ); _
        else if( k==1 ) __ // just one number, seconds --> D:H:M:S
          x = d%60; d /= 60;
          m = d%60; d /= 60;
          h = d%24; d /= 24;
          if( d != 0 ) n = sprintf( s, "%s%u:%02u:%02u:%02u", sign, d, h, m, x );
          else if( h != 0 ) n = sprintf( s, "%s%u:%02u:%02u", sign, h, m, x );
          else n = sprintf( s, "%s%u:%02u", sign, m, x ); _ _
      else if( c=='y' ) __ // year day -> M/D this year or Y/M/D --> yd
        k = sscanf( s, "%u/%u/%u", &y, &m, &d );
        if( k==3 ) __ // Y/M/D
          n = sprintf( s, "%u/%u", y, ymd2n( y,m,d ) - ymd2n( y,1,1 ) + 1 ); _
        else if( k==2 && (y<=12 && m<=31) ) __ // M/D current year
          x = tmptr->tm_year+1900;
          n = sprintf( s, "%u", ymd2n( x,y,m ) - ymd2n( x,1,1 ) + 1 ); _
        else if( k==2 && (y>12 || m>31) ) __ // y/yday --> y/m/d
          n2ymd( ymd2n( y,1,1 ) - 1 + m, &y,&m,&d );
          n = sprintf( s, "%u/%u/%u", y, m, d ); _
        else if( k==1 || k==2 && m>31 ) __ // yday (in y) --> m/d
          n2ymd( ymd2n( tmptr->tm_year+1900,1,1 ) - 1 + y, &y,&m,&d );
          n = sprintf( s, "%u/%u", m, d ); _ _
      else if( c=='z' ) __ // timezone
        if( !tz_is_set ) { tz_offset = gettz( &tz_h, &tz_m ); tz_is_set = true; }
        //if( s is ALPHA ) s = replace( tz-dict[ s ] )
        if( s[0]=='+' || s[0]=='-' ) __
          k = sscanf( s+1, "%d", &h ); // skip sign
          if( k==1 ) __
            if( h<15 ) m=0; else m=h%100, h=h/100; // split into h,m
            int seconds = 3600*h + 60*m; if( s[0]=='-' ) seconds = -seconds;
            time_t new_t = t - 60*tz_offset + seconds;
            tmptr = localtime(&new_t);
            n = strftime( s, mn, "%H:%M:%S %a %m/%d", tmptr ); _ _ _
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
