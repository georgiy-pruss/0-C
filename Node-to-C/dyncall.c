// gcc -std=c99 dyncall.c -o dyncall.exe
// use it in cygwin-64 and with file in utf8
// output: W64  HMODULE:2  l:8  ll:8  p:8

// vcvarsall.bat amd64
// cl /nologo /Tpdyncall.cc "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\Lib\x64\user32.lib"
// output: W64  HMODULE:2  l:4  ll:8  p:8

// vcvarsall.bat x86
// cl /nologo /Tpdyncall.cc "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\Lib\user32.lib"
// output: W32  HMODULE:2  l:4  ll:8  p:4

// For VS2010/VS2013 save this file in utf-16le -- it's native unicode for VS
// It's actually C99 code, but VS doesn't support it, so VS must compile it as C++

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

/* types: c C w W -- char uchar wchar_t unsigned wchar_t
          b B h H i I l L -- int8 uint8 int16 uint16 int32 uint32 int64 uint64
          f d -- float double (yet to do...)
          v u -- void (for *v), utf8 (like char, but contents is unicode */
#define MAX_LIBS 100
#define MAX_PROC 1000
#define MAX_ARGS 8
typedef struct Proc { void* addr; int h;
  char _; char flags; char rettype; char nargs; char args[MAX_ARGS]; } Proc;
struct { int libs_offs; int proc_offs; int libs_cnt; int proc_cnt;
  HMODULE libs[MAX_LIBS]; Proc prc[MAX_PROC]; } z = { 184938517, 392857372, 0, 0, {0} };
#define FLAG_CDECL   1
#define FLAG_FPRESET 2
// types:
#define TYPE_OUT  0x40 // will be used for output
#define TYPE_PTR  0x20 // also TYPE_OUT|TYPE_PTR is for input/output
#define TYPE_UNS  0x10 // unsigned
#define TYPE_VOID 0x01 // 0
#define TYPE_CHAR 0x02 // 1
#define TYPE_BYTE 0x03 // 1
#define TYPE_UTF8 0x04 // 1 // external type, for converting, I guess
#define TYPE_WCHR 0x05 // 2
#define TYPE_HALF 0x06 // 2
#define TYPE_INT  0x07 // 4
#define TYPE_LONG 0x08 // 8
#define TYPE_FLT  0x09 // 4
#define TYPE_DBL  0x0A // 8
#define TYPE_MASK 0x0F // mask to check if type is defined

#ifdef __cplusplus
extern "C" { // VS2013 has to use C++ because it doesn't support C99. This is C99 code.
#endif

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
  int proc_cnt_was = z.proc_cnt;
  for( int proc=0; proc<MAX_PROC; ++proc )
    if( z.prc[proc].h == h ) { memset( &z.prc[proc], 0, sizeof(z.prc[0]) ); --z.proc_cnt; }
  return proc_cnt_was - z.proc_cnt;
}

int _parse_args( char* flags, char* rettype, char* args, const char* descr )
{
  *flags = 0;
  int iarg = 0;
  char* nxt = &args[iarg];
  int c; int t=0;
  for( int i=0; c=descr[i]; ++i )
  {
    if( iarg>MAX_ARGS ) return iarg;
    if( strchr("CWBHIL",c) ) { t |= TYPE_UNS; c=tolower(c); } // unsigned
    switch(c)
    {
      case '=': nxt = rettype; break;
      case '+': *flags |= FLAG_CDECL;   break; case '*': t = TYPE_PTR; break;
      case '%': *flags |= FLAG_FPRESET; break; case '>': t = TYPE_OUT; break;
      case 'v': t|=TYPE_VOID; break; case 'u': t|=TYPE_UTF8; break;
      case 'c': t|=TYPE_CHAR; break; case 'b': t|=TYPE_BYTE; break;
      case 'w': t|=TYPE_WCHR; break; case 'h': t|=TYPE_HALF; break;
      case 'i': t|=TYPE_INT;  break; case 'l': t|=TYPE_LONG; break;
      case 'f': t|=TYPE_FLT;  break; case 'd': t|=TYPE_DBL;  break;
    }
    if( t&TYPE_MASK ) { *nxt=(char)t; t=0; if( nxt!=rettype ) ++iarg; nxt=&args[iarg]; }
  }
  return iarg;
}
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
// args 5, 6, etc are on stack, so it's not 32, 64 etc functions, but not done yet...

typedef struct Arg { int t;
  union { char c; short h; int i; long long l; void* p; float f; double d; }; } Arg;
void call_proc( int proc, int nargs, const Arg args[] )
{
  void* addr = _proc_addr( proc );
  printf("call_proc %016x\n",addr);
  for( int i=0; i<nargs; ++i )
  {
    int t=args[i].t;
    switch( t&TYPE_MASK )
    {
      case TYPE_VOID: if(t&TYPE_PTR) printf("%...\n"); else printf("void\n"); break;
      case TYPE_UTF8: // let's pretend it's ok to print as char/str
      case TYPE_CHAR: if(t&TYPE_PTR) printf("%s\n",args[i].p); else printf("%c\n",args[i].c); break;
      case TYPE_WCHR: if(t&TYPE_PTR) printf("%s\n",args[i].p); else printf("%c\n",args[i].c); break;
      case TYPE_BYTE: if(t&TYPE_PTR) printf("%s\n",args[i].p); else
                        if(t&TYPE_UNS) printf("%u\n",(int)(unsigned char)args[i].c); else
                          printf("%d\n",args[i].c); break;
      case TYPE_HALF: if(t&TYPE_PTR) printf("..\n"); else
                        if(t&TYPE_UNS) printf("%u\n",(unsigned short)args[i].h); else
                          printf("%d\n",args[i].h); break;
      case TYPE_INT:  if(t&TYPE_PTR) printf("..\n"); else
                        if(t&TYPE_UNS) printf("%u\n",args[i].i); else
                          printf("%d\n",args[i].i); break;
      case TYPE_LONG: if(t&TYPE_PTR) printf("..\n"); else
                        if(t&TYPE_UNS) printf("%llu\n",args[i].l); else
                          printf("%lld\n",args[i].l); break;
      case TYPE_FLT:  if(t&TYPE_PTR) printf("..\n"); else printf("%g\n",(double)args[i].f); break;
      case TYPE_DBL:  if(t&TYPE_PTR) printf("..\n"); else printf("%g\n",args[i].d); break;
    }
  }
  // TODO :)
  (*(F4ippi)addr)( (int)args[0].h, args[1].p, args[2].p, args[3].i );
}

void use_call_proc( int p )
{
  int nargs = 4;
  Arg args[4];
  args[0].t = TYPE_UNS|TYPE_HALF; /* handle */   args[0].h = 0; /* no parent window */
  args[1].t = TYPE_PTR|TYPE_WCHR; /* wchar_t* */ args[1].p = (void*)L"hello привет";
  args[2].t = TYPE_PTR|TYPE_WCHR; /* wchar_t* */ args[2].p = (void*)L"title заголовок";
  args[3].t = TYPE_UNS|TYPE_INT;  /* uint */     args[3].i = 0; /* MB_OK */
  call_proc( p, nargs, args );
}


#ifdef _M_AMD64 // something else for Cygwin/Unix
#define W64
#endif

int main()
{
  #ifdef W64
  const char* ww = "W64";
  #else
  const char* ww = "W32";
  #endif
  printf("main %s  HMODULE:%d  ",ww,sizeof(L'x'));
  printf("l:%d  ll:%d  p:%d\n",sizeof(long),sizeof(long long),sizeof(void*));
  int h = load_lib("user32.dll");
  printf("main h:%d\n",h);
  int p = load_proc(h,"MessageBoxW","=i H *w *w I");
  printf("main p:%d\n",p);

  use_call_proc( p ); // <-----------------------

  int n = free_lib( h );
  printf("main n:%d\n",n);
  return 0;
}

#ifdef __cplusplus
};
#endif
