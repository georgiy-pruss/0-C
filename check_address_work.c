/* check_address for work :)
** export:
** int is_address_bad( void* a ) // 0 - good, 1 - bad, 2 - can't tell
*/

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

typedef struct MapEntry { unsigned long long addr; unsigned long long size;
  int accs; /* 2 - w, 1 - r, 0 - no access */
  char* name; void* next; } MapEntry;

static MapEntry* make_mem_map( pid_t pid )
{
  char fname[PATH_MAX]; int cnt;
  FILE* f;
  MapEntry* m_first = 0; MapEntry* m_last = 0;
  sprintf( fname, "/proc/%ld/maps", (long)pid );
  f = fopen(fname, "r"); if(!f) return 0;
  for( cnt=0; !feof(f); ++cnt ) {
    char buf[PATH_MAX+100], perm[5], dev[6], mapname[PATH_MAX];
    unsigned long long begin, end, size, inode, foo;
    int n, accs; MapEntry* new_m;
    if( fgets(buf, sizeof(buf), f) == 0 )
      break;
    mapname[0] = '\0'; if(buf[strlen(buf)-1]=='\x0A')buf[strlen(buf)-1]='\0';
    sscanf(buf, "%llx-%llx %4s %llx %s %lld %[^<]", &begin, &end, perm, &foo, dev, &inode, mapname);
    size = end - begin;
    accs = 0;
    if(perm[0] == 'r') accs=1;
    if(perm[1] == 'w') accs=2;
    if( size!=0 )
    {
      new_m = (MapEntry*)malloc( sizeof(MapEntry) );
      new_m->addr = begin;
      new_m->size = size;
      new_m->accs = accs;
      new_m->name = strdup(mapname);
      new_m->next = 0;
      if( m_first==0 )
        m_first = new_m, m_last = new_m;
      else
        m_last->next = new_m, m_last = new_m;
    }
  }
  fclose(f);
  return m_first;
}

static int check_address( MapEntry* m, void* a )
{
  unsigned long long n;
  for( n=(unsigned long long)a; m; m = m->next )
    if( m->addr <= n && n < m->addr + m->size ) return m->accs;
  return -1; /* not in process' address space */
}

/*
static void print_mem_map( MapEntry* m )
{
  int n;
  for( n=0; m; m = m->next )
  {
    n = printf("%016llx (%lld)", m->addr, m->size);
    n += printf("%*s%2d ", 28-n, "", m->accs);
    printf("%*s %s\n", 31-n, "", m->name);
  }
}
*/

static void free_map( MapEntry* m )
{
  MapEntry* next;
  for( ; m; m = next )
  {
    next = m->next;
    free( m );
  }
}

int is_address_bad( void* a ) /* 0 - good, 1 - bad, 2 - can't tell */
{
  static MapEntry* m = 0;
  pid_t pid;
  if( m==0 )
  {
    pid = getpid();
    m = make_mem_map(pid);
    if( m==0 ) return -1;
  }
  return check_address( m, a) <= 0;
}
