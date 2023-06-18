/*  Override io.h for use in tcc4tcl 
    functions to override:
    lseek
    open
    close
    read
    
    Tcl IO Functions work on EFILE, io.h operate on int handles, 
    so we additionally need a mapping//hashtable to translate from one to the other
    
    in tcc.c beneath include tcc.h
    # @include "tcc.h"
    # @include "../tcl_iomap.c"
    #if ONE_SOURCE
    # @include "libtcc.c"
    #endif
    # @include "tcctools.c"
*/    

#ifdef HAVE_ZIP_H
#include "config_tcc4zip.h"

#ifdef NO_TCC_LIST_SYMBOLS
void tcc_list_symbols(TCCState *s, void *ctx,
    void (*symbol_cb)(void *ctx, const char *name, const void *val)) {
// dummy
    }
#endif

#include <fcntl.h>
#include "zipvfs.c"
#define open t_open
#define read t_read
#define lseek t_lseek
#define close t_close

#define MAXCHAN 128
#define CHANBASE 10000

// these are sometimes undefined under linux
#ifndef O_TEXT
#define O_TEXT 0x4000
#endif
#ifndef O_BINARY
#define O_BINARY 0x8000
#endif


/* taken from rosetta-code https://rosettacode.org/wiki/Stack#C */
#define DECL_STACK_TYPE(type, name)					\
typedef struct stk_##name##_t{type *buf; size_t alloc,len;}*stk_##name;	\
stk_##name stk_##name##_create(size_t init_size) {			\
	stk_##name s; if (!init_size) init_size = 4;			\
	s = tcc_malloc(sizeof(struct stk_##name##_t));			\
	if (!s) return 0;						\
	s->buf = tcc_malloc(sizeof(type) * init_size);			\
	if (!s->buf) { tcc_free(s); return 0; }				\
	s->len = 0, s->alloc = init_size;				\
	return s; }							\
int stk_##name##_push(stk_##name s, type item) {			\
	type *tmp;							\
	if (s->len >= s->alloc) {					\
		tmp = tcc_realloc(s->buf, s->alloc*2*sizeof(type));		\
		if (!tmp) return -1; s->buf = tmp;			\
		s->alloc *= 2; }					\
	s->buf[s->len++] = item;					\
	return s->len; }						\
type stk_##name##_pop(stk_##name s) {					\
	type tmp;							\
	if (!s->len) abort();						\
	tmp = s->buf[--s->len];						\
	if (s->len * 2 <= s->alloc && s->alloc >= 8) {			\
		s->alloc /= 2;						\
		s->buf = tcc_realloc(s->buf, s->alloc * sizeof(type));}	\
	return tmp; }							\
type stk_##name##_peek(stk_##name s) {					\
	type tmp;							\
	if (!s->len) abort();						\
	tmp = s->buf[s->len-1];						\
	return tmp; }							\
void stk_##name##_delete(stk_##name s) {				\
	tcc_free(s->buf); tcc_free(s); }
 
#define stk_empty(s) (!(s)->len)
#define stk_size(s) ((s)->len)
 
DECL_STACK_TYPE(int, int)

EFILE* _tcl_channels[MAXCHAN];
static int _chan_cnt =0;
static int _chaninit=0;
static stk_int chan_stk;


void _initchantable () {
    _tcl_channels[0]=NULL;
    _chan_cnt=0;
    _chaninit=1;
    for(int i=0;i<MAXCHAN;i++) {
        _tcl_channels[i]=NULL;
    }
    chan_stk = stk_int_create(0);
}

void _cleanchanlist () {
    while (_tcl_channels[_chan_cnt]==NULL) {
        _chan_cnt+=-1;
        if(_chan_cnt<1) break;
    }
    _chan_cnt++;
}    

int _chan2int (EFILE* chan) {
    // insert channel to array, incr cnt, return cnt
    // start with 1, not 0
    if(_chaninit==0) _initchantable();
    if(chan==NULL) return -1;
    _cleanchanlist ();
    _chan_cnt++;
    if (_chan_cnt>MAXCHAN) {
        // out of channels
        printf("Out of channels");
        return -1;
    }
    int i=1;
    while(i<_chan_cnt) {
        if(_tcl_channels[i]==NULL) {
            break;
        }
        i++;
    }
    _tcl_channels[i]=chan;
    //printf("Channel %d\n",CHANBASE+i);
    return CHANBASE+i;    
}

EFILE* _int2chan (int fd) {
    // get channel from array by int
    // start with 1 not 0
    EFILE* chan;
    int fn;
    if(fd<CHANBASE) {
        return NULL; // not a tclhannel
    }
    fn=fd-CHANBASE;
    if (fn>_chan_cnt) {
        return NULL;
    }
    chan= _tcl_channels[fn];
    return chan;
}

int t_dup (int fd) {
    //
    int fn;
    if(fd<CHANBASE) {; // not a tclhannel
        #undef dup
        fn=dup(fd);
        #define dup t_dup
        return fn;
    }
    return fd;
}

#define BUF_SIZE 1024
static inline char *_strcatn(char *buf, const char *s)
{
    int len,lens;
    len = strlen(buf);
    lens = strlen(s);
    if (len+lens < BUF_SIZE)
        strcat(buf, s);
    return buf;
}

int callOpen(const char * path, int flags, va_list args) {
    #undef open
  if (flags & (O_CREAT))
  {
    //mode_t mode = va_arg(args, mode_t);
    //fake mode_t to int to satisfy older gcc va_arg can't pass mode_t for some reason
    int mode = va_arg(args, int);
    return open(path, flags, mode);
  }
  else {
    int f= open(path, flags);
    return f;
  }
  #define open t_open
}
int t_open(const char *_Filename,int _OpenFlag,...) {
    EFILE* chan;
    char buf[BUF_SIZE];
    buf[0] = '\0';
    char* rdmode="RDONLY";
    int readmode=1;
    // interpret openflags to tcl
    int flagMask = 1 << 15; // start with high-order bit...
    while( flagMask != 0 )   // loop terminates once all flags have been compared
    {
      // switch on only a single bit...
      switch( _OpenFlag & flagMask )
      {
       //case O_RDONLY:
       //  _strcatn(buf,"RDONLY ");
       //  break;
       case O_WRONLY:
         rdmode="WRONLY";
         readmode=0;
         break;
       case O_RDWR:
         rdmode="RDWR";
         readmode=0;
         break;
       case O_APPEND:
         rdmode="APEND";
         readmode=0;
         break;
       case O_CREAT:
         rdmode="CREAT";
         readmode=0;
         break;
       case O_TRUNC:
         _strcatn(buf,"TRUNC ");
         readmode=0;
         break;
       case O_EXCL:
         _strcatn(buf,"EXCL ");
         break;
       case O_BINARY:
         _strcatn(buf,"BINARY ");
         break;
      }
      flagMask >>= 1;  // bit-shift the flag value one bit to the right
    }    
    _strcatn(buf,rdmode);
    if(readmode==0) {
        va_list args;
        va_start(args, _OpenFlag);
        return callOpen(_Filename, _OpenFlag, args);
    }
    if (strcmp(_Filename, "-") == 0) {
        //chan = Tcl_GetStdChannel(TCL_STDIN);
        //_Filename = "stdin";
        return -1;
    } else {
        chan= zip_fopen(_Filename);
        if (chan==NULL) {
            //printf("Searching file on disk %s\n",_Filename);
            va_list args;
            va_start(args, _OpenFlag);
            int f= callOpen(_Filename, _OpenFlag,args);
            return f;
        }
            
    }    
    return _chan2int(chan);
}

int t_close(int _FileHandle) {
    //
    EFILE* chan;
    chan=_int2chan(_FileHandle);
    if(chan!=NULL) {
        _tcl_channels[_FileHandle-CHANBASE]=NULL;
    }
    _cleanchanlist();
    
    if(chan==NULL) {
        #undef close
        return close(_FileHandle);
        #define close t_close
    }
    return zip_fclose(chan);
}

long t_lseek(int _FileHandle,long _Offset,int _Origin) {
    //
    EFILE* chan;
    chan=_int2chan(_FileHandle);
    if(chan==NULL) {
        #undef lseek
        return lseek(_FileHandle,_Offset, _Origin);
        #define lseek t_lseek
    }
    return zip_lseek(chan, _Offset, _Origin);
}

int t_read(int _FileHandle,void *_DstBuf,unsigned int _MaxCharCount) {
    //
    EFILE* chan;
    chan=_int2chan(_FileHandle);
    if(chan==NULL) {
        #undef read
        return read(_FileHandle,_DstBuf,_MaxCharCount);
        #define read t_read
    }
    return zip_fread(chan, (char * )_DstBuf, _MaxCharCount);

} 

#endif // HAVE_TCL_H
