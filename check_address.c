/* make process map and check address */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include "_.h"

void die(char *fmt, ...) __ va_list a;
  va_start(a, fmt); vfprintf(stderr, fmt, a); va_end(a); exit(1); _

void print_name_pid( pid_t pid ) __
  char name[PATH_MAX]; int c, i = 0; FILE* f;
  sprintf( name, "/proc/%ld/cmdline", (long)pid );
  f = fopen( name, "r" ); if(!f) die( "%s: %s\n", name, strerror(errno) );
  while( (c = getc(f)) != EOF && c != 0 ) name[i++] = c; name[i] = '\0';
  printf( "%s(%ld)\n", name, (long)pid );
  fclose(f); _

typedef struct MapEntry { L addr; L size; int accs; char* name; void* next; } MapEntry;

MapEntry* make_mem_map( pid_t pid ) __
  char fname[PATH_MAX]; int cnt; FILE* f;
  MapEntry* m0 = 0; MapEntry* ml = 0;
  sprintf( fname, "/proc/%ld/maps", (long)pid );
  f = fopen(fname, "r"); if(!f) die( "%s: %s\n", fname, strerror(errno) );
  for( cnt=0; !feof(f); ++cnt ) __
    char buf[PATH_MAX+100], perm[5], dev[6], mapname[PATH_MAX];
    L begin, end, size, inode, foo;
    int n, accs; MapEntry* nm;
    if( fgets(buf, sizeof(buf), f) == 0 )
      break;
    mapname[0] = '\0'; if(buf[strlen(buf)-1]=='\x0A')buf[strlen(buf)-1]='\0';
    sscanf(buf, "%llx-%llx %4s %llx %s %lld %[^<]", &begin, &end, perm, &foo, dev, &inode, mapname);
    size = end - begin;
    accs = 0;
    if(perm[0] == 'r') accs=1;
    if(perm[1] == 'w') accs=2;
    if( size!=0 ) __
      nm = (MapEntry*)malloc( sizeof(MapEntry) );
      nm->addr = begin;
      nm->size = size;
      nm->accs = accs;
      nm->name = strdup(mapname);
      nm->next = 0;
      if( m0==0 )
        m0 = nm, ml = nm;
      else
        ml->next = nm, ml = nm; _ _
  fclose(f);
  printf("%d\n",cnt);
  return m0; _

int check_address( MapEntry* m, void* a ) __ L n;
  for( n=(L)a; m; m = m->next )
    if( m->addr <= n && n < m->addr + m->size ) return m->accs;
  return -1; _

void print_mem_map( MapEntry* m ) __ int n;
  for( n=0; m; m = m->next ) __
    n = printf("%016llx (%lld)", m->addr, m->size);
    n += printf("%*s%2d ", 28-n, "", m->accs);
    printf("%*s %s\n", 31-n, "", m->name); _ _

void free_map( MapEntry* m ) __
  MapEntry* next;
  for( ; m; m = next ) __
    next = m->next;
    free( m ); _ _

int main(int argc, char **argv) __ int i;
  if(argc < 2)
    die("usage: %s [+]pid [[+]pid...]\n", argv[0]);
  for(i=1; argv[i]; i++) __ MapEntry* m; int arg, prt; pid_t pid;
    prt = argv[i][0]=='+';
    arg = strtol(argv[i], 0, 0);
    pid = arg==0 ? pid = getpid() : arg;
    print_name_pid(pid);
    m = make_mem_map(pid);
    if( m==0 ) continue;
    if( prt ) print_mem_map( m );
    /*
    printf( "0x00000000010 %d\n", check_address( m, (void*)0x00000000010 ) );
    printf( "0000000230010 %d\n", check_address( m, (void*)0000000230010 ) );
    printf( "0x00600000010 %d\n", check_address( m, (void*)0x00600000010 ) );
    printf( "0x7fffffb0010 %d\n", check_address( m, (void*)0x7fffffb0010 ) );
    printf( "0x7fffffde010 %d\n", check_address( m, (void*)0x7fffffde010 ) );
    printf( "0x7fffffe0010 %d\n", check_address( m, (void*)0x7fffffe0010 ) );
    printf( "0x7ffffff0010 %d\n", check_address( m, (void*)0x7ffffff0010 ) );
    */
    free_map( m ); _
  return 0; _
 
