// Windows 10 Clock - Copyright (C) Georgy Pruss 2016
// compile with cygwin64 (-std=c11 is default) (-O gains 1.5kB over -O2):
// gcc -O w10clk*.c w10clk_res.o -lgdi32 -lcomdlg32 -lwinmm -Wl,--subsystem,windows -o w10clk.exe
// with resource .o made from .rc (and .ico): windres w10clk_res.rc w10clk_res.o
// https://www.cygwin.com/faq.html#faq.programming.win32-no-cygwin
// http://parallel.vub.ac.be/education/modula2/technology/Win32_tutorial/index.html

// TODO switch two bg colors; alarms; reminders; birthdays; time in expressions
// TODO round window; double-buffering; m - moon phase
// http://stackoverflow.com/questions/3970066/creating-a-transparent-window-in-c-win32
// https://www.codeproject.com/kb/dialog/semitrandlgwithctrls.aspx
// To create a non-rectangular window, have a look at the SetWindowRgn function:
// https://msdn.microsoft.com/en-gb/library/windows/desktop/dd145102(v=vs.85).aspx

#define PROGRAM_NAME "Windows 10 Clock"
#define PGM_NAME "w10clk"
#define HELP_MSG "Unrecognized key. Press F1 or ? for help.\n\n" \
"Configure the clock appearance in file " PGM_NAME ".ini\n\n" \
"Version 1.30 * Copyright (C) Georgiy Pruss 2016\n\n" \
"[Press Cancel to not receive this message again]"

#define WIN32_LEAN_AND_MEAN // Trim fat from windows
#include <stdio.h>
#include <time.h>
#include <math.h> // M_PI sin cos
#include <unistd.h> // access
#include <windows.h>
#include <commdlg.h> // ChooseColor CHOOSECOLOR
#include <shellapi.h> // ShellExecute
#include <Mmsystem.h> // PlaySound
#include "../_.h"
typedef unsigned int uint;
typedef int bool;
#define false 0
#define true 1
#define STRIEQ(s,t) (strcasecmp(s,t)==0) // may need to change for MSVC etc
void strcpyupr( char* d, const char* s ) { if( s && d ) while( *s ) *d++ = toupper( *s++ ); }
extern uint process_char( uint c, char* s, uint max_len, uint n ); // w10clk_procchar.c

// Parameters; all defaults are in read_int()
COLORREF g_bgcolor;     // bg color
int g_init_x, g_init_y; // initial position
int g_width,  g_height; // initial size
int g_tick_w;  COLORREF g_tick_rgb;  // width and color of ticks (0 - no ticks)
int g_shand_w; COLORREF g_shand_rgb; // s,m,h hands
int g_mhand_w; COLORREF g_mhand_rgb;
int g_hhand_w; COLORREF g_hhand_rgb;
int g_t_len, g_sh_len, g_mh_len, g_hh_len;
int g_disp_x, g_disp_y; // display position, percents
int g_corr_x, g_corr_y, g_corrnr_x, g_corrnr_y; // correction, also for non-resizable
bool g_seconds,g_upd_title, g_resizable, g_ontop, g_circle; // flags
char* g_timefmt = NULL; // strftime format of time in title
char* g_tzlist = NULL;  // list of timezones, all in one string
char* g_bell = NULL; char* g_hbell = NULL; // bell and halfbell sounds
char* g_voice_dir = NULL;

enum ProgramKind { K_ERR, K_EXE, K_SCR } pgmKind = K_ERR;
char* pgmName = NULL; // short name of executable w/o extension
char* pgmPath = NULL; // full path to the program, with ending backslash
bool timeAnnounced = false;  // bell hit
#define VOICE_FILE "voice.txt"
#define VOICE_LINES 48 // 24 lines for exact hours, then 24 lines for halves
char** voiceLines = NULL; // from VOICE_FILE: { "xx.wav xy.wav", ..., "y.wav z.wav", NULL }

void
cleanup() __
  #define FREE(x) if(x) { free(x); x=NULL; }
  FREE( g_timefmt ); FREE( g_tzlist ); FREE( pgmName ); FREE( pgmPath );
  FREE( g_bell ); FREE( g_hbell ); FREE( g_voice_dir ); _

bool
get_pgm_name() __ // set pgmKind pgmName pgmPath
  char pgm[MAX_PATH];
  int n = GetModuleFileName( NULL, pgm, sizeof(pgm) );
  if( n <= 0 ) return false;
  if( STRIEQ(pgm+n-4,".exe") ) pgmKind=K_EXE; // normal executable
  else if( STRIEQ(pgm+n-4,".scr") ) pgmKind=K_SCR; // screensaver
  else return false; // could not tell, pgmKind==K_ERR
  char* bkslash = strrchr( pgm, '\\' );
  if( bkslash==NULL ) return false; // strange case - no path, let's give up
  bkslash[ strlen(bkslash) - 4 ] = '\0'; // only name w/o extension
  pgmName = strdup( bkslash+1 );
  bkslash[ 1 ] = '\0'; // only path and backslash
  pgmPath = strdup( pgm );
  return true; _

COLORREF
mix_colors( COLORREF c1, COLORREF c2 ) __
  uint r = ((c1&0xFF)+(c2&0xFF))/2;
  uint g = (((c1&0xFF00)+(c2&0xFF00))/2) & 0xFF00;
  uint b = (((c1&0xFF0000)+(c2&0xFF0000))/2) & 0xFF0000;
  return b|g|r; _

COLORREF
swap_colors( COLORREF c ) __
  return ((c&0xFF)<<16) | (c&0xFF00) | ((c&0xFF0000)>>16); _

void
split_colors( COLORREF c, /* OUT */ uint* r, uint* g, uint* b ) __
  *r = c&0xFF; *g = (c&0xFF00)>>8; *b = (c&0xFF0000)>>16; _

char* prepare_sound( const char* s );
char** read_lines( const char* dir, const char* filename );

bool
read_ini_file( const char* fname, const char* divname ) __
  char p_n_e[MAX_PATH]; strcpy(p_n_e,pgmPath); strcat(p_n_e,fname); strcat(p_n_e,".ini");
  char s[500] = ""; DWORD rc; int x,y; uint w,h,r,g,b,c;
  #define READINI(nm,def) rc = GetPrivateProfileString( divname, nm, def, s, sizeof(s), p_n_e)
  #define BND(x,f,t) if(x<f)x=f;if(x>t)x=t
  #define BNDC(x) if(x>255)x=255
  READINI("xywh","5,0,200,200");
  if( rc>=7 && sscanf( s,"%d,%d,%u,%u", &x,&y,&w,&h )==4 ) { g_init_x = x; g_init_y = y;
      g_width = w; g_height = h; BND( g_width, 8, 9999 ); BND( g_height, 8, 9999 ); }
  READINI("bg","60,0,120");
  if( rc>=5 )
    if( sscanf( s,"%u,%u,%u", &r,&g,&b )==3 ) { BNDC(r);BNDC(g);BNDC(b); g_bgcolor = RGB(r,g,b); }
    else if( sscanf( s,"#%X", &c )==1 ) { BND(c,0,0xFFFFFF); g_bgcolor = swap_colors(c); }
  READINI("ticks","3,210,210,210");
  if( rc>=7 )
    if( sscanf( s,"%u,%u,%u,%u", &w,&r,&g,&b)==4 ) { BND( w, 0,90 ); g_tick_w = w;
      BNDC(r);BNDC(g);BNDC(b); g_tick_rgb = RGB(r,g,b); }
    else if( sscanf( s,"%u,#%X", &w,&c )==2 ) { BND( w, 0,90 ); g_tick_w = w;
      BND(c,0,0xFFFFFF); g_tick_rgb = swap_colors(c); }
  READINI("shand","2,255,255,255");
  if( rc>=7 )
    if( sscanf( s,"%d,%d,%d,%d", &w,&r,&g,&b)==4 ) {  BND( w, 1,90 ); g_shand_w = w;
      BNDC(r);BNDC(g);BNDC(b); g_shand_rgb = RGB(r,g,b); }
    else if( sscanf( s,"%u,#%X", &w,&c )==2 ) { BND( w, 0,90 ); g_shand_w = w;
      BND(c,0,0xFFFFFF); g_shand_rgb = swap_colors(c); }
  READINI("mhand","3,255,255,255");
  if( rc>=7 )
    if( sscanf( s,"%d,%d,%d,%d", &w,&r,&g,&b)==4 ) { BND( w, 1,90 ); g_mhand_w = w;
      BNDC(r);BNDC(g);BNDC(b); g_mhand_rgb = RGB(r,g,b); }
    else if( sscanf( s,"%u,#%X", &w,&c )==2 ) { BND( w, 0,90 ); g_mhand_w = w;
      BND(c,0,0xFFFFFF); g_mhand_rgb = swap_colors(c); }
  READINI("hhand","5,255,255,255");
  if( rc>=7 )
    if( sscanf( s,"%d,%d,%d,%d", &w,&r,&g,&b)==4 ) { BND( w, 1,90 ); g_hhand_w = w;
      BNDC(r);BNDC(g);BNDC(b); g_hhand_rgb = RGB(r,g,b); }
    else if( sscanf( s,"%u,#%X", &w,&c )==2 ) { BND( w, 0,90 ); g_hhand_w = w;
      BND(c,0,0xFFFFFF); g_hhand_rgb = swap_colors(c); }
  READINI("lengths","8,84,84,62");
  if( rc>=10 && sscanf( s,"%d,%d,%d,%d", &x,&y,&w,&h)==4 ) { g_t_len = x; BND( g_t_len, 1,96 );
    g_sh_len = y; g_mh_len = w; g_hh_len = h;
    BND( g_sh_len, 1,99 ); BND( g_mh_len, 1,99 ); BND( g_hh_len, 1,99 ); }
  READINI("disp","20,40");
  if( rc>=3 && sscanf( s,"%d,%d", &x,&y )==2 ) { BND(x,0,99); BND(y,0,99);
    g_disp_x = x; g_disp_y = y; }
  READINI("corr","16,8,6,3");
  if( rc>=7 && sscanf( s,"%d,%d,%d,%d", &x,&y,&w,&h )==4 )
    { g_corr_x = x; g_corr_y = y; g_corrnr_x = w; g_corrnr_y = h; }
  READINI("seconds","no");          if( rc>1 ) g_seconds = STRIEQ(s,"yes");
  READINI("intitle","no");          if( rc>1 ) g_upd_title = STRIEQ(s,"yes");
  READINI("resizable","yes");       if( rc>1 ) g_resizable = STRIEQ(s,"yes");
  READINI("ontop","no");            if( rc>1 ) g_ontop = STRIEQ(s,"yes");
  READINI("circle","no");           if( rc>1 ) g_circle = STRIEQ(s,"yes");
  READINI("timefmt","%T %a %m/%d"); if( rc>1 ) g_timefmt = strdup( s );
  READINI("tz","GMT +0 CET +1 EET +2"); // it's easier to always have something there
  if( rc>=3 ) __
    g_tzlist = (char*)malloc( rc+2 ); g_tzlist[0]=' '; strcpyupr( g_tzlist+1, s ); _
  READINI("bell","");               if( rc>1 ) g_bell = prepare_sound( s );
  READINI("halfbell","");           if( rc>1 ) g_hbell = prepare_sound( s );
  READINI("voice","");
  if( rc>1 ) __ // set g_voice_dir if file VOICE_FILE is readable there (and read it)
    char* v = (char*)malloc( strlen(pgmPath)+rc+1+1 ); // "...path/voice/"
    strcpy( v, pgmPath ); strcat( v, s ); strcat( v, "\\" );
    voiceLines = read_lines( v, VOICE_FILE );
    if( voiceLines==NULL ) free( v ); else g_voice_dir = v; _
  return access(p_n_e, F_OK)==0; _

bool
read_ini() __ // all parameter names and default values are here!
  if( ! get_pgm_name() ) return false; // sets pgmName pgmPath pgmKind
  if( read_ini_file( pgmName, pgmName ) ) return true;
  read_ini_file( PGM_NAME, pgmName ); // if not exist, set default values
  return true; _

// Const (all upper-case) and global state vars (camel-style names)
const int ID_TIMER = 1;
RECT rcClient; // clock client area
HBRUSH hbrBG;    // clock background
HPEN hsPen, hmPen, hhPen, htPen;     // clock hands (1 pixel narrower than required)
HPEN hsPen2, hmPen2, hhPen2, htPen2; // half-tone borders of clock hands
HPEN hbgPen, hbgPen2;
int clrIncr = 1;

void
init_tools(bool for_all) __
  hbrBG = CreateSolidBrush(g_bgcolor);
  hbgPen = CreatePen(PS_SOLID,1,g_bgcolor);
  hbgPen2 = CreatePen(PS_SOLID,1,mix_colors(g_bgcolor,0));
  if( for_all ) __
    hsPen = CreatePen(PS_SOLID,max(g_shand_w-1,1),g_shand_rgb);
    hmPen = CreatePen(PS_SOLID,max(g_mhand_w-1,1),g_mhand_rgb);
    hhPen = CreatePen(PS_SOLID,max(g_hhand_w-1,1),g_hhand_rgb);
    htPen = CreatePen(PS_SOLID,max(g_tick_w-1,1),g_tick_rgb); _
  hsPen2 = CreatePen(PS_SOLID,g_shand_w,mix_colors(g_bgcolor,g_shand_rgb));
  hmPen2 = CreatePen(PS_SOLID,g_mhand_w,mix_colors(g_bgcolor,g_mhand_rgb));
  hhPen2 = CreatePen(PS_SOLID,g_hhand_w,mix_colors(g_bgcolor,g_hhand_rgb));
  htPen2 = CreatePen(PS_SOLID,g_tick_w,mix_colors(g_bgcolor,g_tick_rgb)); _

void
kill_tools(bool for_all) __
  DeleteObject(hbrBG);
  if( for_all ) __
    DeleteObject(hsPen); DeleteObject(hmPen), DeleteObject(hhPen), DeleteObject(htPen); _
  DeleteObject(hsPen2); DeleteObject(hmPen2), DeleteObject(hhPen2), DeleteObject(htPen2); _

bool
help_on_error_input() { return MessageBox(NULL, HELP_MSG, PROGRAM_NAME, MB_OKCANCEL )==IDOK; }

static void
show_help_file() __
  char hfn[MAX_PATH+16]; strcpy( hfn, "file:///" );
  strcat( hfn, pgmPath ); strcat( hfn, PGM_NAME ); strcat( hfn, ".htm" );
  for( char* p=hfn; *p; ++p ) if(*p=='\\') *p='/'; // not needed actually, but let it be
  ShellExecute(NULL, "open", hfn, NULL, NULL, SW_SHOWNORMAL); _

void
mb( const char* txt, const char* cap ) { MessageBox(NULL, txt, cap, MB_OK); }

double sind(double x) { return sin(x*M_PI/180); }
double cosd(double x) { return cos(x*M_PI/180); }

void
draw_clock(HDC hdc,int halfw, int halfh) __
  // draw clock face and ticks
  if( g_tick_w == 0 ) return;
  if( g_circle ) __
    int m = min(rcClient.right, rcClient.bottom); // min size
    int dx = (rcClient.right - m) / 2, dy = (rcClient.bottom - m) / 2;
    SelectObject(hdc, hbrBG);
    SelectObject(hdc, hbgPen2); Ellipse(hdc, dx,dy, m+dx,m+dy);
    SelectObject(hdc, hbgPen);  Ellipse(hdc, dx+1,dy+1, m+dx-1,m+dy-1); _
  else __ // rectangular
    SetBkMode(hdc, OPAQUE);
    FillRect(hdc, &rcClient, hbrBG); _ // need to clean old hands, sorry
  int r = min(halfw,halfh); // radius
  for( int i=0; i<12; ++i ) __
    int start_x = halfw + (int)( r * (96-g_t_len) / 100.0 * cosd(30*i) + 0.5 );
    int start_y = halfh + (int)( r * (96-g_t_len) / 100.0 * sind(30*i) + 0.5 );
    int end_x   = halfw + (int)( r * 0.96                 * cosd(30*i) + 0.5 );
    int end_y   = halfh + (int)( r * 0.96                 * sind(30*i) + 0.5 );
    SelectObject(hdc, htPen2);
    MoveToEx(hdc,start_x,start_y,NULL);
    LineTo(hdc,end_x,end_y);
    SelectObject(hdc, htPen);
    MoveToEx(hdc,start_x,start_y,NULL);
    LineTo(hdc,end_x,end_y); _ _

void
update_clock(HDC hdc,int halfw, int halfh, struct tm* tmptr) __
  // draw arrows
  int s = tmptr->tm_sec;
  int m = tmptr->tm_min;
  int h = tmptr->tm_hour % 12;

  double angle_sec = 270.0 + (s*6.0);
  double angle_min = 270.0 + (m*6.0 + s*0.1);
  double angle_hour = 270.0 + (h*30 + m*0.5);

  int r = min(halfw,halfh); // radius
  int secx  = (int)( r * g_sh_len / 100.0 * cosd(angle_sec) + 0.5 );
  int secy  = (int)( r * g_sh_len / 100.0 * sind(angle_sec) + 0.5 );
  int minx  = (int)( r * g_mh_len / 100.0 * cosd(angle_min) + 0.5 );
  int miny  = (int)( r * g_mh_len / 100.0 * sind(angle_min) + 0.5 );
  int hourx = (int)( r * g_hh_len / 100.0 * cosd(angle_hour)+ 0.5 );
  int houry = (int)( r * g_hh_len / 100.0 * sind(angle_hour)+ 0.5 );

  // right order - hour, minute, second hand on top
  SelectObject(hdc, hhPen2);
  MoveToEx(hdc, halfw, halfh, NULL); LineTo(hdc, halfw + hourx, halfh + houry);
  SelectObject(hdc, hhPen);
  MoveToEx(hdc, halfw, halfh, NULL); LineTo(hdc, halfw + hourx, halfh + houry);
  SelectObject(hdc, hmPen2);
  MoveToEx(hdc, halfw, halfh, NULL); LineTo(hdc, halfw + minx, halfh + miny);
  SelectObject(hdc, hmPen);
  MoveToEx(hdc, halfw, halfh, NULL); LineTo(hdc, halfw + minx, halfh + miny);
  if( g_seconds ) __
    SelectObject(hdc, hsPen2);
    MoveToEx(hdc, halfw, halfh, NULL); LineTo(hdc, halfw + secx, halfh + secy);
    SelectObject(hdc, hsPen);
    MoveToEx(hdc, halfw, halfh, NULL); LineTo(hdc, halfw + secx, halfh + secy); _ _

void
update_title(HWND hwnd, const char* title, time_t t ) __
  if( title ) { SetWindowText(hwnd,title); return; }
  struct tm* tmptr = localtime(&t); if( !tmptr ) return;
  char text[100];
  strftime( text, sizeof(text), g_timefmt, tmptr );
  SetWindowText(hwnd,text); _

char disp[400] = ""; // input field and display
uint ndisp = 0;      // current length of text in disp

void
redraw_window(HWND hwnd) __
  time_t t = time(NULL);
  struct tm* tmptr = localtime(&t); if( !tmptr ) return;
  PAINTSTRUCT paintStruct;
  HDC hDC = BeginPaint(hwnd,&paintStruct);
    draw_clock( hDC, rcClient.right/2, rcClient.bottom/2 );
    update_clock( hDC, rcClient.right/2, rcClient.bottom/2, tmptr );
    if( ndisp!=0 ) TextOut(hDC,rcClient.right*g_disp_x/100,rcClient.bottom*g_disp_y/100,disp,ndisp);
  EndPaint(hwnd, &paintStruct); _

bool
paste_from_clipboard(HWND hwnd, bool append) __
  if( !OpenClipboard(hwnd) ) return false;
  HGLOBAL hglb = GetClipboardData(CF_TEXT);
  if( !hglb ) return false;
  LPSTR lpstr = GlobalLock(hglb);
  GlobalUnlock(hglb);
  CloseClipboard();
  char* s; char* d=disp; uint cnt=0; // one blank for many chars<=32
  if( ! append ) ndisp=0; else d += ndisp;
  for( s=lpstr; *s && ndisp<sizeof(disp)-1; ++s )
    if( *s>32 ) { *d=*s; ++d; ++ndisp; cnt=0; } // just copy
    else if( cnt==0 ) { *d=' '; ++d; ++ndisp; cnt=1; } // set blank for first
  *d = '\0';
  return true; _

void
copy_to_clipboard(HWND hwnd) __
  if( !OpenClipboard(hwnd) ) return;
  if( !EmptyClipboard() ) return;
  HGLOBAL hGlob = GlobalAlloc(GMEM_FIXED, sizeof(disp));
  strcpy( (char*)hGlob, disp );
  if( !SetClipboardData( CF_TEXT, hGlob ) )
    CloseClipboard(), GlobalFree(hGlob); // free allocated string on error
  else
    CloseClipboard(); _

char**
read_lines( const char* path, const char* nm ) __
  if( path==NULL || strlen(path)+strlen(nm) > MAX_PATH-1 ) return NULL;
  char fnm[MAX_PATH]; strcpy(fnm, path), strcat( fnm, nm );
  FILE* f = fopen( fnm, "rt" );
  if( !f ) return NULL;
  char** a = (char**)malloc( (VOICE_LINES+1)*sizeof(char*) );
  for( int i=0; i<VOICE_LINES+1; ++i )
    a[i] = NULL;
  char line[200];
  for( int i=0; i<VOICE_LINES && fgets(line, sizeof(line), f) != NULL; ++i ) __
    int n = strlen(line);
    if( line[n-1] == '\n' )
      line[n-1] = 0;
    char* s = strdup( line );
    a[i] = s; _
  fclose( f );
  return a; _

void
free_lines( char** a ) { if( a ) { for( int i=0; a[i]; ++i ) free( a[i] ); free( a ); } }

void
play_voice_file( const char* w, int l ) __ // file from g_voice_dir directory
  char nm[MAX_PATH]; int gv_len = strlen(g_voice_dir);
  memcpy( nm, g_voice_dir, gv_len ); memcpy( nm+gv_len, w, l ); memcpy( nm+gv_len+l, ".wav", 5 );
  PlaySound( nm, NULL, SND_FILENAME ); _

void
play_voice( time_t t ) __ // play all "words" from corresponding line
  if( !voiceLines ) return;
  struct tm* tmptr = localtime(&t);
  int h = tmptr->tm_hour; // 0..23
  if( tmptr->tm_min >= 30 ) h+=24; // 0..23, 24..47
  char* w = voiceLines[h];
  for( int n = strspn( w, " " );; ) __ // first skip leading blanks if any
    w += n; n = strcspn( w, " " ); play_voice_file( w, n ); // in dir g_voice_dir
    w += n; n = strspn( w, " " ); if( n==0 ) break; _ _

const char* sK[] = {"","Default","Asterisk","Welcome","Start","Exit","Question","Exclamation","Hand"};
LPCTSTR sV[] = { (LPCTSTR)"", (LPCTSTR)SND_ALIAS_SYSTEMDEFAULT, (LPCTSTR)SND_ALIAS_SYSTEMASTERISK,
  (LPCTSTR)SND_ALIAS_SYSTEMWELCOME, (LPCTSTR)SND_ALIAS_SYSTEMSTART, (LPCTSTR)SND_ALIAS_SYSTEMEXIT,
  (LPCTSTR)SND_ALIAS_SYSTEMQUESTION, (LPCTSTR)SND_ALIAS_SYSTEMEXCLAMATION,
  (LPCTSTR)SND_ALIAS_SYSTEMHAND };

char*
prepare_sound( const char* s ) __
  if( s==NULL || strlen(s)<4 ) return strdup( "" );
  for( uint i=1; i < sizeof(sK)/sizeof(sK[0]); ++i )
    if( STRIEQ( sK[i], s ) ) { char sc[2] = "."; sc[0] = (char)i; return strdup( sc ); }
  if( s[1]==':' && s[2]=='\\' ) return strdup( s ); // full path - no check
  char nm[MAX_PATH+20];
  strcpy( nm, pgmPath ); strcat( nm, s );
  if( access( nm, F_OK )==0 ) return strdup( nm );
  int rc = GetEnvironmentVariable("SystemRoot",nm,sizeof(nm));
  if( 3<rc && rc<sizeof(nm) ) __
    strcat( nm, "\\Media\\" ); strcat( nm, s );
    if( access( nm, F_OK )==0 ) return strdup( nm ); _
  return strdup( "" ); _

void
play_sound( const char* s, time_t t ) __ // if t!=0, say it
  if( s==NULL || s[0]==0 ) return;
  uint sync = t ? 0 : SND_ASYNC;
  if( s[0] < sizeof(sV)/sizeof(sV[0]) )
    // see also: define, load, play resource
    // https://msdn.microsoft.com/en-us/library/windows/desktop/dd743679(v=vs.85).aspx
    PlaySound( sV[s[0]], NULL, sync|SND_ALIAS_ID );
  else
    PlaySound( s, NULL, sync|SND_FILENAME );
  if( t )
    play_voice( t ); _

// Main Window Event Handler
LRESULT CALLBACK
WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) __
  switch( message ) __
  case WM_CREATE: // Window is being created, good time for init tasks
    if( SetTimer(hwnd, ID_TIMER, 1000, NULL) == 0 ) // tick every second
      MessageBox(hwnd, "Could not set timer!", "Error", MB_OK | MB_ICONEXCLAMATION);
    init_tools(true);
    break;
  case WM_SIZE: // WM_PAINT will be sent as well
    GetClientRect(hwnd, &rcClient); // left = top = 0
    break;
  case WM_TIMER: __ time_t t = time(NULL);
    if( g_upd_title ) update_title(hwnd,NULL,t); // update caption every tick
    if( g_seconds || t%10==0 )   // but window - every 10 s if w/o sec.hand
      InvalidateRect(hwnd, NULL, FALSE), UpdateWindow(hwnd);
    if( g_bell || g_hbell ) __
      if( t%1800<25 ) __ // time to chime?
        if( !timeAnnounced ) __
          play_sound( t%3600<25 ? g_bell : g_hbell, g_voice_dir ? t : 0 );
          timeAnnounced = true; _ _
      else timeAnnounced = false; _
    break; _
  case WM_PAINT: // Window needs update for whatever reason
    redraw_window(hwnd);
    break;
  case WM_RBUTTONDOWN: // paste
    if( paste_from_clipboard(hwnd,true) )
      InvalidateRect(hwnd, NULL, FALSE), UpdateWindow(hwnd);
    break;
  case WM_KEYDOWN:
    if( wParam==VK_F1 ) show_help_file();
    else if( wParam==VK_DELETE ) __ // del -- delete char at the beginning
      if( ndisp!=0 ) __
        memmove( disp, disp+1, ndisp-- ); // including '\0'
        InvalidateRect(hwnd, NULL, FALSE), UpdateWindow(hwnd); _ _
    else if( pgmKind==K_EXE ) __ // change size/pos with arrows keys and Ctrl and Shift
      RECT r; GetWindowRect( hwnd, &r );
      #define SWP(x,y,cx,cy,f) SetWindowPos( hwnd, HWND_NOTOPMOST, x,y, cx,cy, f )
      int a = GetAsyncKeyState( VK_SHIFT )==0 ? 1 : 10; // do more when Shift pressed
      if( GetAsyncKeyState( VK_CONTROL )==0 ) __ // arrows = move
        if( wParam==VK_RIGHT ) SWP( r.left+a, r.top,   0, 0, SWP_NOSIZE );
        if( wParam==VK_DOWN )  SWP( r.left,   r.top+a, 0, 0, SWP_NOSIZE );
        if( wParam==VK_LEFT )  SWP( r.left-a, r.top,   0, 0, SWP_NOSIZE );
        if( wParam==VK_UP )    SWP( r.left,   r.top-a, 0, 0, SWP_NOSIZE ); _
      else __ // with Ctrl+arrows = resize
        if( wParam==VK_RIGHT ) SWP( 0, 0, r.right-r.left+a, r.bottom-r.top,   SWP_NOMOVE );
        if( wParam==VK_DOWN )  SWP( 0, 0, r.right-r.left,   r.bottom-r.top+a, SWP_NOMOVE );
        if( wParam==VK_LEFT )  SWP( 0, 0, r.right-r.left-a, r.bottom-r.top,   SWP_NOMOVE );
        if( wParam==VK_UP )    SWP( 0, 0, r.right-r.left,   r.bottom-r.top-a, SWP_NOMOVE ); _ _
    break;
  case WM_CHAR:
    if( wParam==22 ) __ // ctrl+v
      if( paste_from_clipboard(hwnd,true) )
        InvalidateRect(hwnd, NULL, FALSE), UpdateWindow(hwnd); _
    else if( wParam==3 && ndisp!=0 ) // ctrl+c
      copy_to_clipboard(hwnd);
    else if( wParam==20 ) __ // ctrl+t
      g_upd_title = ! g_upd_title;
      if( ! g_upd_title ) update_title(hwnd,PROGRAM_NAME,time(NULL)); _
    else if( wParam==19 ) __ // ctrl+s
      g_seconds = ! g_seconds;
      InvalidateRect(hwnd, NULL, FALSE), UpdateWindow(hwnd); _
    else if( wParam==18 || wParam==7 || wParam==2 ) __ // ctrl+r ctrl+g ctrl+b
      uint r = g_bgcolor & 0xFF, g = (g_bgcolor & 0xFF00) >> 8, b = (g_bgcolor & 0xFF0000) >> 16;
      if( wParam==18 && ((clrIncr>0 && r<255) || (clrIncr<0 && r>0)) ) r += clrIncr;
      else if( wParam==7 && ((clrIncr>0 && g<255) || (clrIncr<0 && g>0)) ) g += clrIncr;
      else if( wParam==2 && ((clrIncr>0 && b<255) || (clrIncr<0 && b>0)) ) b += clrIncr;
      g_bgcolor = r|(g<<8)|(b<<16);
      kill_tools(false);
      init_tools(false);
      ndisp = sprintf( disp, "#%06X %s", (uint)swap_colors(g_bgcolor), clrIncr>0 ? "^" : "v" ); _
    else if( wParam==9 ) __ // ctrl+i invert color changing direction
      clrIncr = -clrIncr;
      ndisp = sprintf( disp, "#%06X %s", (uint)swap_colors(g_bgcolor), clrIncr>0 ? "^" : "v" ); _
    else if( wParam=='x' && pgmKind==K_EXE ) __ // maximize
      static bool maximized = false; // maybe some other vars should be like this too
      maximized = ! maximized;
      static RECT r; static LONG style;
      if( maximized ) __
         GetWindowRect( hwnd, &r );
         style = SetWindowLong(hwnd, GWL_STYLE, 0); // returns 0x14CC0000 -- visible, caption
         ShowWindow(hwnd, SW_SHOWMAXIMIZED); _      //         clipsiblings, sizebox, sysmenu
      else __ // restored to original size
         ShowWindow(hwnd, SW_RESTORE);
         SetWindowLong(hwnd, GWL_STYLE, style);
         SetWindowPos( hwnd, g_ontop ? HWND_TOPMOST : HWND_NOTOPMOST,
           r.left, r.top, r.right-r.left, r.bottom-r.top, SWP_FRAMECHANGED ); _ _
    else if( wParam=='b' ) __ // background
      if( ndisp!=0 && disp[0]=='#' ) __
        uint rgb; int k = sscanf( disp+1, "%X", &rgb );
        if( k==1 ) __
          g_bgcolor = swap_colors(rgb);
          kill_tools(false);
          init_tools(false); _ _
      ndisp = sprintf( disp, "#%06X %s", (uint)swap_colors(g_bgcolor), clrIncr>0 ? "^" : "v" ); _
    else if( wParam=='c' && (ndisp==0 || ndisp!=0 && disp[0]=='#') ) __
      CHOOSECOLOR cc;
      static COLORREF acrCustClr[16]; // array of custom colors
      ZeroMemory(&cc, sizeof(cc)); cc.lStructSize = sizeof(cc);
      cc.hwndOwner = hwnd; cc.lpCustColors = (LPDWORD)acrCustClr;
      cc.Flags = CC_FULLOPEN | CC_RGBINIT; cc.rgbResult = (COLORREF)0;
      if( ndisp!=0 && disp[0]=='#' ) __
        uint rgb; int k = sscanf( disp+1, "%X", &rgb );
        if( k==1 ) cc.rgbResult = swap_colors(rgb); _
      if( ChooseColor(&cc)==TRUE )
        ndisp = sprintf( disp, "#%06X", (uint)swap_colors(cc.rgbResult) ); _
    else if( wParam=='?' || wParam=='h' && ndisp==0 )
      show_help_file();
    else if( wParam=='i' ) __ // info on coords
      char msg[200];
      if( pgmKind==K_SCR )
        sprintf( msg, "Screensaver size: %d,%d", (int)rcClient.right, (int)rcClient.bottom );
      else __
        RECT r; GetWindowRect( hwnd, &r );
        int w = (int)r.right-(int)r.left;
        int h = (int)r.bottom-(int)r.top;
        int w2 = w - (g_resizable ? g_corr_x : g_corrnr_x);
        int h2 = h - (g_resizable ? g_corr_y : g_corrnr_y);
        sprintf( msg, "Position: %d,%d\nRaw size: %d,%d\nCorrected size: %d,%d\n"
          "Clockface size: %d,%d", (int)r.left, (int)r.top, w,h, w2,h2,
          (int)rcClient.right, (int)rcClient.bottom ); _
      MessageBox( hwnd, msg, "Window 10 clock -- dimensions", MB_OK ); _
    else if( wParam=='q' ) // quit
      PostMessage(hwnd,WM_CLOSE,0,0);
    else if( wParam==5 )  play_sound( g_bell, 0 );  // test sound ^E
    else if( wParam==21 ) play_sound( g_hbell, 0 ); // test sound ^U
    else if( wParam==6 )  play_voice( time(NULL) ); // test voice ^F
    else
      ndisp = process_char( (uint)wParam, disp, sizeof(disp)-1, ndisp );
    InvalidateRect(hwnd, NULL, FALSE), UpdateWindow(hwnd);
    break;
  case WM_DESTROY:
    KillTimer(hwnd, ID_TIMER);
    break;
  case WM_CLOSE: // Window is closing
    KillTimer(hwnd, ID_TIMER);
    kill_tools(true);
    cleanup();
    free_lines(voiceLines);
    PostQuitMessage(0);
  default:
    return DefWindowProc(hwnd,message,wParam,lParam); _
  return 0; _

// Main function - register window class, create window, start message loop
int APIENTRY
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) __
  WNDCLASSEX windowClass;
  // Fill out the window class structure
  windowClass.cbSize = sizeof(WNDCLASSEX);
  windowClass.style = CS_HREDRAW | CS_VREDRAW | (pgmKind==K_SCR ? CS_SAVEBITS : 0);
  windowClass.cbClsExtra = windowClass.cbWndExtra = 0;
  windowClass.lpfnWndProc = WndProc;
  windowClass.hInstance = hInstance;
  windowClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
  windowClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
  windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
  windowClass.hbrBackground = GetStockObject(BLACK_BRUSH);
  windowClass.lpszMenuName = NULL;
  windowClass.lpszClassName = pgmKind==K_SCR ? "WindowsScreenSaverClass" : PGM_NAME;
  // Register window class
  if( !RegisterClassEx(&windowClass) )
    return 1;
  if( !read_ini() ) // read all parameters from ini file
    return 1;
  DWORD extstyle = g_ontop ? WS_EX_TOOLWINDOW | WS_EX_TOPMOST : WS_EX_TOOLWINDOW;
  DWORD winstyle = WS_POPUPWINDOW | WS_BORDER | WS_CAPTION;
  int w = g_width+6, h = g_height+29; // Add for border (really non-existing) and caption bar
  if( g_resizable ) __
    winstyle = WS_OVERLAPPED | WS_SYSMENU | WS_THICKFRAME | WS_VISIBLE | WS_CAPTION;
    w += 10; h += 10; _ // no visible borders, but this correction is neccessary anyway
  if( pgmKind==K_SCR ) winstyle = WS_VISIBLE | WS_BORDER; // enough!
  // Class is registered, parameters are set-up, so now's the time to create window
  HWND hwnd = CreateWindowEx( extstyle,      // extended style
    windowClass.lpszClassName, PROGRAM_NAME, // class name, program name
    winstyle, g_init_x,g_init_y, w,h,        // window style, initial position and size
    NULL, NULL,  // handles to parent and menu
    hInstance,   // application instance
    NULL);       // no extra parameters
  if( !hwnd ) __ // can it fail, really?
    MessageBox( NULL, "Could not create window!", "Error", MB_OK | MB_ICONEXCLAMATION);
    return 1; _
  if( pgmKind==K_SCR ) SetWindowLong(hwnd, GWL_STYLE, 0);
  ShowWindow(hwnd, pgmKind==K_SCR ? SW_SHOWMAXIMIZED : SW_SHOW);
  UpdateWindow(hwnd); // maybe not really needed
  MSG msg;
  while( GetMessage(&msg, NULL, 0, 0) > 0 ) __
    // Translate and dispatch to event queue
    TranslateMessage(&msg);
    DispatchMessage(&msg); _
  return msg.wParam; _ // I hope it's zero
