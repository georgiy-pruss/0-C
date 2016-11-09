// Windows 10 Clock - Copyright (C) Georgy Pruss 2016
// compile with cygwin64: gcc w10clk.c -lgdi32 -Wl,--subsystem,windows -o w10clk.exe
// (-std=c11 is default) in experiments also -lcomdlg32 or -lcomctl32
// https://www.cygwin.com/faq.html#faq.programming.win32-no-cygwin
// http://parallel.vub.ac.be/education/modula2/technology/Win32_tutorial/index.html

// TODO round window, although it's not so urgent, square is good too
// TODO functions on keypress --
// stopwatch: s - start, l - lap time, f - finish
// timer: HHMMt - set timer, 0t - reset timer (HHMM can be M, MM, HMM, HHMM)
// alarm: HHMMa - set alarm, 0a - reset alarm
// calendar: d - show date data incl. week, year day number, tjd etc.
// datecalc: NNNd/k/m - date in (or back if D/K/M) NNN days/weeks/months
// datedist: YYYYMMDD= - calculate distance to date (and its weekday) calendar?
// timezone: z - tz info, HHw,HHe - time in another timezone (west, east)
// unixtime: u - show unixtime, n - show new time (mine :))
// view: c - center window, [,],{,} - resize, tab - show/hide sec.hand
// calculator: NUM. +-*/%()= ...  <XXXX> - hex 2 dec, # - show in hex
// others: o - show color dialog, ?,h - help window, q,x - exit
// TODO convert units, temperature
// TODO i18n, utf8?
// TODO plugins :D

#define HELP_MSG "Right click - show/hide seconds hand\n\n"\
"Configure the clock appearance in file w10clk.ini"

// Trim fat from windows
#define WIN32_LEAN_AND_MEAN
#pragma comment(linker, "/subsystem:windows") // for MSVC only
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <windows.h>
#define STRIEQ(s,t) (strcasecmp(s,t)==0) // may need to change for MSVC etc

// Parameters; all defaults are in read_int()
COLORREF g_bgcolor;    // bg color
int g_initX, g_initY;  // initial position
int g_width, g_height; // initial size
int g_tick_w;  COLORREF g_tick_rgb;  // width and color of ticks (0 - no ticks)
int g_shand_w; COLORREF g_shand_rgb; // s,m,h hands
int g_mhand_w; COLORREF g_mhand_rgb;
int g_hhand_w; COLORREF g_hhand_rgb;
char g_timefmt[100];   // strftime format of time in title
BOOL g_seconds,g_upd_title, g_resizable, g_ontop; // flags

void
read_ini() // all parameter names and default values are here!
{
  char fnm[500];
  int fnmsz = GetModuleFileName(NULL,fnm,sizeof(fnm));
  if( fnmsz <= 0 || !STRIEQ(fnm+fnmsz-4,".exe") ) return;
  strcpy(fnm+fnmsz-3,"ini");
  char s[100] = ""; DWORD rc; int x,y,r,g,b;
  #define READINI(nm,def) rc = GetPrivateProfileString( "window", nm, def, s, sizeof(s), fnm)
  READINI("init","5,0");
  if( rc>2 && sscanf( s,"%d,%d", &x,&y )==2 ) { g_initX = x; g_initY = y; }
  READINI("size","200,222");
  if( rc>2 && sscanf( s,"%d,%d", &x,&y )==2 ) { g_width = x; g_height = y; }
  READINI("bg","60,0,120");
  if( rc>4 && sscanf( s,"%d,%d,%d", &r,&g,&b )==3 ) { g_bgcolor = RGB(r,g,b); }
  READINI("ticks","3,210,210,210");
  if( rc>6 && sscanf( s,"%d,%d,%d,%d", &x,&r,&g,&b)==4 ) { g_tick_w = x; g_tick_rgb = RGB(r,g,b); }
  READINI("shand","2,255,255,255");
  if( rc>6 && sscanf( s,"%d,%d,%d,%d", &x,&r,&g,&b)==4 ) { g_shand_w = x; g_shand_rgb = RGB(r,g,b); }
  READINI("mhand","3,255,255,255");
  if( rc>6 && sscanf( s,"%d,%d,%d,%d", &x,&r,&g,&b)==4 ) { g_mhand_w = x; g_mhand_rgb = RGB(r,g,b); }
  READINI("hhand","5,255,255,255");
  if( rc>6 && sscanf( s,"%d,%d,%d,%d", &x,&r,&g,&b)==4 ) { g_hhand_w = x; g_hhand_rgb = RGB(r,g,b); }
  READINI("seconds","no");          if( rc>1 ) g_seconds = STRIEQ(s,"yes");
  READINI("intitle","no");          if( rc>1 ) g_upd_title = STRIEQ(s,"yes");
  READINI("resizable","yes");       if( rc>1 ) g_resizable = STRIEQ(s,"yes");
  READINI("ontop","no");            if( rc>1 ) g_ontop = STRIEQ(s,"yes");
  READINI("timefmt","%T %a %m/%d"); if( rc>1 && rc<sizeof(g_timefmt) ) { strcpy( g_timefmt, s ); }
  if( g_shand_w <= 0 ) g_shand_w = 1;
  if( g_mhand_w <= 0 ) g_mhand_w = 1;
  if( g_hhand_w <= 0 ) g_hhand_w = 1;
  // ticks' width can be 0 (meaning no ticks at all)
}

// Const (all upper-case) and global state vars (camel-style names)
const int ID_TIMER = 1;
RECT rcClient; // clock client area
HBRUSH hbrBG;    // clock background
HPEN hsPen, hmPen, hhPen, htPen;     // clock hands (1 pixel narrower than required)
HPEN hsPen2, hmPen2, hhPen2, htPen2; // half-tone borders of clock hands

COLORREF mix_colors( COLORREF c1, COLORREF c2 )
{
  int r = ((c1&0xFF)+(c2&0xFF))/2;
  int g = (((c1&0xFF00)+(c2&0xFF00))/2) & 0xFF00;
  int b = (((c1&0xFF0000)+(c2&0xFF0000))/2) & 0xFF0000;
  return b|g|r;
}

void
init_tools()
{
  hbrBG = CreateSolidBrush(g_bgcolor);
  hsPen = CreatePen(PS_SOLID,max(g_shand_w-1,1),g_shand_rgb);
  hmPen = CreatePen(PS_SOLID,max(g_mhand_w-1,1),g_mhand_rgb);
  hhPen = CreatePen(PS_SOLID,max(g_hhand_w-1,1),g_hhand_rgb);
  htPen = CreatePen(PS_SOLID,max(g_tick_w-1,1),g_tick_rgb);
  hsPen2 = CreatePen(PS_SOLID,g_shand_w,mix_colors(g_bgcolor,g_shand_rgb));
  hmPen2 = CreatePen(PS_SOLID,g_mhand_w,mix_colors(g_bgcolor,g_mhand_rgb));
  hhPen2 = CreatePen(PS_SOLID,g_hhand_w,mix_colors(g_bgcolor,g_hhand_rgb));
  htPen2 = CreatePen(PS_SOLID,g_tick_w,mix_colors(g_bgcolor,g_tick_rgb));
}

double sind(double x) { return sin(x*M_PI/180); }
double cosd(double x) { return cos(x*M_PI/180); }

void draw_clock(HDC hdc,int halfw, int halfh)
{
  if( g_tick_w == 0 ) return;
  int R = min(halfw,halfh);
  for( int i=0; i<12; ++i )
  {
    int start_x = (int)( halfw+(0.88*R*cosd(30*i))+0.5 );
    int start_y = (int)( halfh+(0.88*R*sind(30*i))+0.5 );
    int end_x   = (int)( halfw+(0.96*R*cosd(30*i))+0.5 );
    int end_y   = (int)( halfh+(0.96*R*sind(30*i))+0.5 );
    SelectObject(hdc, htPen2);
    MoveToEx(hdc,start_x,start_y,NULL);
    LineTo(hdc,end_x,end_y);
    SelectObject(hdc, htPen);
    MoveToEx(hdc,start_x,start_y,NULL);
    LineTo(hdc,end_x,end_y);
  }
}

void
update_clock(HDC hdc,int halfw, int halfh, struct tm* tmptr)
{
  int s = tmptr->tm_sec;
  int m = tmptr->tm_min;
  int h = tmptr->tm_hour;
  if( h>12 ) h -= 12;

  double angle_sec = 270.0 + (s*6.0);
  double angle_min = 270.0 + (m*6.0 + s*0.1);
  double angle_hour = 270.0 + (h*30 + m*0.5);

  int R = min(halfw,halfh);
  int mlen = R*84/100;   // Length of minute hand (pixels?) 0.84
  int secx = (int)( mlen * cosd(angle_sec) );
  int secy = (int)( mlen * sind(angle_sec) );
  int minx = (int)( mlen * cosd(angle_min) );
  int miny = (int)( mlen * sind(angle_min) );
  int hourx = (int)( 0.73*mlen * cosd(angle_hour) );
  int houry = (int)( 0.73*mlen * sind(angle_hour) );

  if( g_seconds )
  {
    SelectObject(hdc, hsPen2);
    MoveToEx(hdc, halfw, halfh, NULL); LineTo(hdc, halfw + secx, halfh + secy);
    SelectObject(hdc, hsPen);
    MoveToEx(hdc, halfw, halfh, NULL); LineTo(hdc, halfw + secx, halfh + secy);
  }
  SelectObject(hdc, hmPen2);
  MoveToEx(hdc, halfw, halfh, NULL); LineTo(hdc, halfw + minx, halfh + miny);
  SelectObject(hdc, hmPen);
  MoveToEx(hdc, halfw, halfh, NULL); LineTo(hdc, halfw + minx, halfh + miny);
  SelectObject(hdc, hhPen2);
  MoveToEx(hdc, halfw, halfh, NULL); LineTo(hdc, halfw + hourx, halfh + houry);
  SelectObject(hdc, hhPen);
  MoveToEx(hdc, halfw, halfh, NULL); LineTo(hdc, halfw + hourx, halfh + houry);
}

void
update_title(HWND hwnd)
{
  time_t t = time(NULL);
  struct tm* tmptr = localtime(&t); if( !tmptr ) return;
  char text[100];
  strftime( text, sizeof(text), g_timefmt, tmptr );
  SetWindowText(hwnd,text);
}

void
redraw_window(HWND hwnd)
{
  time_t t = time(NULL);
  struct tm* tmptr = localtime(&t); if( !tmptr ) return;
  PAINTSTRUCT paintStruct;
  HDC hDC = BeginPaint(hwnd,&paintStruct);
    SetBkMode(hDC, OPAQUE);
    FillRect(hDC, &rcClient, hbrBG); // need to clean old hands, sorry
    draw_clock( hDC, rcClient.right/2, rcClient.bottom/2 );
    update_clock( hDC, rcClient.right/2, rcClient.bottom/2, tmptr );
  EndPaint(hwnd, &paintStruct);
}

// Main Window Event Handler
LRESULT CALLBACK
WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch( message )
  {
  case WM_CREATE: // Window is being created, good time for init tasks
    if( SetTimer(hwnd, ID_TIMER, 1000, NULL) == 0 ) // tick every second
      MessageBox(hwnd, "Could not set timer!", "Error", MB_OK | MB_ICONEXCLAMATION);
    init_tools();
    break;
  case WM_SIZE: // WM_PAINT will be sent as well
    GetClientRect(hwnd, &rcClient); // left = top = 0
    break;
  case WM_TIMER:
    if( g_upd_title ) update_title(hwnd); // update caption every tick if required
    if( g_seconds || time(NULL)%10==0 )   // but window - every 10 s if w/o sec.hand
      InvalidateRect(hwnd, NULL, FALSE), UpdateWindow(hwnd);
    break;
  case WM_PAINT: // Window needs update for whatever reason
    redraw_window(hwnd);
    break;
  case WM_LBUTTONDOWN: // to be changed...
    MessageBox(hwnd, HELP_MSG, "Windows 10 Clock Help", MB_OK);
    break;
  case WM_RBUTTONDOWN: // maybe menu with all fns?
    g_seconds = ! g_seconds;
    InvalidateRect(hwnd, NULL, FALSE), UpdateWindow(hwnd);
    break;
  case WM_CLOSE: // Window is closing
    KillTimer(hwnd, ID_TIMER);
    DeleteObject(hsPen);
    DeleteObject(hmPen), DeleteObject(hhPen), DeleteObject(htPen);
    PostQuitMessage(0);
  default:
    return DefWindowProc(hwnd,message,wParam,lParam);
  }
  return 0;
}

// Main function - register window class, create window, start message loop
int APIENTRY
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
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
  if( g_resizable )
  {
    winstyle = WS_OVERLAPPED | WS_SYSMENU | WS_THICKFRAME | WS_VISIBLE | WS_CAPTION;
    w += 10; h += 10; // no visible borders, but this correction is neccessary anyway
  }
  // Class is registered, parameters are set-up, so now's the time to create window
  HWND hwnd = CreateWindowEx( extstyle,            // extended style
    windowClass.lpszClassName, "Windows 10 Clock", // class name, program name
    winstyle, g_initX,g_initY, w,h,     // window style, initial position and size
    NULL, NULL, // handles to parent and menu
    hInstance,  // application instance
    NULL);      // no extra parameters
  if( !hwnd )   // can it fail, really?
  {
    MessageBox( NULL, "Could not create window!", "Error", MB_OK | MB_ICONEXCLAMATION);
    return 1;
  }
  ShowWindow(hwnd, SW_SHOW); // needed when WS_POPUPWINDOW
  UpdateWindow(hwnd); // maybe not really needed
  MSG msg;
  while( GetMessage(&msg, NULL, 0, 0) > 0 )
  {
    // Translate and dispatch to event queue
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  return msg.wParam; // I hope it's zero
}
