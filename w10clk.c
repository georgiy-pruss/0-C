// compile with: gcc w10clk.c -lgdi32 -lcomdlg32 -Wl,--subsystem,windows -o w10clk.exe
// (-std=c11 is default)
// https://www.cygwin.com/faq.html#faq.programming.win32-no-cygwin
// http://parallel.vub.ac.be/education/modula2/technology/Win32_tutorial/index.html

// TODO 1. recentering/resizing
// TODO 2. draw clock hands and ticks
// TODO 3. round window
// TODO 4. pop up calendar etc
// TODO 5. always on top
// TODO 6. don't show application window in usual taskbar etc

/*
https://www-user.tu-chemnitz.de/~heha/petzold/ch05d.htm
SetPixel(hdc, x, y, crColor);
MoveToEx (hdc, xBeg, yBeg, NULL);
LineTo (hdc, xEnd, yEnd);
Rectangle(hdc, 1, 1, 5, 4); // within bounding box
Ellipse(hdc, xLeft, yTop, xRight, yBottom);
Arc   (hdc, xLeft, yTop, xRight, yBottom, xStart, yStart, xEnd, yEnd);
Chord (hdc, xLeft, yTop, xRight, yBottom, xStart, yStart, xEnd, yEnd);
Pie   (hdc, xLeft, yTop, xRight, yBottom, xStart, yStart, xEnd, yEnd);
*/

// Trim fat from windows
#define WIN32_LEAN_AND_MEAN
#pragma comment(linker, "/subsystem:windows") // for MSVC only
#include <stdio.h>
#include <time.h>
#include <windows.h>
#include <commdlg.h>

// Parameters
COLORREF g_bgcolor = RGB(196,136,255); // all bg color
int g_initX=100, g_initY=100;  // initial position
int g_width=280, g_height=280; // initial size

// Const and global state vars
const int ID_TIMER = 1;
HFONT g_hfFont = 0;
RECT g_rcClient;
HBRUSH hbrWhite, hbrGray;

const char*
calendar()
{
  return "Mo Tu We Th Fr Sa Su\n"
    "      1  2  3  4  5  6\n"
    " 7  8  9 10 11 12 13\n"
    "14 15 16 17 18 19 20\n"
    "21 22 23 24 25 26 27\n"
    "28 29 30";
}

// Windows Procedure Event Handler
LRESULT CALLBACK
WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch( message )
  {
    case WM_CREATE: // Window is being created
    {
      HDC hDC = GetDC(NULL);
      long lfHeight = -MulDiv(16, GetDeviceCaps(hDC, LOGPIXELSY), 72);
      ReleaseDC(NULL, hDC);
      HFONT hf = CreateFont(lfHeight, 0, 0, 0, 0, TRUE, 0, 0, 0, 0, 0, 0, 0,
          "Times New Roman");
      if( hf )
        g_hfFont = hf;
      else
        MessageBox(hwnd, "Font creation failed!", "Error", MB_OK | MB_ICONEXCLAMATION);
      if( SetTimer(hwnd, ID_TIMER, 1000, NULL) == 0 )
        MessageBox(hwnd, "Could not SetTimer()!", "Error", MB_OK | MB_ICONEXCLAMATION);
      hbrWhite = GetStockObject(WHITE_BRUSH);
      hbrGray  = GetStockObject(GRAY_BRUSH);
      return 0;
    }
    case WM_TIMER:
    {
      time_t t = time(NULL);
      struct tm* tmptr = localtime(&t);
      if( !tmptr )
      {
        MessageBox(hwnd, "Could not get localtime!", "Error", MB_OK | MB_ICONEXCLAMATION);
        return 0;
      }
      char text[100];
      size_t sz = strftime( text, sizeof(text), "%T", tmptr );
      HDC hDC = GetDC(hwnd);
      TextOut(hDC,50,98,text,sz);
      strftime( text, sizeof(text), "%T %a %m/%d", tmptr );
      SetWindowText(hwnd,text);
      ReleaseDC(hwnd, hDC);
      return 0;
    }
    case WM_SIZE:
    {
      GetClientRect(hwnd, &g_rcClient);
      char text[100];
      sprintf( text, "%d %d %d %d",
          g_rcClient.left, g_rcClient.top, g_rcClient.right, g_rcClient.bottom );
      HDC hDC = GetDC(hwnd);
      TextOut(hDC,50,80,text,strlen(text));
      SetBkMode(hDC,TRANSPARENT);
      TextOut(hDC,g_rcClient.right/2,g_rcClient.bottom/2,"^",1);
      int w=g_rcClient.right-g_rcClient.left;
      int h=g_rcClient.bottom-g_rcClient.top;
      SetBkMode(hDC,TRANSPARENT);
      Ellipse(hDC,g_rcClient.right-w/4, g_rcClient.bottom-h/4,
          g_rcClient.right, g_rcClient.bottom);
      ReleaseDC(hwnd, hDC);
      return 0;
    }
    case WM_PAINT: // Window needs update
    {
      SendMessage( hwnd, WM_TIMER, ID_TIMER, 0 ); // to have updated time
      PAINTSTRUCT paintStruct;
      HDC hDC = BeginPaint(hwnd,&paintStruct);
      HFONT hfOld = g_hfFont ? (HFONT)SelectObject(hDC, g_hfFont) : 0;
      // Set txt color to blue and background to yellow
      SetTextColor(hDC, (COLORREF)0x00FF0000); // TTBBGGRR
      SetBkColor(hDC, (COLORREF)0x0000FFFF);   // = RGB(255,255,0)
      SetBkMode(hDC, OPAQUE); // or SetBkMode(hDC, TRANSPARENT);
      // Display text in middle of window
      const char h_w_msg[] = "Hello, World!"; // Text for display
      //TextOut(hDC,150,150,h_w_msg,sizeof(h_w_msg)-1);
      RECT rc = {50,50,170,80};
      DrawText(hDC, h_w_msg, -1, &rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
      EndPaint(hwnd, &paintStruct);
      if( hfOld ) SelectObject(hDC, hfOld);
      return 0;
    }
    /*
    case WM_ERASEBKGND:
    {
      HDC hdc = GetDC(hwnd);
      RECT rc;
      GetClientRect(hwnd, &rc);
      SetMapMode(hdc, MM_ANISOTROPIC);
      SetWindowExtEx(hdc, 100, 100, NULL);
      SetViewportExtEx(hdc, rc.right, rc.bottom, NULL);
      FillRect(hdc, &rc, hbrWhite);
      for( int i = 0; i < 13; ++i )
      {
        int x = (i * 40) % 100;
        int y = ((i * 40) / 100) * 20;
        SetRect(&rc, x, y, x + 20, y + 20);
        FillRect(hdc, &rc, hbrGray);
      }
      return 1;
    }
    */
    case WM_LBUTTONDOWN:
    {
      MessageBox(hwnd, calendar(), "Calendar", MB_OK);
      return 0;
    }
    case WM_RBUTTONDOWN:
    {
      CHOOSECOLOR cc;
      static COLORREF acrCustClr[16]; // array of custom colors
      ZeroMemory(&cc, sizeof(cc));
      cc.lStructSize = sizeof(cc);
      cc.hwndOwner = hwnd;
      cc.lpCustColors = (LPDWORD) acrCustClr;
      cc.rgbResult = (COLORREF)0;
      cc.Flags = CC_FULLOPEN | CC_RGBINIT;
      if( ChooseColor(&cc)==TRUE )
        cc.rgbResult; // use it
        // hbrush = CreateSolidBrush(cc.rgbResult);
      return 0;
    }
    case WM_CLOSE: // Window is closing
    {
      if( g_hfFont ) DeleteObject( g_hfFont );
      KillTimer(hwnd, ID_TIMER);
      PostQuitMessage(0);
      return 0;
    }
    default:
      break;
  }
  return DefWindowProc(hwnd,message,wParam,lParam);
}

/*
 * also http://www.codeproject.com/Articles/215690/Minimal-WinApi-Window
 *
//  Get the rectangle
CRect rect;
GetWindowRect(&rect);
int w = rect.Width();
int h = rect.Height();
CRgn rgn1;
//  Create the top ellipse
rgn1.CreateEllipticRgn(1, 1, w, h/2 + 30)
//  Set the window region
SetWindowRgn(static_cast<HRGN>(rgn1.GetSafeHandle()), TRUE);
rgn1.Detach();
*/

// Main function - register window class, create window, start message loop
int APIENTRY
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  WNDCLASSEX windowClass;  //window class
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
    // was (HBRUSH)GetStockObject(WHITE_BRUSH);
  windowClass.lpszMenuName = NULL;
  windowClass.lpszClassName = "w10clk";
  windowClass.hIconSm = LoadIcon(NULL, IDI_WINLOGO);
  // Register window class
  if( !RegisterClassEx(&windowClass) )
    return 0;
  // Class registerd, so now create window
  HWND hwnd = CreateWindowEx( WS_EX_TOOLWINDOW,    // extended style |WS_EX_TOPMOST
    windowClass.lpszClassName,                     // class name
    "Windows 10 Clock",                            // app name
    WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_VISIBLE, // window style
    //WS_POPUPWINDOW | WS_BORDER | WS_CAPTION, // <-- good, but not resizable
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
  /*
  ShowWindow(hw, SW_HIDE);
  SetWindowLongPtr(hw, GWL_EXSTYLE,GetWindowLongPtr(hw, GWL_EXSTYLE)| WS_EX_TOOLWINDOW);
  ShowWindow(hw, SW_SHOW);  // main message loop
  */
  ShowWindow(hwnd, SW_SHOW);  // main message loop
  // ? ShowWindow(hwnd, iCmdShow);
  // ? UpdateWindow(hwnd);
  MSG msg;
  while( GetMessage(&msg, NULL, 0, 0) > 0 )
  {
    // Translate and dispatch to event queue
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  return msg.wParam;
}
