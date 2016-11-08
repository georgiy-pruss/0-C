// compile with: gcc w10clk.c -lgdi32 -Wl,--subsystem,windows -o w10clk.exe
// (-std=c11 is default) if needed, -lcomdlg32 or -lcomctl32
// https://www.cygwin.com/faq.html#faq.programming.win32-no-cygwin
// http://parallel.vub.ac.be/education/modula2/technology/Win32_tutorial/index.html

// TODO round window, although it's not so urgent, square is good too
// TODO pop up calendar etc --> no, better stopwatch/timer
// TODO always on top --> option, and it's no good, and I have PowerPro
// TODO clear up evens:redraw situation; who calls what

// Trim fat from windows
#define WIN32_LEAN_AND_MEAN
#pragma comment(linker, "/subsystem:windows") // for MSVC only
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <windows.h>

// Parameters
COLORREF g_bgcolor = RGB(196,136,255); // bg color
int g_initX=6, g_initY=6;      // initial position
int g_width=200, g_height=212; // initial size
int g_tick_w = 4; COLORREF g_tick_rgb = RGB(64,64,64);
int g_shand_w = 1; COLORREF g_shand_rgb = RGB(0,0,0);
int g_mhand_w = 3; COLORREF g_mhand_rgb = RGB(0,0,0);
int g_hhand_w = 6; COLORREF g_hhand_rgb = RGB(0,0,0);
BOOL g_seconds = FALSE;
BOOL g_upd_title = FALSE;
BOOL g_resizable = FALSE;

void
read_ini()
{
  char fn[500];
  size_t fnsz = GetModuleFileName(NULL,fn,sizeof(fn));
  if( fnsz <= 0 || strcmp(fn+fnsz-4,".exe") != 0 ) return;
  strcpy(fn+fnsz-3,"ini");
  char s[100] = ""; DWORD rc; int x,y,r,g,b;
  rc = GetPrivateProfileString( "window", "init", "6,6", s, sizeof(s), fn);
  if( rc>2 && sscanf( s,"%d,%d", &x,&y )==2 ) { g_initX = x; g_initY = y; }
  rc = GetPrivateProfileString( "window", "size", "200,212", s, sizeof(s), fn);
  if( rc>2 && sscanf( s,"%d,%d", &x,&y )==2 ) { g_width = x; g_height = y; }
  rc = GetPrivateProfileString( "window", "bg", "196,136,255", s, sizeof(s), fn);
  if( rc>4 && sscanf( s,"%d,%d,%d", &r,&g,&b )==3 ) { g_bgcolor = RGB(r,g,b); }
  rc = GetPrivateProfileString( "window", "ticks", "4,64,64,64", s, sizeof(s), fn);
  if( rc>6 && sscanf( s,"%d,%d,%d,%d", &x,&r,&g,&b)==4 ) { g_tick_w = x; g_tick_rgb = RGB(r,g,b); }
  rc = GetPrivateProfileString( "window", "shand", "1,0,0,0", s, sizeof(s), fn);
  if( rc>6 && sscanf( s,"%d,%d,%d,%d", &x,&r,&g,&b)==4 ) { g_shand_w = x; g_shand_rgb = RGB(r,g,b); }
  rc = GetPrivateProfileString( "window", "mhand", "4,0,0,0", s, sizeof(s), fn);
  if( rc>6 && sscanf( s,"%d,%d,%d,%d", &x,&r,&g,&b)==4 ) { g_mhand_w = x; g_mhand_rgb = RGB(r,g,b); }
  rc = GetPrivateProfileString( "window", "hhand", "6,0,0,0", s, sizeof(s), fn);
  if( rc>6 && sscanf( s,"%d,%d,%d,%d", &x,&r,&g,&b)==4 ) { g_hhand_w = x; g_hhand_rgb = RGB(r,g,b); }
  rc = GetPrivateProfileString( "window", "seconds", "no", s, sizeof(s), fn);
  if( rc>1 ) g_seconds = strcmp(s,"yes")==0;
  rc = GetPrivateProfileString( "window", "title", "yes", s, sizeof(s), fn);
  if( rc>1 ) g_upd_title = strcmp(s,"yes")==0;
  rc = GetPrivateProfileString( "window", "resizable", "no", s, sizeof(s), fn);
  if( rc>1 ) g_resizable = strcmp(s,"yes")==0;
}

// Const and global state vars
const int ID_TIMER = 1;
RECT g_rcClient;
HBRUSH hbrBG;
HPEN hsPen, hmPen, hhPen, htPen;

const char*
calendar() // soon we'll have stopwatch instead
{
  return "Mo Tu We Th Fr Sa Su\n       1  2  3  4  5  6\n 7  8  9 10 11 12 13\n"
         "14 15 16 17 18 19 20\n21 22 23 24 25 26 27\n28 29 30";
}

double sind(double x) { return sin(x*M_PI/180); }
double cosd(double x) { return cos(x*M_PI/180); }

void
update_clock(HDC hdc,int halfw, int halfh)
{
  time_t t = time(NULL);
  struct tm* tmptr = localtime(&t);
  if( !tmptr ) return;
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
    SelectObject(hdc, hsPen);
    MoveToEx(hdc, halfw, halfh, NULL); LineTo(hdc, halfw + secx, halfh + secy);
  }
  SelectObject(hdc, hmPen);
  MoveToEx(hdc, halfw, halfh, NULL); LineTo(hdc, halfw + minx, halfh + miny);
  SelectObject(hdc, hhPen);
  MoveToEx(hdc, halfw, halfh, NULL); LineTo(hdc, halfw + hourx, halfh + houry);
}

void draw_clock(HDC hdc,int halfw, int halfh)
{
  SelectObject(hdc, htPen);
  int R = min(halfw,halfh);
  //lineWidth = 2;
  for(int i=0;i<12;++i)
  {
    int start_x = (int)( halfw+(0.88*R*cosd(30*i))+0.5 );
    int start_y = (int)( halfh+(0.88*R*sind(30*i))+0.5 );
    int end_x   = (int)( halfw+(0.96*R*cosd(30*i))+0.5 );
    int end_y   = (int)( halfh+(0.96*R*sind(30*i))+0.5 );
    MoveToEx(hdc,start_x,start_y,NULL);
    LineTo(hdc,end_x,end_y);
  }
}

void
redraw(HWND hwnd,HDC hDC)
{
  if(hwnd) hDC = GetDC(hwnd);
  SetBkMode(hDC, OPAQUE);
  FillRect(hDC, &g_rcClient, hbrBG);
  draw_clock( hDC, g_rcClient.right/2, g_rcClient.bottom/2 );
  update_clock( hDC, g_rcClient.right/2, g_rcClient.bottom/2 );
  if(hwnd) ReleaseDC(hwnd, hDC);
}

// Main Window Event Handler
LRESULT CALLBACK
WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch( message )
  {
    case WM_CREATE: // Window is being created
    {
      if( SetTimer(hwnd, ID_TIMER, 1000, NULL) == 0 )
        MessageBox(hwnd, "Could not SetTimer()!", "Error", MB_OK | MB_ICONEXCLAMATION);
      hbrBG = CreateSolidBrush(g_bgcolor);
      hsPen = CreatePen(PS_SOLID,g_shand_w,g_shand_rgb);
      hmPen = CreatePen(PS_SOLID,g_mhand_w,g_mhand_rgb);
      hhPen = CreatePen(PS_SOLID,g_hhand_w,g_hhand_rgb);
      htPen = CreatePen(PS_SOLID,g_tick_w,g_tick_rgb);
      return 0;
    }
    case WM_TIMER:
    {
      time_t t = time(NULL);
      struct tm* tmptr = localtime(&t);
      if( !tmptr ) return 0;
      if( g_upd_title )
      {
        char text[100];
        strftime( text, sizeof(text), "%T %a %m/%d", tmptr );
        SetWindowText(hwnd,text);
      }
      redraw(hwnd,0);
      return 0;
    }
    case WM_SIZE: // will send WM_PAINT as well
    {
      GetClientRect(hwnd, &g_rcClient); // left = top = 0
      return 0;
    }
    case WM_PAINT: // Window needs update
    {
      SendMessage( hwnd, WM_TIMER, ID_TIMER, 0 ); // to have updated time
      PAINTSTRUCT paintStruct;
      HDC hDC = BeginPaint(hwnd,&paintStruct);
      redraw(0,hDC);
      EndPaint(hwnd, &paintStruct);
      return 0;
    }
    case WM_LBUTTONDOWN:
    {
      MessageBox(hwnd, calendar(), "Calendar", MB_OK); // or rather stopwatch
      return 0;
    }
    case WM_RBUTTONDOWN:
    {
      g_seconds = ! g_seconds;
      return 0;
    }
    case WM_CLOSE: // Window is closing
    {
      KillTimer(hwnd, ID_TIMER);
      DeleteObject(hsPen);
      DeleteObject(hmPen); DeleteObject(hhPen); DeleteObject(htPen);
      PostQuitMessage(0);
      return 0;
    }
    default:
      break;
  }
  return DefWindowProc(hwnd,message,wParam,lParam);
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
  windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
  windowClass.hbrBackground = CreateSolidBrush(g_bgcolor); // see also WM_ERASEBKGND
  windowClass.lpszMenuName = NULL;
  windowClass.lpszClassName = "w10clk";
  windowClass.hIconSm = LoadIcon(NULL, IDI_WINLOGO);
  // Register window class
  if( !RegisterClassEx(&windowClass) ) return 0;

  read_ini();

  DWORD style = (g_resizable ? WS_OVERLAPPED | WS_SYSMENU | WS_THICKFRAME | WS_VISIBLE
                             : WS_POPUPWINDOW | WS_BORDER) | WS_CAPTION;
  // Class registerd, so now create window
  HWND hwnd = CreateWindowEx( WS_EX_TOOLWINDOW, // extended style |WS_EX_TOPMOST
    windowClass.lpszClassName,                  // class name
    "Windows 10 Clock",                         // app name
    style,
    g_initX,g_initY, g_width,g_height, // initial position and size
    NULL,         // handle to parent
    NULL,         // handle to menu
    hInstance,    // application instance
    NULL);        // no extra parameter's
  // Check if window creation failed
  if( !hwnd )
  {
    MessageBox( NULL, "Could not create window!", "Error", MB_OK | MB_ICONEXCLAMATION);
    return 1;
  }
  ShowWindow(hwnd, SW_SHOW); // needed when WS_POPUPWINDOW
  UpdateWindow(hwnd); // not really needed, left since 1990?
  MSG msg;
  while( GetMessage(&msg, NULL, 0, 0) > 0 )
  {
    // Translate and dispatch to event queue
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  return msg.wParam;
}
