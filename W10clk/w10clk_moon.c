// Moon phase ------------------------------------------------------------------------

#include <math.h>
#include "../_.h"

#define sind(x) (sin((x)*(M_PI/180.0)))
#define cosd(x) (cos((x)*(M_PI/180.0)))

D t2lunation( D t ) { R floor( (t/(864e2*365.25)+70.0)*12.3685-284.0 ); }
D y2lunation( D y ) { R floor( (y-1900.0)*12.3685-284.0 ); }

D
moon_phase_date( D lunation, D phase ) __
  /*
  official lunations are counted from 1923 http://en.wikipedia.org/wiki/Lunation_Number
  -- Lunation 1 occurred at approximately 02:41 UTC, January 17, 1923
  so please correct the k argument by 284:
  Lun   k  new-moon-jd  full-moon-jd
  ...   0  2415021.0767 2415035.2961  1900.01.01-13:50 1900.01.15-19:06
  ...   1  2415050.5567 2415065.0757  1900.01.31-01:22 1900.02.14-13:49
    0 284  2423407.0141 2423422.6071  1922.12.18-12:20 1923.01.03-02:34
    1 285  2423436.6123 2423452.1624  1923.01.17-02:42 1923.02.01-15:54
  result is +- 2 minutes from http://www.timeanddate.com/calendar/moonphases.html
  */

  D k = lunation + 284 + phase/4.0; // 284 more than Ernest William Brown lun.nr.
  D t  = k/1236.85; // time in 'century' units
  D t2 = t*t;
  D t3 = t*t*t;

  D j = (2415020.75933  // 2415020 = 1900/1/0.5
    + 29.53058868*k // or 29.530588861 or 29.530587981 ...
    + 0.0001178*t2 - 0.000000155*t3
    + 0.00033*sind( 166.56 + 132.87*t - 0.009173*t2 ) );
    // -= (0.000837*T + 0.000335*t2) ???

  D sa = fmod(359.2242    // sun anomaly
    + 29.10535608*k // was M0 = k*0.08084821133; M0 = 360*(M0 - INT(M0))
    - 0.0000333*t2
    - 0.00000347*t3, 360.0);

  D ma = fmod(306.0253   // moon anomaly
    + 385.81691806*k // was M1 = k*0.07171366128; M1 = 360*(M1 - INT(M1))
    + 0.0107306*t2
    + 0.00001236*t3, 360.0);

  D ml = fmod(21.2964    // moon latitude
    + 390.67050646*k // was B1 = k*0.08519585128; B1 = 360*(B1 - INT(B1))
    - 0.0016528*t2
    - 0.00000239*t3, 360.0);

  if( phase==0 || phase==2 )
    j += ( (0.1734 - 0.000393*t) * sind(sa)
      + 0.0021 * sind( 2*sa )
      - 0.4068 * sind( ma )
      + 0.0161 * sind( 2*ma )
      - 0.0004 * sind( 3*ma )
      + 0.0104 * sind( 2*ml )
      - 0.0051 * sind( sa+ma )
      - 0.0074 * sind( sa-ma )
      + 0.0004 * sind( 2*ml + sa )
      - 0.0004 * sind( 2*ml - sa )
      - 0.0006 * sind( 2*ml + ma )
      + 0.0010 * sind( 2*ml - ma )
      + 0.0005 * sind( 2*ma + sa ) );
  else
    j += ( (0.1721 - 0.0004*t) * sind(sa)
      + 0.0021 * sind( 2*sa )
      - 0.6280 * sind( ma )
      + 0.0089 * sind( 2*ma )
      - 0.0004 * sind( 3*ma )
      + 0.0079 * sind( 2*ml )
      - 0.0119 * sind( sa+ma )
      - 0.0047 * sind( sa-ma )
      + 0.0003 * sind( 2*ml + sa )
      - 0.0004 * sind( 2*ml - sa )
      - 0.0006 * sind( 2*ml + ma )
      + 0.0021 * sind( 2*ml - ma )
      + 0.0003 * sind( 2*ma + sa )
      + 0.0004 * sind( sa - 2*ma )
      - 0.0003 * sind( 2*sa + ma ) );

  D adj = 0.0028 - 0.0004 * cosd( sa ) + 0.0003 * cosd( ma ); // for 1Q and 3Q

  if( phase==1 ) j += adj;
  else if( phase==3 ) j -= adj;   // was "==2", I guess it should be 3

  return j; _

#ifdef MAKEMAIN
#include <stdio.h>
#include <stdlib.h>
I main( I ac, KS av[] ) __
  /*
  def j2s(j,tz):
    n = tjd.jdn2dn( j, tz )
    o = math.floor( n )
    y,m,d = tjd.dn2ymd( o )
    h,n,_ = tjd.d2hms( n-o+0.5/1440 )
    return "%04d.%02d.%02d-%02d:%02d"%(y,m,d,h,n)
  def j2S(j,tz):
    n = tjd.jdn2dn( j, tz )
    o = math.floor( n )
    y,m,d = tjd.dn2ymd( o )
    p = "abcdefgh"[math.floor(8*(n-o))]
    m = "-JFM45678SOND"[m]
    return "%s%d%s"%(m,d,p)
  */
  if( ac!=3 && ac!=4 ) __
    printf( "%s %s\n%s\n%s\n", av[0], "lunation_from lunation_to [tz_in_hrs]",
        "will show table: (for official lunation subtract284: #1 - 1923/1/17",
        "lunation new-moon-jd full-moon-jd new-moon-date full-moon-date" );
    exit(0); _
  I k1 = atoi( av[1] );
  I k2 = atoi( av[2] );
  for( I k=k1; k<=k2; ++k ) __
    D n = moon_phase_date( k, 0 ); // was k+284
    D f = moon_phase_date( k, 2 ); // was k+284
    /*
    NM = j2S(n,z); FM = j2S(f,z)
    if FM[0]==NM[0]: FM=FM[1:]
    print( "%3d  %.4f %.4f  %s %s  %s/%s" % (k,n,f,j2s(n,z),j2s(f,z),NM,FM) )
    */
    printf( "%4d  %12.4f %12.4f\n", k, n, f ); _
  R 0; _
#endif
