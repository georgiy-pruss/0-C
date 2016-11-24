// Part of w10clk -- Evaluate expression
// Based on Henrik's (http://stackoverflow.com/users/148897/henrik) answer here:
// http://stackoverflow.com/questions/9329406/evaluating-arithmetic-expressions-from-string-in-c
// Well, I added some features there too

#include <string.h>
#include <math.h>
#include "../_.h"
typedef unsigned int uint;

#define BLANKS " \t\n\r\v\f"
enum ERRORS {E_OK=0, E_CLOSE,E_ZDIV,E_ZMOD,E_NEG,E_NEGLOG,E_CIRC,E_BINOP,E_HEXDIGIT};
static uint err_flag;
static char* curr_pos;
// curr_pos += strspn( curr_pos, BLANKS ); // we don't have blanks, so no skip blanks

static uint peek() { return *curr_pos; }
static uint get() { return *curr_pos++; }

static double expression(); // declaration for recursion

static double number() __
  double result = get() - '0';
  while('0'<=peek() && peek() <= '9') result = 10*result + get() - '0';
  if( peek()=='.' ) __
    get();
    double div=1.0;
    while( '0'<=peek() && peek() <= '9' )
      result += (get() - '0') / (div *= 10.0); _
  return result; _

static uint hexdigit( int c ) __
  if( '0'<=c && c<='9' ) return c-'0';
  if( 'A'<=c && c<='F' ) return c-'A'+10;
  err_flag=E_HEXDIGIT; // just for a case. e.g. right after '#'
  return 0; _

static double hexnumber() __
  get(); // '#'
  double result = hexdigit(get());
  while('0'<=peek() && peek() <= '9' || 'A'<=peek() && peek() <= 'F')
    result = 16*result + hexdigit(get());
  return result; _

static double cubrt( double x ) __
  if( x<0.0 ) return -cubrt(-x);
  return pow(x,1.0/3.0); _

static double factorial( double x ) __
  if( x<0.0 ) { err_flag=E_NEG; return x; }
  uint n = (uint)x;
  x = 1.0; for( uint p=2; p<=n; ++p ) x *= p; return x; _

static double perm( double x, double y ) __
  if( x<0.0 || y<0.0) { err_flag=E_NEG; return x; }
  uint n = (uint)x; uint k = (uint)y;
  if( k>n ) { uint nn = n; n = k; k = nn; } // now k<=n
  x = 1.0; for( uint p=n-k+1; p<=n; ++p ) x *= p; return x; _

static double factor() __
  int c = peek();
  if( c=='#' ) return hexnumber();
  if( '0'<=c && c <= '9') return number();
  if(c == '(') __
    get(); // '('
    double result = expression();
    if(peek() != ')') { err_flag=E_CLOSE; return 0; }
    get(); // ')'
    return result; _
  if(c == '-') { get(); return -factor(); }
  if(c == '*') { get(); double x=factor(); return x*x; }
  if(c == '/') { get(); double x=factor(); if(x<0) err_flag=E_NEG; else x=sqrt(x); return x; }
  if(c == '\\'){ get(); return cubrt(factor()); }
  if(c == '^') { get(); return exp(factor()); }
  if(c == 'o') { get(); return M_PI*factor(); }
  if(c == '%') { get(); double x=factor(); if(x<=0) err_flag=E_NEGLOG; else x=log(x); return x; }
  if(c == '|') { get(); double x=factor(); if(x<=0) err_flag=E_NEGLOG; else x=log10(x); return x; }
  if(c == '&') { get(); double x=factor(); if(x<=0) err_flag=E_NEGLOG; else x=log(x)/log(2.0); return x; }
  if(c == '!') { get(); return factorial(factor()); }
  if(c == '+') { get(); return factor(); }
  err_flag=E_BINOP;
  return 0; _

static double power() __
  double x = factor();
  while( peek() == '^' ) { get(); x = pow( x, factor() ); }
  return x; _

static double term() __
  double x = power();
  while( memchr( "*/%|\\o&!", peek(), 8 ) ) __
    int c = get();
    double y = power();
    if(c == '*') x *= y;
    else if(c=='/') { if(y==0.0) err_flag=E_ZDIV; else x /= y; }
    else if(c=='%') { if(y==0.0) err_flag=E_ZMOD; else x -= y*floor(x/y); }
    else if(c=='|') { if(x+y==0.0) err_flag=E_ZDIV; else x = x*y/(x+y); }
    else if(c=='\\') { if(x==0.0) err_flag=E_ZDIV; else x = y/x; }
    else if(c=='&') { x = sqrt(x*x + y*y); }
    else if(c=='!') { x = perm(x,y); }
    else __ // 'o' -- circuit functions
      if( x==1 ) return sin(y);   if( x==10 ) return sin(y*M_PI/180.0);
      if( x==2 ) return cos(y);   if( x==20 ) return cos(y*M_PI/180.0);
      if( x==3 ) return tan(y);   if( x==30 ) return tan(y*M_PI/180.0);
      if( x==-1 ) return asin(y); if( x==-10 ) return asin(y)*180.0/M_PI;
      if( x==-2 ) return acos(y); if( x==-20 ) return acos(y)*180.0/M_PI;
      if( x==-3 ) return atan(y); if( x==-30 ) return atan(y)*180.0/M_PI;
      err_flag = E_CIRC; _ _
  return x; _

static double expression() __
  double result = term();
  while( peek() == '+' || peek() == '-' )
    if( get() == '+' ) result += term(); else result -= term();
  return result; _

static void trim( char* s ) __ int i;
  for( i=strlen(s)-1; i>=0; --i ) if( strchr( BLANKS, s[i] )==NULL ) break;
  s[i+1] = '\0'; _

uint calculate( char* s, double* r ) __
  trim(s); // trim trailing blanks although it's not so neccessary
  curr_pos = s  +strspn( s, BLANKS ); // skip leading blanks
  err_flag = E_OK;
  *r = expression();
  return err_flag; _
