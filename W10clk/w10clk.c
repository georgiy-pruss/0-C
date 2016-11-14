// Windows 10 Clock - Copyright (C) Georgy Pruss 2016
// compile with cygwin64 (-std=c11 is default):
// gcc -O2 w10clk*.c -lgdi32 -Wl,--subsystem,windows -o w10clk.exe
// for experiments: -lcomdlg32 or -lcomctl32
// https://www.cygwin.com/faq.html#faq.programming.win32-no-cygwin
// http://parallel.vub.ac.be/education/modula2/technology/Win32_tutorial/index.html

// TODO date/time calculations, timezone conversions, what else?..
// TODO show help (w10clk.txt); show color selection window
// TODO argument of program as ini file etc.
// TODO round window, although it's not so urgent, square is good too

#define PROGRAM_NAME "Windows 10 Clock"
#define HELP_MSG "Unrecognized key. Press h or ? or F1 for help.\n\n" \
"Left button click - nothing, just bring focus to window\n" \
"Right button click - show/hide seconds hand\n\n" \
"Configure the clock appearance in file w10clk.ini\n\n" \
"Version 1.4 * Copyright (C) Georgiy Pruss 2016"

// Trim fat from windows
#define WIN32_LEAN_AND_MEAN
#pragma comment(linker, "/subsystem:windows") // for MSVC?
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#include "../_.h"
typedef unsigned int uint;
typedef int bool;
#define false 0
#define true 1
#define STRIEQ(s,t) (strcasecmp(s,t)==0) // may need to change for MSVC etc
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
char g_timefmt[100];    // strftime format of time in title
bool g_seconds,g_upd_title, g_resizable, g_ontop; // flags

char pgmFileName[500]; // last 4 chars can be .ini, .exe, .htm etc

void
read_ini() __ // all parameter names and default values are here!
  int fnmsz = GetModuleFileName(NULL,pgmFileName,sizeof(pgmFileName));
  if( fnmsz <= 0 || !STRIEQ(pgmFileName+fnmsz-4,".exe") ) return;
  strcpy(pgmFileName+fnmsz-3,"ini");
  char s[100] = ""; DWORD rc; int x,y,w,h,r,g,b;
  #define READINI(nm,def) rc = GetPrivateProfileString( "window", nm, def, s, sizeof(s), pgmFileName)
  #define BND(x,f,t) if(x<f)x=f;if(x>t)x=t
  READINI("xywh","5,0,200,200");
  if( rc>6 && sscanf( s,"%d,%d,%d,%d", &x,&y,&w,&h )==4 ) { g_init_x = x; g_init_y = y;
      g_width = w; g_height = h; BND( g_width, 8, 9999 ); BND( g_height, 8, 9999 ); }
  READINI("bg","60,0,120");
  if( rc>4 && sscanf( s,"%d,%d,%d", &r,&g,&b )==3 ) { BND(r,0,255);BND(g,0,255);BND(b,0,255);
    g_bgcolor = RGB(r,g,b); }
  READINI("ticks","3,210,210,210");
  if( rc>6 && sscanf( s,"%d,%d,%d,%d", &x,&r,&g,&b)==4 ) { g_tick_w = x; BND( g_tick_w, 0,50 );
    BND(r,0,255);BND(g,0,255);BND(b,0,255); g_tick_rgb = RGB(r,g,b); }
  READINI("shand","2,255,255,255");
  if( rc>6 && sscanf( s,"%d,%d,%d,%d", &x,&r,&g,&b)==4 ) { g_shand_w = x; BND( g_shand_w, 1,50 );
    BND(r,0,255);BND(g,0,255);BND(b,0,255); g_shand_rgb = RGB(r,g,b); }
  READINI("mhand","3,255,255,255");
  if( rc>6 && sscanf( s,"%d,%d,%d,%d", &x,&r,&g,&b)==4 ) { g_mhand_w = x; BND( g_mhand_w, 1,50 );
    BND(r,0,255);BND(g,0,255);BND(b,0,255); g_mhand_rgb = RGB(r,g,b); }
  READINI("hhand","5,255,255,255");
  if( rc>6 && sscanf( s,"%d,%d,%d,%d", &x,&r,&g,&b)==4 ) { g_hhand_w = x; BND( g_hhand_w, 1,50 );
    BND(r,0,255);BND(g,0,255);BND(b,0,255); g_hhand_rgb = RGB(r,g,b); }
  READINI("lengths","8,84,84,62");
  if( rc>9 && sscanf( s,"%d,%d,%d,%d", &x,&y,&w,&h)==4 ) { g_t_len = x; BND( g_t_len, 1,96 );
    g_sh_len = y; g_mh_len = w; g_hh_len = h;
    BND( g_sh_len, 1,99 ); BND( g_mh_len, 1,99 ); BND( g_hh_len, 1,99 ); }
  READINI("disp","20,40");
  if( rc>2 && sscanf( s,"%d,%d", &x,&y )==2 ) { BND(x,0,99); BND(y,0,99);
    g_disp_x = x; g_disp_y = y; }
  READINI("seconds","no");          if( rc>1 ) g_seconds = STRIEQ(s,"yes");
  READINI("intitle","no");          if( rc>1 ) g_upd_title = STRIEQ(s,"yes");
  READINI("resizable","yes");       if( rc>1 ) g_resizable = STRIEQ(s,"yes");
  READINI("ontop","no");            if( rc>1 ) g_ontop = STRIEQ(s,"yes");
  READINI("timefmt","%T %a %m/%d"); if( rc>1 && rc<sizeof(g_timefmt) ) { strcpy( g_timefmt, s ); } _

// Const (all upper-case) and global state vars (camel-style names)
const int ID_TIMER = 1;
RECT rcClient; // clock client area
HBRUSH hbrBG;    // clock background
HPEN hsPen, hmPen, hhPen, htPen;     // clock hands (1 pixel narrower than required)
HPEN hsPen2, hmPen2, hhPen2, htPen2; // half-tone borders of clock hands

COLORREF
mix_colors( COLORREF c1, COLORREF c2 ) __
  int r = ((c1&0xFF)+(c2&0xFF))/2;
  int g = (((c1&0xFF00)+(c2&0xFF00))/2) & 0xFF00;
  int b = (((c1&0xFF0000)+(c2&0xFF0000))/2) & 0xFF0000;
  return b|g|r; _

void
init_tools() __
  hbrBG = CreateSolidBrush(g_bgcolor);
  hsPen = CreatePen(PS_SOLID,max(g_shand_w-1,1),g_shand_rgb);
  hmPen = CreatePen(PS_SOLID,max(g_mhand_w-1,1),g_mhand_rgb);
  hhPen = CreatePen(PS_SOLID,max(g_hhand_w-1,1),g_hhand_rgb);
  htPen = CreatePen(PS_SOLID,max(g_tick_w-1,1),g_tick_rgb);
  hsPen2 = CreatePen(PS_SOLID,g_shand_w,mix_colors(g_bgcolor,g_shand_rgb));
  hmPen2 = CreatePen(PS_SOLID,g_mhand_w,mix_colors(g_bgcolor,g_mhand_rgb));
  hhPen2 = CreatePen(PS_SOLID,g_hhand_w,mix_colors(g_bgcolor,g_hhand_rgb));
  htPen2 = CreatePen(PS_SOLID,g_tick_w,mix_colors(g_bgcolor,g_tick_rgb)); _

void
help_on_error_input() { MessageBox(NULL, HELP_MSG, PROGRAM_NAME, MB_OK ); }

static void
show_help_file() __
  uint n=strlen(pgmFileName);
  if( n<=4 || pgmFileName[n-4]!='.' ) return;
  char h[300] = "file:///"; strcat( h, pgmFileName ); strcpy( h+strlen(h)-3, "htm" );
  for( char* p=h; *p; ++p ) if(*p=='\\') *p='/'; // not needed actually, but let it be
  ShellExecute(NULL, "open", h, NULL, NULL, SW_SHOWNORMAL); _

double sind(double x) { return sin(x*M_PI/180); }
double cosd(double x) { return cos(x*M_PI/180); }

void draw_clock(HDC hdc,int halfw, int halfh) __
  if( g_tick_w == 0 ) return;
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
  int s = tmptr->tm_sec;
  int m = tmptr->tm_min;
  int h = tmptr->tm_hour;
  if( h>12 ) h -= 12;

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

  if( g_seconds ) __
    SelectObject(hdc, hsPen2);
    MoveToEx(hdc, halfw, halfh, NULL); LineTo(hdc, halfw + secx, halfh + secy);
    SelectObject(hdc, hsPen);
    MoveToEx(hdc, halfw, halfh, NULL); LineTo(hdc, halfw + secx, halfh + secy); _
  SelectObject(hdc, hmPen2);
  MoveToEx(hdc, halfw, halfh, NULL); LineTo(hdc, halfw + minx, halfh + miny);
  SelectObject(hdc, hmPen);
  MoveToEx(hdc, halfw, halfh, NULL); LineTo(hdc, halfw + minx, halfh + miny);
  SelectObject(hdc, hhPen2);
  MoveToEx(hdc, halfw, halfh, NULL); LineTo(hdc, halfw + hourx, halfh + houry);
  SelectObject(hdc, hhPen);
  MoveToEx(hdc, halfw, halfh, NULL); LineTo(hdc, halfw + hourx, halfh + houry); _

void
update_title(HWND hwnd, const char* title) __
  if( title ) { SetWindowText(hwnd,title); return; }
  time_t t = time(NULL);
  struct tm* tmptr = localtime(&t); if( !tmptr ) return;
  char text[100];
  strftime( text, sizeof(text), g_timefmt, tmptr );
  SetWindowText(hwnd,text); _

char disp[100] = "";
uint ndisp = 0;

void
redraw_window(HWND hwnd) __
  time_t t = time(NULL);
  struct tm* tmptr = localtime(&t); if( !tmptr ) return;
  PAINTSTRUCT paintStruct;
  HDC hDC = BeginPaint(hwnd,&paintStruct);
    SetBkMode(hDC, OPAQUE);
    FillRect(hDC, &rcClient, hbrBG); // need to clean old hands, sorry
    draw_clock( hDC, rcClient.right/2, rcClient.bottom/2 );
    update_clock( hDC, rcClient.right/2, rcClient.bottom/2, tmptr );
    if( ndisp!=0 ) TextOut(hDC,rcClient.right*g_disp_x/100,rcClient.bottom*g_disp_y/100,disp,ndisp);
  EndPaint(hwnd, &paintStruct); _

void
paste_from_clipboard(HWND hwnd) __
  if( !OpenClipboard(hwnd) ) return;
  HGLOBAL hglb = GetClipboardData(CF_TEXT);
  if( !hglb ) return;
  LPSTR lpstr = GlobalLock(hglb);
  GlobalUnlock(hglb);
  CloseClipboard();
  char* s; char* d=disp; ndisp=0; uint cnt=0; // one blank for many chars<=32
  for( s=lpstr; *s && ndisp<sizeof(disp)-1; ++s )
    if( *s>32 ) { *d=*s; ++d; ++ndisp; cnt=0; } // just copy
    else if( cnt==0 ) { *d=' '; ++d; ++ndisp; cnt=1; } // set blank for first
  *d = '\0'; _

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

// Main Window Event Handler
LRESULT CALLBACK
WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) __
  switch( message ) __
  case WM_CREATE: // Window is being created, good time for init tasks
    if( SetTimer(hwnd, ID_TIMER, 1000, NULL) == 0 ) // tick every second
      MessageBox(hwnd, "Could not set timer!", "Error", MB_OK | MB_ICONEXCLAMATION);
    init_tools();
    break;
  case WM_SIZE: // WM_PAINT will be sent as well
    GetClientRect(hwnd, &rcClient); // left = top = 0
    break;
  case WM_TIMER:
    if( g_upd_title ) update_title(hwnd,NULL); // update caption every tick
    if( g_seconds || time(NULL)%10==0 )   // but window - every 10 s if w/o sec.hand
      InvalidateRect(hwnd, NULL, FALSE), UpdateWindow(hwnd);
    break;
  case WM_PAINT: // Window needs update for whatever reason
    redraw_window(hwnd);
    break;
  case WM_RBUTTONDOWN: // maybe menu with all fns?
    g_seconds = ! g_seconds;
    InvalidateRect(hwnd, NULL, FALSE), UpdateWindow(hwnd);
    break;
  case WM_KEYDOWN:
    if( wParam==VK_F1 ) show_help_file();
    break;
  case WM_CHAR:
    if( wParam==22 ) // ctrl+v
      paste_from_clipboard(hwnd);
    else if( wParam==3 && ndisp!=0 ) // ctrl+c
      copy_to_clipboard(hwnd);
    else if( wParam==20 ) // ctrl+t
      { g_upd_title = ! g_upd_title; if( ! g_upd_title ) update_title(hwnd,PROGRAM_NAME); }
    else if( wParam=='?' || wParam=='h' && ndisp==0 )
      show_help_file();
    else if( wParam=='c' && (ndisp==0 || ndisp!=0 && disp[0]=='#') ) __
      CHOOSECOLOR cc;
      static COLORREF acrCustClr[16]; // array of custom colors
      ZeroMemory(&cc, sizeof(cc));
      cc.lStructSize = sizeof(cc);
      cc.hwndOwner = hwnd;
      cc.lpCustColors = (LPDWORD)acrCustClr;
      cc.rgbResult = (COLORREF)0;
      if( ndisp!=0 && disp[0]=='#' ) { uint rgb; sscanf( disp+1, "%X", &rgb ); cc.rgbResult = rgb; }
      cc.Flags = CC_FULLOPEN | CC_RGBINIT;
      if( ChooseColor(&cc)==TRUE )
        ndisp = sprintf( disp, "#%X", cc.rgbResult ); _
    else
      ndisp = process_char( (uint)wParam, disp, sizeof(disp)-1, ndisp );
    InvalidateRect(hwnd, NULL, FALSE), UpdateWindow(hwnd);
    break;
  case WM_CLOSE: // Window is closing
    KillTimer(hwnd, ID_TIMER);
    DeleteObject(hsPen);
    DeleteObject(hmPen), DeleteObject(hhPen), DeleteObject(htPen);
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
  windowClass.style = CS_HREDRAW | CS_VREDRAW;
  windowClass.lpfnWndProc = WndProc;
  windowClass.cbClsExtra = 0;
  windowClass.cbWndExtra = 0;
  windowClass.hInstance = hInstance;
  windowClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
  windowClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
  windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
  windowClass.hbrBackground = CreateSolidBrush(g_bgcolor); // see also WM_ERASEBKGND
  windowClass.lpszMenuName = NULL;
  windowClass.lpszClassName = "w10clk";
  // Register window class
  if( !RegisterClassEx(&windowClass) ) return 0;

  read_ini(); // read all parameters from ini file
  DWORD extstyle = g_ontop ? WS_EX_TOOLWINDOW | WS_EX_TOPMOST : WS_EX_TOOLWINDOW;
  DWORD winstyle = WS_POPUPWINDOW | WS_BORDER | WS_CAPTION;
  int w = g_width+6, h = g_height+29; // Add for border (really non-existing) and caption bar
  if( g_resizable ) __
    winstyle = WS_OVERLAPPED | WS_SYSMENU | WS_THICKFRAME | WS_VISIBLE | WS_CAPTION;
    w += 10; h += 10; _ // no visible borders, but this correction is neccessary anyway
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
  ShowWindow(hwnd, SW_SHOW); // needed when WS_POPUPWINDOW
  UpdateWindow(hwnd); // maybe not really needed
  MSG msg;
  while( GetMessage(&msg, NULL, 0, 0) > 0 ) __
    // Translate and dispatch to event queue
    TranslateMessage(&msg);
    DispatchMessage(&msg); _
  return msg.wParam; _ // I hope it's zero

