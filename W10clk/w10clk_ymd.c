// Part of w10clk -- day number calculations
// gcc -O2 -DTESTALL w10clk_ymd.c -o test.exe && test.exe

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/timeb.h>
#include <unistd.h>
#include "../_.h"

// Common Era Days. January 1, 1 AD is day 1. For simplicity, no negative days.
// As from https://en.wikipedia.org/wiki/Gregorian_calendar ... papal bull
// Inter gravissimas dated 24 February 1582, specified: "Julian Thursday,
// 4 October 1582, being followed by Gregorian Friday, 15 October 1582"
// That is, from 1/1/1 to 1582/10/4 the Julian calendar is used (days 1..), and
// from 1582/10/15 for at least a few thousand years the Gregorian calendar is used.
// CED 678578 = Modified Julian Day 0 (difference is 678578). And TJD+718578

U
ymd2n( U year, U month, U day ) __
  // Convert date to CED number.
  // Year >= 1. Result is >0, or 0 if there's some error.
  if( year==0 || month==0 || day==0 ) R 0;
  if( month > 12 ) { year = year + month/12; month = month%12; }
  if( month < 3 ) { year -= 1; month += 12; }
  I b = year*10000 + month*100 + day > 15821014 ? 2 - year/100 + year/400 : 0;
  R 1461*year/4 + 306001*(month + 1)/10000 + day + b - 428; _

U
n2wd( U dn ) { R (dn+5) % 7; }

V
n2ymd( U dn, OUT U* yy, U* mm, U* dd ) __
  // Convert TJD day number to tuple year, month, day and weekday"
  // Probably needs some correction for dates before 1/1/1
  I jdi = dn + 1721422; I b;
  if( jdi < 2299160 ) // (1582, 10, 15)
    b = jdi + 1525;
  else __
    I alpha = (4*jdi - 7468861) / 146097;
    b = jdi + 1526 + alpha - alpha/4; _
  I c = (20*b - 2442) / 7305;
  I d = 1461*c/4;
  I e = 10000*(b - d)/306001;
  I day = b - d - 306001*e/10000;
  I month = e - (e < 14 ? 1 : 13);
  I year = c - (month > 2 ? 4716 : 4715);
  *yy = year; *mm = month; *dd = day; _

I
gettz( I* h, U* m ) __
  // return offset in minutes, also set h to hours (+-12) and m to abs minutes
  I u_mins,u_date,l_mins,l_date,offset;
  time_t currtime = time(NULL);
  struct tm* ptm = gmtime(&currtime);
  u_mins = ptm->tm_hour*60+ptm->tm_min;
  u_date = ptm->tm_year*1024+ptm->tm_mon*32+ptm->tm_mday; // artificial date number
  ptm = localtime(&currtime);                             // we use such "dates" just
  l_mins = ptm->tm_hour*60+ptm->tm_min;                   // to see if they
  l_date = ptm->tm_year*1024+ptm->tm_mon*32+ptm->tm_mday; // change, then correct
  offset = l_mins - u_mins;                               // offset accordingly
  if( u_date < l_date )
    offset += 24*60;
  else if( u_date > l_date )
    offset -= 24*60;
  I h_o, m_o;
  if( offset >= 0 ) { h_o = offset/60; m_o = offset - 60*h_o; }
  else { h_o = (-offset)/60; m_o = (-offset) - 60*h_o; h_o = -h_o; }
  if( h ) *h = h_o; if( m ) *m = m_o;
  R offset; _

/*
Return approx moon phase, in days. Error is +-0.3 day, max +-0.6 day.
jdn = dn2jdn( current_dn() + hms2d(*current_hms()), current_tz() )
return (jdn-2449128.59)%DN_MOON   ## DN_MOON = 29.53058867 # lunar synodic month
mp = jdn2moonphase(j)
np = DN_MOON-mp
mph = nph = ""
if mp<1: mph = "/%.1fh"%(mp*24)
if np<1: nph = "/%.1fh"%(np*24)
print("%.1f%% %.2f%s (%.2f%s)"%(mp*100/DN_MOON,mp,mph,np,nph))
*/

#ifdef TESTALL
I
main() __
  U y,m,d,ced,cc,xy,xm,xd,w;
  ced = 1;  w = 6; // it all starts with Saturday
  for( y=1; y<=4000; ++y )
    for( m=1; m<=12; ++m )
      for( d=1; d<=31; ++d ) __
        // skip non-existing days
        if( d>30 && (m==4 || m==6 || m==9 || m==11) ) continue;
        I l = (y<=1582) ? (y%4==0) : ((y%4==0) && (y%100!=0 || y%400==0));
        if( m==2 && d>(l?29:28) ) continue;
        if( y==1582 && m==10 && 4<d && d<15 ) continue;
        cc = ymd2n( y, m, d );
        if( ced==1 )
          printf( "ymd->ced->ymd started: %d/%d/%d --> %d\n", y,m,d, ced );
        // make sure we have next day
        if( cc != ced )
          printf( "Error in sequence: %d/%d/%d --> %d instead of %d\n", y,m,d, cc, ced );
        n2ymd( cc, &xy,&xm,&xd );
        if( xy != y || xm != m || xd != d )
          printf( "Error rev.conv.: %d/%d/%d --> %d --> %d/%d/%d\n", y,m,d, cc, xy,xm,xd );
        if( n2wd( cc ) != w ) printf( "Error in n2wd: %d --> %d != %d\n", cc, n2wd( cc ), w );
        ++ced; w = (w+1) % 7; _
  printf( "ymd->ced->ymd ended: %d/%d/%d --> %d\n", xy, xm, xd, cc );
  // ymd->ced->ymd started: 1/1/1 --> 1
  // ymd->ced->ymd ended: 4000/12/31 --> 1460972
  for( ced=1; ced<=1500000; ++ced ) __
    n2ymd( ced, &xy,&xm,&xd );
    if( ced==1 ) __
      if( xy!=1 || xm!=1 || xd!=1 )
        printf( "Wrong day 1: %d %d %d\n", xy, xm, xd );
      printf( "ced->ymd->ced started: %d --> %d/%d/%d\n", ced, xy,xm,xd ); _
    else __
      if( !(xy == y && xm == m && xd == d+1 || // next day
            xy == y && xm == m+1 && xd == 1 || // next month
            xy == y+1 && xm == 1 && xd == 1 || // next year
            xy == y && xm == m && y==1582 && m==10 && d==4 && xd==15) )
        printf( "Error: not next day: %d/%d/%d --> %d --> %d/%d/%d\n",
            y,m,d, ced, xy, xm, xd ); _
    cc = ymd2n( xy,xm,xd );
    if( cc != ced )
      printf( "Error rev.conv.: %d --> %d/%d/%d --> %d\n", ced, xy,xm,xd, cc );
    y = xy; m = xm; d = xd; _ // save the day to serve as previous day in next loop
  printf( "ced->ymd->ced ended: %d --> %d/%d/%d\n", cc, xy,xm,xd );
  // ced->ymd->ced started: 1 --> 1/1/1
  // ced->ymd->ced ended: 1500000 --> 4107/11/9
  // now some selected dates
  #define TEST(y,m,d,n,wd) if( ymd2n(y,m,d)!=n ) \
    printf( "error: %d/%d/%d --> %d != %d\n", y,m,d, ymd2n(y,m,d), n ); \
    n2ymd(n,&xy,&xm,&xd); \
    if( xy!=y||xm!=m||xd!=d ) \
    printf( "error: %d --> %d/%d/%d != %d/%d/%d\n", n, xy,xm,xd, y,m,d ); \
    if( n2wd(n)!=wd ) printf( "%d/%d/%d - %d --> %d != %d\n", y,m,d,n,n2wd(n),wd )
  TEST(    1, 1, 1, 1, 6 ); // 6
  TEST( 1582,10, 4, 718578-140841, 4 ); // 4 end of Julian calendar
  TEST( 1582,10,15, 718578-140840, 5 ); // 5 start of Gregorian calendar
  TEST( 1694, 8, 9, 718578 -99999, 1 );
  TEST( 1878, 9, 5, 718578 -32768, 4 );
  TEST( 1965, 8, 5, 718578  -1023, 4 );
  TEST( 1968, 5,24, 718578     +0, 5 ); // start of TJD
  TEST( 1968, 5,25, 718578     +1, 6 );
  TEST( 1970, 1, 1, 718578   +587, 4 );
  TEST( 1995,10, 9, 718578  +9999, 1 );
  TEST( 2005,12,31, 718578 +13735, 6 );
  TEST( 2006, 8,18, 718578 +13965, 5 );
  TEST( 2013, 4,01, 718578 +16383, 1 );
  TEST( 2023, 2,24, 718578 +19999, 5 );
  TEST( 2038, 1,18, 718578 +25441, 1 );
  TEST( 2058, 2, 8, 718578 +32767, 5 );
  TEST( 2147,10,28, 718578 +65535, 6 );
  TEST( 2242, 3, 8, 718578 +99999, 2 );
  TEST(  274,10,14, 100000, 3 );
  TEST( 1001, 1, 1, 365251, 3 );
  TEST( 1369,12, 4, 500000, 2 );
  TEST( 1582,10, 4, 577737, 4 );
  TEST( 1582,10,15, 577738, 5 );
  TEST( 1700, 1, 1, 620550, 5 );
  TEST( 1800, 1, 1, 657074, 3 );
  TEST( 1858,11,17, 678578, 3 ); // MJD 0 = JD 2400000.5
  TEST( 1900, 1, 1, 693598, 1 );
  TEST( 1901, 1, 1, 693963, 2 );
  TEST( 1917, 7,13, 700000, 5 );
  TEST( 1946,12,26, 710758, 4 ); // MJD 32180 = JD 2432180.5
  TEST( 1965, 8, 5, 717555, 4 );
  TEST( 1966,10,24, 718000, 1 );
  TEST( 1968, 5,24, 718578, 5 ); // TJD 0
  TEST( 1969, 7,20, 719000, 0 );
  TEST( 1970, 1, 1, 719165, 4 );
  TEST( 1972, 4,15, 720000, 6 );
  TEST( 2000, 1, 1, 730122, 6 ); // MJD 51544 = JD 2451544.5
  TEST( 2001, 1, 1, 730488, 1 );
  TEST( 2013, 1, 1, 734871, 2 );
  TEST( 2016,11,11, 736281, 5 );
  TEST( 2038, 1,19, 744020, 2 );
  TEST( 2050, 1, 1, 748385, 6 );
  TEST( 2100,12,31, 767011, 5 );
  TEST( 2054, 6, 4, 750000, 4 );
  TEST( 2191, 4,27, 800000, 3 );
  TEST( 2738,11,25, 999999, 5 );
  #define PYMD(y,m,d) printf( "%d/%d/%d --> %d\n", y,m,d, ymd2n(y,m,d) )
  #define PCED(n) n2ymd(n,&y,&m,&d),printf( "%d --> %d/%d/%d\n", n,y,m,d )
  /*
  PYMD( 1582,10, 4 ); // 577737
  PYMD( 1582,10,15 ); // 577738
  PCED( 577737 ); // 1582/10/4
  PCED( 577738 ); // 1582/10/15
  */
  R 0; _
#endif
