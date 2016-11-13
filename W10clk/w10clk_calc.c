#include <string.h>

static int errorflag;
static char* expressionToParse;
#define BLANKS " \t\n\r\v\f"

static char peek()
{
  expressionToParse += strspn( expressionToParse, BLANKS );
  return *expressionToParse;
}

static char get()
{
  expressionToParse += strspn( expressionToParse, BLANKS );
  return *expressionToParse++;
}

static double expression();

static double number()
{
  double result = get() - '0';
  while('0'<=peek() && peek() <= '9') result = 10*result + get() - '0';
  if( peek()=='.' )
  {
    get();
    double div=1.0;
    while( '0'<=peek() && peek() <= '9' )
      result += (get() - '0') / (div *= 10.0);
  }
  return result;
}

static double factor()
{
  if( '0'<=peek() && peek() <= '9') return number();
  if(peek() == '(')
  {
    get(); // '('
    double result = expression();
    if(peek() != ')') { errorflag=1; return 0; }
    get(); // ')'
    return result;
  }
  if(peek() == '-') { get(); return -factor(); }
  if(peek() == '+') { get(); return factor(); }
  errorflag=2;
  return 0;
}

static double term()
{
  double result = factor();
  while(peek() == '*' || peek() == '/')
    if(get() == '*') result *= factor();
    else result /= factor();
  return result;
}

static double expression()
{
  double result = term();
  while( peek() == '+' || peek() == '-' )
    if( get() == '+' ) result += term();
    else result -= term();
  return result;
}

static void trim( char* s )
{
  int i;
  for( i=strlen(s)-1; i>=0; --i ) if( strchr( BLANKS, s[i] )==NULL ) break;
  s[i+1] = '\0';
}


int calculate( char* s, double* r )
{
  errorflag=0;
  trim(s);
  expressionToParse = s;
  *r = expression();
  return errorflag;
}

