#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>

#ifdef _WIN32
#include "memrchr.c"
#endif

#include "zip.h"
#include "zip.c"

#if defined LIBTCC_AS_DLL && !defined CONFIG_TCCDIR
static HMODULE tcc_module;
BOOL WINAPI DllMain (HINSTANCE hDll, DWORD dwReason, LPVOID lpReserved)
{
    if (DLL_PROCESS_ATTACH == dwReason)
        tcc_module = hDll;
    return TRUE;
}
#else
#define tcc_module NULL /* NULL means executable itself */
#endif

#ifndef CONFIG_TCCDIR
/* on win32, we suppose the lib and includes are at the location of 'tcc.exe' */
static inline char *config_tccdir_w32(char *path)
{
    char temp[1024];
    char try[1024];
    char *p;
    int c;
    GetModuleFileName(tcc_module, path, MAX_PATH);
    p = tcc_basename(normalize_slashes(strlwr(path)));
    if (p > path)
        --p;

    *p = 0;

    return path;
}
#define CONFIG_TCCDIR config_tccdir_w32(alloca(MAX_PATH))
#endif

char* print_argv0() {
    char cmdline[2048];
#ifndef _WIN32
    int pid = getpid();
    char fname[2048];
    snprintf(fname, sizeof fname, "/proc/%d/cmdline", pid);
    FILE *fp = fopen(fname,"r");
    fgets(cmdline, sizeof cmdline, fp);
    // the arguments are in cmdline
#else
    strncpy(cmdline,__argv[0],256);
#endif
    /* Make a copy of the string. */
    //printf("%s\n", cmdline);
    /* Clean up. */
    return tcc_strdup(cmdline);

}

// Error Handling taken from c-embed

static int eerrcode = 0;
static int zipopen=0;

#define ethrow(err) { (eerrcode = (err)); return NULL; }
#define eerrno (eerrcode)

#define EERRCODE_SUCCESS 0
#define EERRCODE_NOFILE 1
#define EERRCODE_NOMAP 2
#define EERRCODE_NULLSTREAM 3
#define EERRCODE_OOBSTREAMPOS 4

const char* eerrstr(int e){
switch(e){
  case 0: return "Success.";
  case 1: return "No file found.";
  case 2: return "Mapping stucture error.";
  case 3: return "File stream pointer is NULL.";
  case 4: return "File stream pointer is out-of-bounds.";
  default: return "Unknown cembed error code.";
};
};

static char zipfilename [1024]={0}; // contains name of zipfile if given
static char zipfileopen [1024]={0}; // contains name of zipfile if given
static struct zip_t *zip=NULL;
static int exitreg =0;

void cleanupzip () {
    // close zip
    if(zip!=NULL) {
        //printf("Closing zip...\n");
        zip_close(zip);zip=NULL;
    }
    //printf("Exiting...\n");
}
#define eerror(c) printf("%s: (%u) %s\n", c, eerrcode, eerrstr(eerrcode))
char * normalize_path(char* pwd, const char * src, char* res) {
	size_t res_len;
	size_t src_len = strlen(src);

	const char * ptr = src;
	const char * end = &src[src_len];
	const char * next;

	if (src_len == 0 || src[0] != '/') {
		// relative path
		size_t pwd_len;

		pwd_len = strlen(pwd);
		memcpy(res, pwd, pwd_len);
		res_len = pwd_len;
	} else {
		res_len = 0;
		// handle absolute path case
		if(src[0]=='/') {
		    res[0]=src[0];
		    res_len++;
		}
	}
	
	for (ptr = src; ptr < end; ptr=next+1) {
		size_t len;
		next = (char*)memchr(ptr, '/', end-ptr);
		if (next == NULL) {
			next = end;
		}
		len = next-ptr;
		switch(len) {
		case 2:
			if (ptr[0] == '.' && ptr[1] == '.') {
				const char * slash = (char*)memrchr(res, '/', res_len);
				if (slash != NULL) {
					res_len = slash - res;
				} else {
				    res_len=0;
				}
				continue;
			}
			break;
		case 1:
			if (ptr[0] == '.') {
				continue;
			}
			break;
		case 0:
			continue;
		}

		if ((res_len >= 1) && (!IS_DIRSEP(res[res_len-1]))) 
			res[res_len++] = '/';
		
		memcpy(&res[res_len], ptr, len);
		res_len += len;
	}

	if (res_len == 0) {
		res[res_len++] = '/';
	}
	res[res_len] = '\0';
	
	return res;
}

struct EFILE_S {    // Virtual File Stream
  char* data;
  size_t size;
  size_t pos;
  size_t end;
  int err;
};
typedef struct EFILE_S EFILE;

int fexists(const char *fname)
{
    FILE *file;
    //printf("searhcing  %s\n",fname);
    if ((file = fopen(fname, "r")))
    {
        //printf("found %s\n",fname);
        fclose(file);
        return 1;
    }
    //printf("Error: %d %s\n",errno,strerror(errno));
    return 0;
}
/* extract the path of a file */
char *tcc_pathname(const char *name)
{
    
    char *parent = NULL;
    char *p = strchr(name, 0);
    int i=strlen(name);
    while (p > name && !IS_DIRSEP(p[-1])) {
        --p;
        --i;
    }
    parent = tcc_strdup(name);  
    *(parent+i)=0;
   // printf("Base of %s is %s (%d)\n",name,parent,i);
    return parent;
}

static size_t on_extract(void *arg, uint64_t offset, const void *data, size_t size) {
    struct EFILE_S *buf = (struct EFILE_S *)arg;
    buf->data = tcc_realloc(buf->data, buf->size + size + 1);
    //assert(NULL != buf->data);

    memcpy(&(buf->data[buf->size]), data, size);
    buf->size += size;
    buf->data[buf->size] = 0;
    return size;
}

EFILE* zip_fopen (const char* filename) {

    struct EFILE_S buf = {0};
    EFILE* e;
    int sz=0;
    char* ptr=NULL;
    static char* selfpath;
    static char* self;

    if (exitreg==0) {//register on_exit to close zip if neccessary
        atexit(cleanupzip);
        exitreg++;
        selfpath=CONFIG_TCCDIR;
        self=print_argv0();
        
        if(!fexists(self)) {
            // try selfpath/self
           // printf("Searching %s for self\n",selfpath);
            strncpy(zipfilename, selfpath, sizeof(zipfilename)-1);
            strcat(zipfilename, "/");
            strcat(zipfilename,self);
           // printf("Lookingup %s for self\n",zipfilename);
            if(fexists(zipfilename)) {
                self=tcc_strdup(zipfilename);
            } else {
               // printf("failed...\n");
            }
        } 
        if(!fexists(self)) {
           // printf("Searching %s for tcc.exe\n",selfpath);
            strncpy(zipfilename, selfpath, sizeof(zipfilename)-1);
            strcat(zipfilename,"/tcc.exe");
            if(fexists(zipfilename)) {
                self=tcc_strdup(zipfilename);
               // printf("found...\n");
            } 
        }
        
       // printf("Searching %s for includes\n",selfpath);
        strncpy(zipfilename, selfpath, sizeof(zipfilename)-1);
        strcat(zipfilename, "/includes.zip");

        if(!fexists(zipfilename)) {
            strncpy(zipfilename, tcc_pathname(self), sizeof(zipfilename)-1);
           // printf("Searching selfpath %s for includes\n",zipfilename);
            strcat(zipfilename, "includes.zip");
        }
            
        
       // printf("config_tccdir %s\n",CONFIG_TCCDIR);
       // printf("zipfilename  %s\n",zipfilename);
       // printf("self  %s\n",self);
    }

    if(tcc_state!=NULL) {
        //printf("Libpath is %s\n",tcc_state->tcc_lib_path);
        if (ptr=strstr(tcc_state->tcc_lib_path,"zip:")) {
            strncpy(zipfilename,ptr+4,strlen(tcc_state->tcc_lib_path)-4);
            if(!fexists(zipfilename)) {// get back last opened zipfile if any
                strcpy(zipfilename,zipfileopen);
            }
        } 
    }
    
    if(strcmp(zipfilename,zipfileopen)!=0) {
        if(zip!=NULL) {//reopen zip
            zip_close(zip);zip=NULL;zipfileopen[0]=0;
        }
    }
    
    if (zip==NULL) {// try opening zip or self
       if(strlen(zipfilename)>0&&fexists(zipfilename)) {
           // printf("Opening  zip %s\n",zipfilename);
            if(zip!=NULL) zip_close(zip);
            zip = zip_open(zipfilename, 0, 'r');
            if(zip==NULL) {
                return NULL;
            } 
            strcpy(zipfileopen,zipfilename);
        } else if(fexists(self)) {
           // printf("Opening self %s\n",self);
            if(zip!=NULL) zip_close(zip);
            zip = zip_open(self, 0, 'r');
            if(zip==NULL) {// self seems not to be a valid zip, so drop this
                self="";
                return NULL;
            } 
            strcpy(zipfilename,self);
            strcpy(zipfileopen,zipfilename);
        } else {
            zip=NULL;
        }
    }

    zipopen++;
    //printf("searching %s %d\n",filename,zipopen);
    if(zip==NULL) {
        //printf("zip not found \n");
        return NULL;
    }
    {
        char filenameok[1024];
        normalize_path("",filename,filenameok);
        if(IS_ABSPATH(filenameok)) return NULL;
        //printf("normalize %s -> %s\n",filename,filenameok);
        int r=zip_entry_open(zip, filenameok);
        if(r<0) {
            //printf("file not found %s as %s\n",filename,filenameok);
            zip_entry_close(zip);
            //zip_close(zip);zip=NULL;
            return NULL;
        }
        {
            //printf("file found %s as %s\n",filename,filenameok);
            sz=zip_entry_extract(zip, on_extract, &buf);
            sz=buf.size;
            if(sz<=0) {
                printf ("Extraction failed for %s\n",filenameok);
                zip_entry_close(zip);
                //zip_close(zip);zip=NULL;
                return NULL;
            }
            e = (EFILE*)tcc_malloc(sizeof *e);
            e->data=tcc_malloc(sz+1);
            memcpy(e->data,buf.data, sz);
            e->pos = 0;
            e->end = sz;
            e->size = sz;
            tcc_free(buf.data);
        }
        zip_entry_close(zip);
    }
    //zip_close(zip);zip=NULL;
    return e;
}

int zip_fclose (EFILE* file) {
    if(file!=NULL) {
        tcc_free(file->data);
        tcc_free(file);
    }
}

long int etell(EFILE* e){
  return e->end - e->pos;
}

int zip_lseek ( EFILE* stream, long int offset, int origin ){

  if(origin == SEEK_SET)
    stream->pos = offset;
  if(origin == SEEK_CUR)
    stream->pos += offset;
  if(origin == SEEK_END)
    stream->pos = stream->end + offset;

  if(stream->end < stream->pos || etell(stream)  < 0){
    (eerrcode = (EERRCODE_OOBSTREAMPOS));
    return -1;
  }

  return stream->pos;

}

long zip_fread (EFILE* stream, char* _DstBuf, unsigned int _MaxCharCount) {
    //

    if(stream->end - stream->pos < _MaxCharCount){
        //printf("reading %d bytes\n",_MaxCharCount);
        size_t scount = stream->end - stream->pos;
        memcpy(_DstBuf, (void*)stream->data+stream->pos, scount);
        stream->pos = stream->end;
        return scount;
    }
    
    memcpy(_DstBuf, (void*)stream->data+stream->pos, _MaxCharCount);
    stream->pos+=_MaxCharCount;
    return _MaxCharCount;

}

int zip_stat(const char *entryname, struct stat *st) {
  size_t entrylen = 0;
  mz_zip_archive *pzip = NULL;
  mz_zip_archive_file_stat stats;

  if (!zip) {
    return -1;
  }
  if (!entryname) {
    return -1;
  }
    if(IS_ABSPATH(entryname)) {
        //
        return -1;
    }

  entrylen = strlen(entryname);
  if (entrylen == 0) {
    return -1;
  }

  if (zip->entry.name) {
    CLEANUP(zip->entry.name);
  }
#ifdef ZIP_RAW_ENTRYNAME
  zip->entry.name = STRCLONE(entryname);
#else
  zip->entry.name = zip_strrpl(entryname, entrylen, '\\', '/');
#endif

  if (!zip->entry.name) {
    // Cannot parse zip entry name
    return -1;
  }

  pzip = &(zip->archive);
  if (pzip->m_zip_mode == MZ_ZIP_MODE_READING) {
    zip->entry.index = (ssize_t)mz_zip_reader_locate_file(
        pzip, zip->entry.name, NULL, 0);
    if (zip->entry.index < (ssize_t)0) {
      goto cleanup;
    }

    if (!mz_zip_reader_file_stat(pzip, (mz_uint)zip->entry.index, &stats)) {
      goto cleanup;
    }

    /*zip->entry.comp_size = stats.m_comp_size;
    zip->entry.uncomp_size = stats.m_uncomp_size;
    zip->entry.uncomp_crc32 = stats.m_crc32;
    zip->entry.offset = stats.m_central_dir_ofs;
    zip->entry.header_offset = stats.m_local_header_ofs;
    zip->entry.method = stats.m_method;
    zip->entry.external_attr = stats.m_external_attr;*/
    //zip->entry.m_time = stats.m_time;
    st->st_size=stats.m_uncomp_size;
    st->st_dev=stats.m_local_header_ofs;
    st->st_ino=stats.m_central_dir_ofs;
    return 0;
  }

cleanup:
  CLEANUP(zip->entry.name);
  return -1;
}

    