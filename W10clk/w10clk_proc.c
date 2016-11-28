// Part of w10clk -- process character/command

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/timeb.h>
#include <unistd.h>
#include "../_.h"

typedef I bool;
#define false 0
#define true 1
E bool g_seconds; // show second hand
E bool help_on_error_input(); // true if OK pressed
E V mb( KS txt, KS cap ); // message box
E U calculate( S src, D* res ); // returns 0 if ok
E S g_tzlist;
E V strcpyupr( S dst, KS src );

E U ymd2n( U y, U m, U d );
E V n2ymd( U n, /* OUT */ U* y, U* m, U* d );
E U n2wd( U n );
E I gettz( I* h, U* m );

C WD[7][4] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};

bool tz_is_set = false;
I tz_offset; I tz_h; U tz_m;

U
process_char( U c, S s, U mn, U n ) __ // mn - max length
  time_t t; struct tm* tmptr;  U y,m,d, w,h, x; I k,a,z; L xx; D b; // temp. vars.
  struct timeb tb;
  O time_t t_start;       O I t_start_ms;
  O I      t_idle;        O I t_idle_ms;
  O time_t t_break_start; O I t_break_start_ms;
  if( '0'<=c && c<='9' || 'A'<=c && c<='Z' || memchr( "+-*/%^o\\&|!.:()#", c, 16 ) ) __
    s[n]=(C)c; if(n<mn) ++n; s[n]=0; _
  else if( c==32 || c==27 ) __ // space escape
    n=0; s[n]=0; _
  else if( c=='\b' ) __ // bksp
    if( n>0 ) --n; s[n]=0; _
  else if( c=='\t' ) __ // tab
    g_seconds = ! g_seconds; _
  else if( c=='u' || c=='d' || c=='t' || c=='z' || c=='y' ) __
    t = time(NULL);
    tmptr = localtime(&t); // assume it can't be NULL
    U today = ymd2n( tmptr->tm_year+1900, tmptr->tm_mon+1, tmptr->tm_mday );
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
            if( (I)t != -1 ) n = sprintf( s, "%u", (U)t ); _ _
        else if( sscanf( s, "%llu", &xx )==1 && xx<0x100000000ull ) __ // NNNNNNN
          time_t t1 = (time_t)xx;
          tmptr = localtime(&t1);
          n = strftime( s, mn, "%Y/%m/%d.%H:%M:%S %a", tmptr ); _ _
      else if( c=='d' ) __ // NNNNNNd or YYYY/MM/DDd
        C diff[20] = "";
        if( strchr( s, '/' ) ) __ // YYYY/MM/DD or MM/DD
          k = sscanf( s, "%u/%u/%u", &y, &m, &d );
          if( k==3 && 0<y && y<=10000 && 1<=m && m<=12 && 1<=d && d<=31 )
            x = ymd2n( y, m, d );
          else if( k==2 && 1<=y && y<=12 && 1<=m && m<=31 ) // y is m, m is d
            x = ymd2n( tmptr->tm_year+1900, y, m );
          else
            return n; // for now, the only return from inside . . . . . . . . . . . . . . .
          if( x != today ) sprintf( diff, " %+d", (I)x - (I)today );
          n = sprintf( s, "%u %s%s", x, WD[n2wd(x)], diff ); _
        else if( s[0]=='+' || s[0]=='-' ) __ I xd;
          if( sscanf( s, "%d", &xd )==1 && -xd<(I)today ) __
            x = (U)((I)today + xd);
            n2ymd( x, &y,&m,&d );
            if( x != today ) sprintf( diff, " %+d", (I)x - (I)today );
            n = sprintf( s, "%d/%d/%d %s%s", y,m,d, WD[n2wd(x)], diff ); _ _
        else __ // NNNNN
          if( sscanf( s, "%u", &x )==1 && x>0 ) __ // don't do it for 0
            n2ymd( x, &y,&m,&d );
            if( x != today ) sprintf( diff, " %+d", (I)x - (I)today );
            n = sprintf( s, "%d/%d/%d %s%s", y,m,d, WD[n2wd(x)], diff ); _ _ _
      else if( c=='t' ) __ // time conversions
        KS s_abs = (s[0]=='+' || s[0]=='-') ? s+1 : s;
        KS sign = (s[0]=='-') ? "-" : "";
        k = sscanf( s_abs, "%u:%u:%u:%u", &d,&h,&m,&x );
        if( k>1 ) __
          if( k==2 ) { x=h; m=d; h=0; d=0; }        // M:S
          else if( k==3 ) { x=m; m=h; h=d; d=0; }   // H:M:S
          I secs = 86400*d + 3600*h + 60*m + x;   // D:H:M:S
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
        if( s[0]=='.' ) mb( g_tzlist+1, "W10clk - time zones" );
        if( 'A'<=s[0] && s[0]<='Z' ) __
          S tzn = malloc( n+3 );
          tzn[0] = ' '; strcpyupr( tzn+1, s ); tzn[n+1] = ' '; tzn[n+2]='\0'; // " TZN "
          S pos = strstr( g_tzlist, tzn ); I tzv;
          if( pos && sscanf( pos+n+1, "%d", &tzv )==1 ) n = sprintf( s, "%d", tzv );
          free( tzn ); _
        if( s[0]=='+' || s[0]=='-' || '0'<=s[0] && s[0]<='9' ) __ // number N +N -N
          k = sscanf( s, "%d", &z ); h = (U)(z>=0 ? z : -z);
          if( k==1 ) __
            if( h<15 ) m=0; else m=h%100, h=h/100; // split into h,m
            I seconds = 3600*h + 60*m; if( z<0 ) seconds = -seconds;
            time_t new_t = t - 60*tz_offset + seconds;
            tmptr = localtime(&new_t);
            n = strftime( s, mn, "%H:%M:%S %a %m/%d", tmptr ); _ _ _
      else // it can't be here
        n = sprintf( s, "huh?" ); _ _
  else if( c=='f' ) __ // F --> C
    if( sscanf( s, "%lf", &b )==1 ) { b = (b-32.0)/1.8; n = sprintf( s, "%.1f%cC", b, 176 ); } _
  else if( c=='c' ) __ // C --> F
    if( sscanf( s, "%lf", &b )==1 ) { b = b*1.8+32.0; n = sprintf( s, "%.1f%cF", b, 176 ); } _
  else if( c=='k' ) __ // Ki --> k
    if( sscanf( s, "%lf", &b )==1 ) { n = sprintf( s, "%g", b*1.024 ); } _
  else if( c=='h' ) __ // help | decimal --> XXXX (hexadecimal)
    if( sscanf( s, "%llu", &xx )==1 ) n = sprintf( s, "#%llX", xx ); _
  else if( c=='=' || c==13 ) __ // = or enter -- calculate expression
    if( calculate( s, &b ) == 0 ) n = sprintf( s, "%.15g", b ); _
  else if( c=='s' ) __ // start stopwatch
    ftime(&tb); t_start = tb.time; t_start_ms = (I)tb.millitm;
    t_break_start = 0; t_idle = 0; t_idle_ms = 0;
    n = sprintf( s, "%s", "started" ); _
  else if( c=='e' ) __ // elapsed (can be finish)
    if( t_start == 0 ) n = sprintf( s, "%s", "not started" );
    else __ ftime(&tb);
      I elapsed = tb.time - t_start; I elapsed_ms = tb.millitm - t_start_ms;
      if( elapsed_ms<0 ) { elapsed_ms += 1000; elapsed -= 1; }
      n = sprintf( s, "%d.%03d", elapsed, elapsed_ms );
      if( t_idle > 0 ) __
        I w = elapsed - t_idle; I w_ms = elapsed_ms - t_idle_ms;
        if( w_ms<0 ) { w_ms += 1000; w -= 1; }
        n += sprintf( s+n, ", idle:%d.%03d, work:%d.%03d", t_idle, t_idle_ms, w, w_ms ); _ _ _
  else if( c=='p' ) __ // pause, start idle time
    if( t_start == 0 ) n = sprintf( s, "%s", "not started" );
    else if( t_break_start == 0 ) __ ftime(&tb);
      t_break_start = tb.time; t_break_start_ms = tb.millitm;
      n = sprintf( s, "%s", "paused" ); _
    else n = sprintf( s, "%s", "already paused" ); _
  else if( c=='g' ) __ // go on, stop break, return to action time
    if( t_start == 0 ) n = sprintf( s, "%s", "not started" );
    if( t_break_start != 0 ) __ ftime(&tb);
      I br = tb.time - t_break_start; I br_ms = tb.millitm - t_break_start_ms;
      if( br_ms<0 ) { br_ms += 1000; br -= 1; }
      t_idle += br; t_idle_ms += br_ms;
      if( t_idle_ms >= 1000 ) { t_idle += 1; t_idle_ms -= 1000; }
      t_break_start = 0;
      n = sprintf( s, "%d.%03d was on pause, idle:%d.%03d", br, br_ms, t_idle, t_idle_ms ); _
    else n = sprintf( s, "%s", "not paused" ); _
  else __ // unrecognized keypress
    O bool to_show_help = true;
    if( to_show_help )
      if( !help_on_error_input() )
         to_show_help = false; _
  return n; _
