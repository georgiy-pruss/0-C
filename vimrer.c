// vimrer -- vim registry-exec-registry ~~ simulate colorpicker -- for vCoolor.vim:
// let l:comm = s:path . '/../vimrer.exe HKCU Software\Veign\Pixeur\3.0 last_color ' .
//   a:hexColor . ' C:/BIN/Pixeur/pixeur.exe ' . s:blackHole

// Accept and parse arguments,
// Write some value to registry,
// Execute a program,
// Read value from registry,
// Output it to stdout

// gcc -O2 -std=c99 vimrer.c -o vimrer.exe && strip vimrer.exe

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include "_.h"

I reg_write( HKEY root, K S key, K S name, K S value ) __ // 0=OK
  HKEY hKey;
  DWORD keystatus = RegOpenKeyEx( root, key, 0, KEY_WRITE, &hKey );
  if( keystatus != ERROR_SUCCESS ) return 1;
  keystatus = RegSetValueEx( hKey, name, 0, REG_SZ, (LPBYTE)value, strlen(value)+1 );
  if( keystatus != ERROR_SUCCESS ) return RegCloseKey(hKey), 2;
  RegCloseKey(hKey); return 0; _

I reg_read( HKEY root, K S key, K S name, OUT S value, I size ) __ // 0=OK; ERROR_MORE_DATA
  HKEY hKey;
  DWORD keystatus = RegOpenKeyEx( root, key, 0, KEY_READ, &hKey );
  if( keystatus != ERROR_SUCCESS ) return 1;
  DWORD cbData = size;
  keystatus = RegQueryValueEx( hKey, name, NULL, NULL, (LPBYTE)value, &cbData );
  RegCloseKey(hKey);
  if( keystatus == ERROR_MORE_DATA ) return ERROR_MORE_DATA; // 234 or so
  if( keystatus != ERROR_SUCCESS ) return 2;
  return 0; _

I main( int ac, char* av[] ) __
  if( ac!=6 ) __
    printf( "Syntax: vimrer.exe hkroot key name value program\n" ); return 0; _
  K HKEY root = strcmp(av[1],"HKCU")==0 ? HKEY_CURRENT_USER :
                strcmp(av[1],"HKLM")==0 ? HKEY_LOCAL_MACHINE : HKEY_CLASSES_ROOT;
  if( reg_write( root, av[2], av[3], av[4]+1 ) != 0 ) return 1;
  system( av[5] );
  char data[200];
  if( reg_read( root, av[2], av[3], data, sizeof(data) ) != 0 ) return 2;
  printf( "#%s\n", data );
  return 0; _
