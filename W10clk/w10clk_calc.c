// Part of w10clk -- Evaluate expression
// Based on Henrik's (http://stackoverflow.com/users/148897/henrik) answer here:
// http://stackoverflow.com/questions/9329406/evaluating-arithmetic-expressions-from-string-in-c
// Well, I added some features there too

#include <string.h>
#include <math.h>
#include "../_.h"

#define BLANKS " \t\n\r\v\f"
enum ERRORS {E_OK=0, E_CLOSE,E_ZDIV,E_ZMOD,E_NEG,E_NEGLOG,E_CIRC,E_BINOP,E_HEXDIGIT,E_COLON};
KS error_msg( U e ) __
  O KS msg[] = { "No error", "No closing parenthesis", "Zero division", "Zero modulo",
    "Negative argument", "Neg. or zero in log", "Error circular fn", "Invalid binary op",
    "Error hex. number", "Error colon number"};
  R msg[e]; _
O U err_flag;
O S curr_pos;
// curr_pos += strspn( curr_pos, BLANKS ); // we don't have blanks, so no skip blanks

O U peek() { R *curr_pos; }
O U get() { R *curr_pos++; }

O D expression(); // declaration for recursion

O D number() __
  D parts[3] = {0.0}; // parts before last colon: AA:BB:CC:
  U nparts = 0;       // count of parts saved
  D result;
  for(;;) __ // for up to three colons for AA:BB:CC:DD
    result = get() - '0';
    while('0'<=peek() && peek()<='9') result = 10*result + get() - '0';
    if( peek()=='.' ) __
      get();
      D div=1.0;
      while( '0'<=peek() && peek()<='9' ) result += (get() - '0') / (div *= 10.0);
      break; _
    else if( peek()==':' ) __
      get();
      if( nparts>=3 ) { err_flag=E_COLON; R 0; } // too many colons xx:xx:xx:xx:
      if( !('0'<=peek() && peek()<='9') ) { err_flag=E_COLON; R 0; }
      parts[nparts] = result;
      ++nparts;
      result = 0; _
    else break; _ // not '.' and not ':'
  if( nparts==1 ) result += parts[0]*60.0;
  else if( nparts==2 ) result += parts[0]*3600.0 + parts[1]*60.0;
  else if( nparts==3 ) result += parts[0]*86400.0 + parts[1]*3600.0 + parts[2]*60.0;
  R result; _

O U hexdigit( int c ) __
  if( '0'<=c && c<='9' ) R c-'0';
  if( 'A'<=c && c<='F' ) R c-'A'+10;
  err_flag=E_HEXDIGIT; // just for a case. e.g. right after '#'
  R 0; _

O D hexnumber() __
  get(); // '#'
  D result = hexdigit(get());
  while('0'<=peek() && peek()<='9' || 'A'<=peek() && peek()<='F')
    result = 16*result + hexdigit(get());
  R result; _

O D cubrt( D x ) __
  if( x<0.0 ) R -cubrt(-x);
  R pow(x,1.0/3.0); _

O D factorial( D x ) __
  if( x<0.0 ) { err_flag=E_NEG; R x; }
  U n = (U)x;
  x = 1.0; for( U p=2; p<=n; ++p ) x *= p; R x; _

O D perm( D x, D y ) __
  if( x<0.0 || y<0.0) { err_flag=E_NEG; R x; }
  U n = (U)x; U k = (U)y;
  if( k>n ) { U nn = n; n = k; k = nn; } // now k<=n
  x = 1.0; for( U p=n-k+1; p<=n; ++p ) x *= p; R x; _

O D factor() __
  int c = peek();
  if( c=='#' ) R hexnumber();
  if( '0'<=c && c<='9') R number();
  if(c == '(') __
    get(); // '('
    D result = expression();
    if(peek() != ')') { err_flag=E_CLOSE; R 0; }
    get(); // ')'
    R result; _
  if(c == '-') { get(); R -factor(); }
  if(c == '*') { get(); D x=factor(); R x*x; }
  if(c == '/') { get(); D x=factor(); if(x<0) err_flag=E_NEG; else x=sqrt(x); R x; }
  if(c == '\\'){ get(); R cubrt(factor()); }
  if(c == '^') { get(); R exp(factor()); }
  if(c == 'o') { get(); R M_PI*factor(); }
  if(c == '%') { get(); D x=factor(); if(x<=0) err_flag=E_NEGLOG; else x=log(x); R x; }
  if(c == '|') { get(); D x=factor(); if(x<=0) err_flag=E_NEGLOG; else x=log10(x); R x; }
  if(c == '&') { get(); D x=factor(); if(x<=0) err_flag=E_NEGLOG; else x=log(x)/M_LN2; R x; }
  if(c == '!') { get(); R factorial(factor()); }
  if(c == '+') { get(); R factor(); }
  err_flag=E_BINOP;
  R 0; _

O D power() __
  D x = factor();
  while( peek() == '^' ) { get(); x = pow( x, factor() ); }
  R x; _

O D term() __
  D x = power();
  while( memchr( "*/%|\\o&!", peek(), 8 ) ) __
    int c = get();
    D y = power();
    if(c == '*') x *= y;
    else if(c=='/') { if(y==0.0) err_flag=E_ZDIV; else x /= y; }
    else if(c=='%') { if(y==0.0) err_flag=E_ZMOD; else x -= y*floor(x/y); }
    else if(c=='|') { if(x+y==0.0) err_flag=E_ZDIV; else x = x*y/(x+y); }
    else if(c=='\\') { if(x==0.0) err_flag=E_ZDIV; else x = y/x; }
    else if(c=='&') { x = sqrt(x*x + y*y); }
    else if(c=='!') { x = perm(x,y); }
    else __ // 'o' -- circuit functions
      if( x==1 ) R sin(y);   if( x==10 ) R sin(y*M_PI/180.0);
      if( x==2 ) R cos(y);   if( x==20 ) R cos(y*M_PI/180.0);
      if( x==3 ) R tan(y);   if( x==30 ) R tan(y*M_PI/180.0);
      if( x==-1 ) R asin(y); if( x==-10 ) R asin(y)*180.0/M_PI;
      if( x==-2 ) R acos(y); if( x==-20 ) R acos(y)*180.0/M_PI;
      if( x==-3 ) R atan(y); if( x==-30 ) R atan(y)*180.0/M_PI;
      err_flag = E_CIRC; _ _
  R x; _

O D expression() __
  D result = term();
  while( peek() == '+' || peek() == '-' )
    if( get() == '+' ) result += term(); else result -= term();
  R result; _

O V trim( S s ) __ int i;
  for( i=strlen(s)-1; i>=0; --i ) if( strchr( BLANKS, s[i] )==NULL ) break;
  s[i+1] = '\0'; _

U calculate( S s, OUT D* r ) __
  trim(s); // trim trailing blanks although it's not so neccessary
  curr_pos = s  +strspn( s, BLANKS ); // skip leading blanks
  err_flag = E_OK;
  *r = expression();
  R err_flag; _
