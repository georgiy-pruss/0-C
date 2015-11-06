int load_lib( const char* libname ) => 0 or handler
int load_proc( int h, const char* procname, const char* procargs ) => 0 or handler
int free_lib( int h ) => 0 or number of procedures removed
<how to call -- to follow>

procargs: "[opts][argspec]..."  blanks ignored
opts: + cdecl % fpreset
argspec: * pointer > pointer for output = return
  types: c C w W -- char uchar wchar_t unsigned wchar_t
         b B h H i I l L -- int8 uint8 int16 uint16 int32 uint32 int64 uint64
         f d -- float double (yet to do...)
         s t u -- *char *wchar_t *char-as-utf8
         p -- *void


J: see file x15.c

'filename procedure opts declaration' cd parameters

opts = [>][+][%]  unbox; stdcall  +=cdecl; % fpreset

parameters:
  c         1     C uc
  w         2
  s         2     S us
  i         4     I ui
  l         8     L ul
  f         4
  d         8
  *x        ptr
  n         no return

  dllcall 'kernel32 GetProfileStringA s *c *c *c *c s',
    'windows','device','default',,32
  ret 31


VS UNICODE/ASCII
Config props > general > char.set


win-64 calling convension:
rcx rdx r8  r9  then stack
ecx edx r8d r9d - for 32 bit
