// Part of w10clk

// gcc -O2 -DSEPARATE_TEST w10clk_tjd.c -o test.exe

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/timeb.h>
#include <unistd.h>

// TJD calculations. Good at least for dates from 0001/01/01 to 3000/12/31
// ... Julian Thursday, 4 October 1582, being followed by Gregorian Friday, 15 October 1582

int
ymd2dn( int year, int month, int day )
{
  // Convert date to TJD number. Month and day may be quite wild.
  // Years BC: 0 = 1 BC, -1 = 2 BC, -2 = 3 BC, etc
  int b = 0;
  if( month > 12 )
  {
    year = year + month/12;
    month = month%12;
  }
  else if( month < 1 )
  {
    month = -month;
    year = year - month/12 - 1;
    month = 12 - month%12;
  }
  int yearCorr = year > 0 ? 0 : 3;
  if( month < 3 )
  {
    year -= 1;
    month += 12;
  }
  if( year*10000 + month*100 + day > 15821014 )
    b = 2 - year/100 + year/400;
  return (1461*year - yearCorr)/4 + 306001*(month + 1)/10000 + day + b - 719006;
}

int dn2wd( int dn ) { return dn >= 0 ? (dn+5) % 7 : (-dn+2) % 7; }

void
dn2ymd( int dn, /* OUT */ struct tm* t ) // t will have its wicked values!
{
  // Convert TJD day number to tuple year, month, day and weekday"
  // Probably needs some correction for dates before 1.1.1
  int jdi = dn + 2440000;
  int b;
  if( jdi < 2299160 ) // (1582, 10, 15)
    b = jdi + 1525;
  else
  {
    int alpha = (4*jdi - 7468861) / 146097;
    b = jdi + 1526 + alpha - alpha/4;
  }
  int c = (20*b - 2442) / 7305;
  int d = 1461*c/4;
  int e = 10000*(b - d)/306001;
  int day = b - d - 306001*e/10000;
  int month = e - (e < 14 ? 1 : 13);
  int year = c - (month > 2 ? 4716 : 4715);
  t->tm_year = year-1900;   // since 1900
  t->tm_mon = month-1;      // 0..11
  t->tm_mday = day;         // 1..31
  t->tm_wday = dn2wd( dn ); // 0..6
}

#ifdef SEPARATE_TEST
int
main()
{
  int y,m,d,j,p,i,w; struct tm x;
  i = 0; // p not initialized
  for( y=1; y<=3000; ++y )
    for( m=1; m<=12; ++m )
      for( d=1; d<=31; ++d )
      {
        // skip non-existing days
        if( d>30 && (m==4 || m==6 || m==9 || m==11) ) continue;
        int l = (y<=1582) ? (y%4==0) : ((y%4==0) && (y%100!=0 || y%400==0));
        if( m==2 && d>(l?29:28) ) continue;
        if( y==1582 && m==10 && 4<d && d<15 ) continue;
        j = ymd2dn( y, m, d );
        if( !i )
        { i = 1;
          p = j-1;
          printf( "started: %d/%d/%d --> %d\n", y,m,d, j );
        }
        // make sure we have next day
        if( j != p+1 )
          printf( "Error in sequence: %d/%d/%d --> %d instead of %d\n", y,m,d, j, p+1 );
        dn2ymd( j, &x );
        if( x.tm_year+1900 != y || x.tm_mon+1 != m || x.tm_mday != d )
          printf( "Error rev.conv.: %d/%d/%d --> %d --> %d/%d/%d\n", y,m,d, j,
            x.tm_year+1900, x.tm_mon+1, x.tm_mday );
        p = j;
      }
  printf( "ended: %d/%d/%d --> %d\n", x.tm_year+1900, x.tm_mon+1, x.tm_mday, j );
  // started: 1/1/1 --> -718577
  // ended: 3000/12/31 --> 377151
  i = 0;
  for( j=-718577; j<=377151+100000; ++j )
  {
    dn2ymd( j, &x );
    if( i==0 ) // skip the very first run
      i = 1;
    else
      if( x.tm_year+1900 == y && x.tm_mon+1 == m && x.tm_mday == d+1 || // next day
          x.tm_year+1900 == y && x.tm_mon+1 == m+1 && x.tm_mday == 1 || // next month
          x.tm_year+1900 == y+1 && x.tm_mon+1 == 1 && x.tm_mday == 1 ||  // next year
          x.tm_year+1900 == y && x.tm_mon+1 == m && y==1582 && m==10 && d==4 && x.tm_mday==15 )
        {} // OK
      else
        printf( "Error: not next day: %d/%d/%d --> %d --> %d/%d/%d\n",
            y,m,d, j, x.tm_year+1900, x.tm_mon+1, x.tm_mday );
    y = x.tm_year+1900; m = x.tm_mon+1; d = x.tm_mday;
    int jj = ymd2dn( y, m, d );
    if( jj != j )
      printf( "Error rev.conv.: %d --> %d/%d/%d --> %d\n", j, y,m,d, jj );
  }
  // now some selected dates
  if( ymd2dn( 1582,10, 4 ) != -140841 ) printf( "error 1582,10,04" ); // end of Gregorian calendar
  if( ymd2dn( 1582,10,15 ) != -140840 ) printf( "error 1582,10,15" ); // start of Julian calendar
  if( ymd2dn( 1694, 8, 9 ) !=  -99999 ) printf( "error 1694,08,09" );
  if( ymd2dn( 1878, 9, 5 ) !=  -32768 ) printf( "error 1878,09,05" );
  if( ymd2dn( 1965, 8, 5 ) !=   -1023 ) printf( "error 1965,08,05" );
  if( ymd2dn( 1968, 5,24 ) !=       0 ) printf( "error 1968,05,24" ); // start of TJD
  if( ymd2dn( 1968, 5,25 ) !=       1 ) printf( "error 1968,05,25" );
  if( ymd2dn( 1970, 1, 1 ) !=     587 ) printf( "error 1970,01,01" );
  if( ymd2dn( 1995,10, 9 ) !=    9999 ) printf( "error 1995,10,09" );
  if( ymd2dn( 2005,12,31 ) !=   13735 ) printf( "error 2005,12,31" );
  if( ymd2dn( 2006, 8,18 ) !=   13965 ) printf( "error 2006,08,18" );
  if( ymd2dn( 2013, 4,01 ) !=   16383 ) printf( "error 2013,04,01" );
  if( ymd2dn( 2023, 2,24 ) !=   19999 ) printf( "error 2023,02,24" );
  if( ymd2dn( 2038, 1,18 ) !=   25441 ) printf( "error 2038,01,18" );
  if( ymd2dn( 2058, 2, 8 ) !=   32767 ) printf( "error 2058,02,08" );
  if( ymd2dn( 2147,10,28 ) !=   65535 ) printf( "error 2147,10,28" );
  if( ymd2dn( 2242, 3, 8 ) !=   99999 ) printf( "error 2242,03,08" );
  return 0;
}
#endif
