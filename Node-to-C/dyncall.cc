// vcvarsall.bat amd64
// cl /nologo /Tpdyncall.cc "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\Lib\x64\user32.lib"
// output: W64  HMODULE:2  l:4  ll:8  p:8  ... only first msgbox ok

// vcvarsall.bat x86
// cl /nologo /Tpdyncall.cc "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\Lib\user32.lib"
// output: W32  HMODULE:2  l:4  ll:8  p:4  ... all ok

// It's actually C99 code, but VS2013 doesn't support it, so VS2013 must compile it as C++

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#ifdef UNIX // not implemented yet...
  #include <dlfcn.h>
  typedef void *HMODULE;
  typedef char *LPSTR;
  typedef I (*FARPROC)();
  #define __stdcall
  #define _cdecl
#endif

// 'User32 MessageBoxW >i x * * x' cd 0;w;title;0  NB. cd is 15!:0

/*
HMODULE WINAPI LoadLibrary( _In_ LPCTSTR lpFileName );
BOOL WINAPI FreeLibrary( _In_ HMODULE hModule );

FARPROC WINAPI GetProcAddress( _In_ HMODULE hModule, _In_ LPCSTR  lpProcName );

int WINAPI MessageBox( _In_opt_ HWND hWnd, _In_opt_ LPCTSTR lpText, // __stdcall
  _In_opt_ LPCTSTR lpCaption, _In_ UINT uType );
// 0..4  MB_OK MB_OKCANCEL MB_ABORTRETRYIGNORE MB_YESNOCANCEL MB_YESNO
// 5..6 MB_RETRYCANCEL MB_CANCELTRYCONTINUE
// 16 MB_ICONERROR=MB_ICONSTOP=MB_ICONHAND  32 MB_ICONQUESTION
// 48 MB_ICONWARNING=MB_ICONEXCLAMATION  64 MB_ICONINFORMATION
*/

#define MAX_LIBS 100
#define MAX_PROC 1000
#define MAX_ARGS 8
typedef struct Proc { void* addr; int h;
  char _; char flags; char rettype; char nargs; char args[MAX_ARGS]; } Proc;
struct { int libs_offs; int proc_offs; int libs_cnt; int proc_cnt;
  HMODULE libs[MAX_LIBS]; Proc prc[MAX_PROC]; } z = { 184938517, 392857372, 0, 0, {0} };
#define FLAG_CDECL   1
#define FLAG_FPRESET 2

extern "C" { // VS2013 has to use C++ because it doesn't support C99. This is C99 code.

int load_lib( const char* libname )
{
  if( z.libs_cnt>=MAX_LIBS) return 0;
  HMODULE h = LoadLibrary( libname ); if(!h) return 0;
  printf("ll libhandle:%016x\n",(long long)h);
  for( int lib=0; lib<MAX_LIBS; ++lib )
    if( z.libs[lib]==0 ) { z.libs[lib]=h; ++z.libs_cnt; return lib+z.libs_offs; }
  return 0; // can't be here actually
}
int free_lib( int h )
{
  if(h<z.libs_offs||z.libs_offs+MAX_LIBS<=h) return 0;
  int lib = h-z.libs_offs;
  FreeLibrary(z.libs[lib]);
  z.libs[lib]=0;
  --z.libs_cnt;
  int pc = 0;
  for( int proc=0; proc<MAX_PROC; ++proc )
    if( z.prc[proc].h == h )
    {
      memset( &z.prc[proc], 0, sizeof(z.prc[0]) );
      --z.proc_cnt;
      ++pc;
    }
  return pc;
}
int _parse_args( char* flags, char* rettype, char* args,
  const char* descr )
{
  *flags = 0;
  int iarg = 0;
  int rettp = 0; // boolean
  char c; char t=0;
  for( int i=0; c=descr[i]; ++i )
  {
    if( iarg>MAX_ARGS ) return iarg;
    if( strchr("CWBHIL",c) ) { t = 0x80; c=tolower(c); }// unsigned
    switch(c)
    {
      case '+': *flags |= FLAG_CDECL; break;
      case '%': *flags |= FLAG_FPRESET; break;
      case '*': t = 0x20; break;
      case '>': t = 0x40; break;
      case '=': rettp = 1; break;
      case 'v': t|=0; if(rettp) *rettype=t; else args[iarg++]=t; t=0; break;
      case 'c': t|=1; if(rettp) *rettype=t; else args[iarg++]=t; t=0; break;
      case 'b': t|=1; if(rettp) *rettype=t; else args[iarg++]=t; t=0; break;
      case 'w': t|=2; if(rettp) *rettype=t; else args[iarg++]=t; t=0; break;
      case 'h': t|=2; if(rettp) *rettype=t; else args[iarg++]=t; t=0; break;
      case 'i': t|=4; if(rettp) *rettype=t; else args[iarg++]=t; t=0; break;
      case 'l': t|=8; if(rettp) *rettype=t; else args[iarg++]=t; t=0; break;
      case 'd': case 'f': /* not implemented yet */ break;
      case 's': /* == *c */
      case 't': /* == *w */
      case 'u': /* == *c */
      case 'p': /* == *v */ break;
    }
  }
  return iarg;
}
/* types: c C w W -- char uchar wchar_t unsigned wchar_t
          b B h H i I l L -- int8 uint8 int16 uint16 int32 uint32 int64 uint64
          f d -- float double (yet to do...)
          s t u p S? T? U? -- *char *wchar_t *char-as-utf8 *void   */
int load_proc( int h, const char* procname, const char* procdescr )
{
  if(z.proc_cnt>=MAX_PROC) return 0;
  if(h<z.libs_offs||z.libs_offs+MAX_LIBS<=h) return 0;
  int lib = h-z.libs_offs;
  printf("lp lib:%d\n",lib);
  printf("lp libhandle:%016x\n",(long long)z.libs[lib]);
  void* a = (void*)GetProcAddress( z.libs[lib], procname );
  if(!a) return 0;
  printf("lp a:%016x\n",(long long)a);
  for( int proc=0; proc<MAX_PROC; ++proc )
    if( z.prc[proc].addr == 0 )
    {
      z.prc[proc].addr = a;
      z.prc[proc].h = h;
      z.prc[proc].nargs = _parse_args( &z.prc[proc].flags, &z.prc[proc].rettype, z.prc[proc].args,
          procdescr );
      if( z.prc[proc].nargs > MAX_ARGS ) return 0;
      ++z.proc_cnt;
      return proc+z.proc_offs;
    }
  return 0; // can't be here actually
}
void* _proc_addr( int p )
{
  if(p<z.proc_offs||z.proc_offs+MAX_PROC<=p) return 0;
  int proc = p-z.proc_offs;
  return z.prc[proc].addr;
}


#define I int
#define P void* /* can be long long */
typedef I (__stdcall* F0)();
typedef I (__stdcall* F1i)(I);
typedef I (__stdcall* F1p)(P);
#define F2(n,a,b) typedef I (__stdcall* n)(a,b)
F2(F2ii,I,I); F2(F2ip,I,P); F2(F2pi,P,I); F2(F2pp,P,P);
#define F3(n,a,b,c) typedef I (__stdcall* n)(a,b,c)
F3(F3iii,I,I,I); F3(F3iip,I,I,P); F3(F3ipi,I,P,I); F3(F3ipp,I,P,P);
F3(F3pii,P,I,I); F3(F3pip,P,I,P); F3(F3ppi,P,P,I); F3(F3ppp,P,P,P);
#define F4(n,a,b,c,d) typedef I (__stdcall* n)(a,b,c,d)
F4(F4iiii,I,I,I,I); F4(F4iiip,I,I,I,P); F4(F4iipi,I,I,P,I); F4(F4iipp,I,I,P,P);
F4(F4ipii,I,P,I,I); F4(F4ipip,I,P,I,P); F4(F4ippi,I,P,P,I); F4(F4ippp,I,P,P,P);
F4(F4piii,P,I,I,I); F4(F4piip,P,I,I,P); F4(F4pipi,P,I,P,I); F4(F4pipp,P,I,P,P);
F4(F4ppii,P,P,I,I); F4(F4ppip,P,P,I,P); F4(F4pppi,P,P,P,I); F4(F4pppp,P,P,P,P);

typedef enum ArgType { VT,CT,UCT,WT,UWT,BT,UBT,HT,UHT,IT,UIT,LT,ULT,FT,DT,
                       VP,CP,UCP,WP,UWP,BP,UBP,HP,UHP,IP,UIP,LP,ULP,FP,DP } ArgType;
typedef struct Arg { ArgType t;
                     union { char c; short h; int i; long long l; void* p; }; } Arg;
void call_proc( int proc, int nargs, const Arg args[] )
{
  void* addr = _proc_addr( proc );
  printf("call_proc %016x\n",addr);
  printf("arg[0] %d\n",(int)args[0].h );
  printf("arg[1] %s\n",args[1].p );
  printf("arg[2] %s\n",args[2].p );
  printf("arg[3] %d\n",args[3].i );
  (*(F4ippi)addr)( (int)args[0].h, args[1].p, args[2].p, args[3].i );
}

void use_call_proc( int p )
{
  int nargs = 4;
  Arg args[4];
  args[0].t = UHT; // handle
  args[0].h = 0;
  args[1].t = WP; // wchar_t*
  args[1].p = (void*)L"hello привет";
  args[2].t = WP; // wchar_t*
  args[2].p = (void*)L"title заголовок";
  args[3].t = UIT; // unsigned
  args[3].i = 0; // MB_OK
  call_proc( p, nargs, args );
}


#ifdef _M_AMD64
#define W64
#else
#endif

int main()
{
  const char* ww =
  #ifdef W64
    "W64";
  #else
    "W32";
  #endif
  printf("main %s  HMODULE:%d  ",ww,sizeof(L'x'));
  printf("l:%d  ll:%d  p:%d\n",sizeof(long),sizeof(long long),sizeof(void*));
  int h = load_lib("user32.dll");
  printf("main h:%d\n",h);
  int p = load_proc(h,"MessageBoxW","=i h *w *w I");
  printf("main p:%d\n",p);

  use_call_proc( p );

  int n = free_lib( h );
  printf("main n:%d\n",n);
  return 0;
}

};
