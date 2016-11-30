// Part of w10clk -- process character/command

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <wchar.h> // swprintf
#include <time.h>
#include <sys/timeb.h>
#include <unistd.h>
#include <math.h> // floor
#include "../_.h"

typedef I bool;
#define false 0
#define true 1
E bool g_seconds; // show second hand
E bool help_on_error_input(); // true if OK pressed
E V mb( KS txt, KS cap ); // message box
E V mbw( K wchar_t* txt, K wchar_t* cap ); // message box for unicode
E U calculate( S src, OUT D* res ); // returns 0 if ok
E S g_tzlist;
E V strcpyupr( S dst, KS src );

E U ymd2n( U y, U m, U d );
E V n2ymd( U n, OUT U* y, U* m, U* d );
E U n2wd( U n );
E I gettz( I* h, U* m );
E D moon_phase_date( D myLunation, D phase );
D t2lunation( D t ) { R floor( (t/(864e2*365.25)+70.0)*12.3685 ); }
D y2lunation( D y ) { R floor( (y-1900.0)*12.3685 ); }
D j2n( D j ) { R j-(2400000.5-678578); }

C WD[7][4] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};

bool tz_is_set = false;
I tz_offset; I tz_h; U tz_m;

#define u2j(t) ((D)(t)/864e2+2440587.5)
#define j2u(j) (((j)-2440587.5)*864e2)

U
fmtj( D j, KS fmt, S s, U mn ) __
  time_t t = (time_t)j2u(j); // through CED: ... b-2400000.5+678578;
  struct tm* tmptr = localtime(&t); ///////tmptr->tm_year -= 2015;
  R strftime( s, mn, fmt, tmptr ); _

K wchar_t*
moon_phase_name( D x ) __
  if( 0.00<=x && x<0.02 ) R L"New Moon  ●";
  if( 0.02<=x && x<0.23 ) R L"Waxing Crescent   ☽";
  if( 0.23<=x && x<0.27 ) R L"First Quarter   ◐";
  if( 0.27<=x && x<0.48 ) R L"Waxing Gibbous ◖";
  if( 0.48<=x && x<0.52 ) R L"Full Moon   ◯";
  if( 0.52<=x && x<0.73 ) R L"Waning Gibbous   ◗";
  if( 0.73<=x && x<0.77 ) R L"Last Quarter   ◑";
  if( 0.77<=x && x<0.98 ) R L"Waning Crescent   ☾";
  if( 0.98<=x && x<1.01 ) R L"New Moon  ●";
  R L"Unknown"; _

U
current_moon( S s, U mn, D t, U n ) __
  D j = u2j(t);
  D k = t2lunation(t);
  D newm = moon_phase_date( k, 0.0 ); // new moon
  if( j<newm ) { k -= 1.0; newm = moon_phase_date( k, 0.0 ); }
  D fullm = moon_phase_date( k, 2.0 ); // full moon
  D nw1m = moon_phase_date( k+1.0, 0.0 ); // next new moon
  D z = (j-newm)/(nw1m-newm);
  D v = 0.5 - 0.5*cos( 2*M_PI*z );
  C f[64]; C m[128];
  if( j<=fullm ) __ // between new and full
    fmtj( newm, "Last New Moon: %m/%d %H:%M\n", f, sizeof(f) );
    U len = sprintf( m, f );
    fmtj( fullm,"Next Full Moon: %m/%d %H:%M (in %%.1f days)", f, sizeof(f) );
    sprintf( m+len, f, fullm-j ); _
  else __ // between full and new
    fmtj( fullm, "Last Full Moon: %m/%d %H:%M (%%.1f days ago)\n", f, sizeof(f) );
    U len = sprintf( m, f, j-fullm );
    fmtj( nw1m, "Next New Moon: %m/%d %H:%M (in %%.1f days)", f, sizeof(f) );
    sprintf( m+len, f, nw1m-j ); _
  wchar_t o[512];
  swprintf( o, 512, L"%S\nPhase now: %.1f days (%.1f%%)\nVisible part: %.1f%%\n%s",
      moon_phase_name(z), j-newm, 100.0*z, 100.0*v, m );
  mbw( o, L"W10clk - Moon phase" );
  R sprintf( s, "phase %.1f%%  visible %.1f%%  full %+.1f", 100.0*z, 100.0*v, fullm-j ); _

U
show_moon_phases( S s, U n ) __
  U year;
  if( sscanf( s, "%u", &year )!=1 ) R n;
  D k = y2lunation( year );
  D j2 = moon_phase_date( k, 2.0 );
  U n2 = j2n( j2 );
  U y2,m2,d2;
  n2ymd( n2, OUT &y2, &m2, &d2 );
  while( y2==year ) __
    k -= 1.0;
    j2 = moon_phase_date( k, 2.0 );
    n2 = j2n( j2 );
    n2ymd( n2, OUT &y2, &m2, &d2 ); _
  C o[2000]; U len=sprintf( o, "%s\n", "Lun# NewMoonJD FullMoonJD NewMoonDate FullMoonDate" );
  D j0; U y0,m0,d0;
  for( ;;) __ U n0;
    k += 1.0;
    j0 = moon_phase_date( k, 0.0 ); n0 = j2n( j0 ); n2ymd( n0, OUT &y0, &m0, &d0 );
    j2 = moon_phase_date( k, 2.0 ); n2 = j2n( j2 ); n2ymd( n2, OUT &y2, &m2, &d2 );
    if( y0!=year && y2!=year ) break;
    len += sprintf( o+len, "%d - %.4f %.4f - %u.%02u.%02u  %u.%02u.%02u\n", (int)k,
        j0, j2, y0,m0,d0, y2,m2,d2 ); _
  mb( o, "year" );
  R n;
_

U
current_time_in_many_forms( U c, S s, U mn, time_t t, U n ) __
  if( c=='j' ) R sprintf( s, "%12.4f", u2j(t) );
  struct tm* tmptr = localtime(&t);
  if( c=='u' ) R strftime( s, mn, "%s", tmptr ); // %s -- unix seconds
  if( c=='d' ) R strftime( s, mn, "%Y/%m/%d %a", tmptr );
  if( c=='y' ) R strftime( s, mn, "%j", tmptr ); // yday
  if( c=='t' ) R sprintf( s, "%u:%02u:%02u", tmptr->tm_hour, tmptr->tm_min, tmptr->tm_sec );
  if( c=='z' ) __
    if( !tz_is_set ) { tz_offset = gettz( &tz_h, &tz_m ); tz_is_set = true; }
    R tz_m==0 ? sprintf( s, "%+d", tz_h ) : sprintf( s, "%+d%02u", tz_h, tz_m ); _ R n; _

time_t
cvt_from_str( S s ) __ U y,m,d, h,mm,ss; struct tm tmv; tmv.tm_isdst = -1;
  if( sscanf( s, "%u/%u/%u.%u:%u:%u", &y,&m,&d, &h,&mm,&ss )!=6 ) R (time_t)-2;
  if( !(1970<=y && 1<=m && m<=12 && 1<=d && d<=31 && h<24 && mm<60 && ss<60 &&
        (y<2106 || y==2106 && (m<2 || m==2 && d<=7))) ) R (time_t)-1;
  tmv.tm_year = y-1900; tmv.tm_mon = m-1; tmv.tm_mday = d;
  tmv.tm_hour = h; tmv.tm_min = mm; tmv.tm_sec = ss;
  R mktime( &tmv ); _

U
cvt_unix_time( S s, U mn, U n ) __ L u;
  time_t t = cvt_from_str( s ); if( t == (time_t)-1 ) R n; // 6 nums, but wrong values
  if( t != (time_t)-2 ) R sprintf( s, "%u", (U)t );        // y/m/d.h:m:s
  if( sscanf( s, "%llu", &u )!=1 || u>=0x100000000ull ) R n; // limit is 2^32
  t = (time_t)u; struct tm* tmptr = localtime(&t);
  R strftime( s, mn, "%Y/%m/%d.%H:%M:%S %a", tmptr ); _

U
cvt_julian_day( S s, U mn, U n ) __ D j;
  time_t t = cvt_from_str( s ); if( t == (time_t)-1 ) R n;
  if( t != (time_t)-2 ) R sprintf( s, "%12.4f", u2j(t) );
  if( sscanf( s, "%lf", &j )!=1 ) R n;
  R fmtj( j, "%Y/%m/%d.%H:%M:%S", s, mn ); _

U
cvt_date( S s, U mn, time_t t, U n ) __ U y,m,d, x; I xd; struct tm* tmptr = localtime(&t);
  C diff[20] = "";
  U today = ymd2n( tmptr->tm_year+1900, tmptr->tm_mon+1, tmptr->tm_mday );
  if( strchr( s, '/' ) ) __ // YYYY/MM/DD or MM/DD
    I k = sscanf( s, "%u/%u/%u", &y, &m, &d );
    if( k==3 && 0<y && y<=10000 && 1<=m && m<=12 && 1<=d && d<=31 )
      x = ymd2n( y, m, d );
    else if( k==2 && 1<=y && y<=12 && 1<=m && m<=31 ) // y is m, m is d
      x = ymd2n( tmptr->tm_year+1900, y, m );
    else R n;
    if( x != today ) sprintf( diff, " %+d", (I)x - (I)today );
    R sprintf( s, "%u %s%s", x, WD[n2wd(x)], diff ); _
  if( s[0]=='+' || s[0]=='-' ) __
    if( sscanf( s, "%d", &xd )!=1 || -xd>=(I)today ) R n;
    x = (U)((I)today + xd); _
  else // NNNNN
    if( sscanf( s, "%u", &x )!=1 || x==0 ) R n; // don't do it for 0
  n2ymd( x, &y,&m,&d );
  if( x != today ) sprintf( diff, " %+d", (I)x - (I)today );
  R sprintf( s, "%d/%d/%d %s%s", y,m,d, WD[n2wd(x)], diff ); _

U
cvt_time( S s, U mn, U n ) __ U d,h,mm,ss;
  KS sign = (s[0]=='-') ? "-" : "";
  I k = sscanf( ((s[0]=='+' || s[0]=='-') ? s+1 : s), "%u:%u:%u:%u", &d,&h,&mm,&ss );
  if( k>1 ) __
    if( k==2 ) { ss=h; mm=d; h=0; d=0; }       //     M:S
    else if( k==3 ) { ss=mm; mm=h; h=d; d=0; } //   H:M:S
    I secs = 86400*d + 3600*h + 60*mm + ss;    // D:H:M:S
    R sprintf( s, "%s%d", sign, secs ); _
  // else just one number, seconds --> D:H:M:S
  ss = d%60; d /= 60;
  mm = d%60; d /= 60;
  h  = d%24; d /= 24;
  if( d != 0 ) R sprintf( s, "%s%u:%02u:%02u:%02u", sign, d, h, mm, ss );
  if( h != 0 ) R sprintf( s, "%s%u:%02u:%02u", sign, h, mm, ss );
  R sprintf( s, "%s%u:%02u", sign, mm, ss ); _

U
cvt_yearday( S s, U mn, time_t t, U n ) __ U y,m,d; struct tm* tmptr = localtime(&t);
  I k = sscanf( s, "%u/%u/%u", &y, &m, &d );
  if( k==3 ) // Y/M/D
    R sprintf( s, "%u/%u", y, ymd2n( y,m,d ) - ymd2n( y,1,1 ) + 1 );
  if( k==2 && (y<=12 && m<=31) ) __ // M/D current year
    d=m; m=y; y = tmptr->tm_year+1900;
    R sprintf( s, "%u", ymd2n( y,m,d ) - ymd2n( y,1,1 ) + 1 ); _
  if( k==2 && (y>12 || m>31) ) __ // y/yday --> y/m/d
    n2ymd( ymd2n( y,1,1 ) - 1 + m, &y,&m,&d );
    R sprintf( s, "%u/%u/%u", y, m, d ); _
  if( k==1 ) __ // yday (in y) --> m/d
    n2ymd( ymd2n( tmptr->tm_year+1900,1,1 ) - 1 + y, &y,&m,&d );
    n = sprintf( s, "%u/%u", m, d ); _                           R n; _

U
cvt_time_zone( S s, U mn, time_t t, U n ) __
  if( !tz_is_set ) { tz_offset = gettz( &tz_h, &tz_m ); tz_is_set = true; }
  if( s[0]=='.' ) { mb( g_tzlist+1, "W10clk - time zones" ); R 0; }
  if( 'A'<=s[0] && s[0]<='Z' ) __ // named time zone
    S tzn = malloc( n+3 );
    tzn[0] = ' '; strcpyupr( tzn+1, s ); tzn[n+1] = ' '; tzn[n+2]='\0'; // " TZN "
    S pos = strstr( g_tzlist, tzn ); I tzv;
    if( pos && sscanf( pos+n+1, "%d", &tzv )==1 ) n = sprintf( s, "%d", tzv );
    free( tzn ); _
  else if( s[0]=='+' || s[0]=='-' || '0'<=s[0] && s[0]<='9' ) __ I z; // number N +N -N
    if( sscanf( s, "%d", &z )==1 ) __
      U m = 0; U h = (U)(z>=0 ? z : -z); if( h>=15 ) { m=h%100; h=h/100; }
      I seconds = 3600*h + 60*m; if( z<0 ) seconds = -seconds;
      time_t new_t = t - 60*tz_offset + seconds;
      struct tm* tmptr = localtime(&new_t);
      n = strftime( s, mn, "%H:%M:%S %a %m/%d", tmptr ); _ _
  R n; _

U
process_char( U c, S s, U mn, U n ) __ // mn - max length
  time_t t; D x; L h;
  struct timeb tb;
  O time_t t_start;       O I t_start_ms;
  O I      t_idle;        O I t_idle_ms;
  O time_t t_break_start; O I t_break_start_ms;
  if( '0'<=c && c<='9' || 'A'<=c && c<='Z' || memchr( "+*-/%^o\\&|!.:()#", c, 16 ) ) __
    s[n]=(C)c; if(n<mn) ++n; s[n]=0; _
  else if( c==32 || c==27 ) __ // space escape
    n=0; s[n]=0; _
  else if( c=='\b' ) __ // bksp
    if( n>0 ) --n; s[n]=0; _
  else if( c=='\t' ) __ // tab
    g_seconds = ! g_seconds; _
  else if( c=='u' || c=='d' || c=='t' || c=='z' || c=='y' || c=='j' || c=='m' ) __
    t = time(NULL);
    if( n==0 )
      if( c=='m' ) n = current_moon( s, mn, (D)t, n );
      else         n = current_time_in_many_forms( c, s, mn, t, n );
    else
      if( c=='u' )      n = cvt_unix_time( s, mn, n );
      else if( c=='d' ) n = cvt_date( s, mn, /* today */ t, n ); // NNNNNN or YYYY/MM/DD
      else if( c=='t' ) n = cvt_time( s, mn, n );
      else if( c=='y' ) n = cvt_yearday( s, mn, t, n ); // YD -> M/D this year or Y/M/D --> YD
      else if( c=='z' ) n = cvt_time_zone( s, mn, t, n );
      else if( c=='m' ) n = show_moon_phases( s, n );
      else n = cvt_julian_day( s, mn, n ); _ // 'j' -- yyyy/mm/dd.hh:mm:ss to jd or back
  else if( c=='f' ) __ // F --> C
    if( sscanf( s, "%lf", &x )==1 ) { n = sprintf( s, "%.1f%cC", (x-32.0)/1.8, 176 ); } _
  else if( c=='c' ) __ // C --> F
    if( sscanf( s, "%lf", &x )==1 ) { n = sprintf( s, "%.1f%cF", (x*1.8)+32.0, 176 ); } _
  else if( c=='k' ) __ // Ki --> k
    if( sscanf( s, "%lf", &x )==1 ) { n = sprintf( s, "%g", x*1.024 ); } _
  else if( c=='h' ) __ // help | decimal --> XXXX (hexadecimal)
    if( sscanf( s, "%llu", &h )==1 ) n = sprintf( s, "#%llX", h ); _
  else if( c=='=' || c==13 ) __ // = or enter -- calculate expression
    if( calculate( s, &x ) == 0 ) n = sprintf( s, "%.15g", x ); _
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
