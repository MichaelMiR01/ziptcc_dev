// additional configuration directives for tcc4tcl

#ifndef _USE_32BIT_TIME_T 
#define _USE_32BIT_TIME_T
#endif

//define PE_PRINT_SECTIONS

#ifndef _WIN32
#include <stdint.h>
#endif

#ifndef _WIN32
#ifdef TCC_TARGET_X86_64
# define CONFIG_LDDIR__ "lib/x86_64-linux-gnu"
# define CONFIG_LIBDIR "lib"
#endif
#endif

#ifdef HAVE_ZIP_H
/* system include paths */
#ifdef _WIN32  
#define CONFIG_TCC_SYSINCLUDEPATHS "{B}/include;{B}/include/stdinc;{B}/include/winapi;{B}/win32;{B}/win32/winapi;./include;./include/stdinc;./include/winapi;./win32;./win32/winapi;"
#else
#define CONFIG_TCC_SYSINCLUDEPATHS "{B}/include/stdinc/:{B}/include/:./include/stdinc/:./include/"\
    ":" ALSO_TRIPLET(CONFIG_SYSROOT "/usr/local/include") \
    ":" ALSO_TRIPLET(CONFIG_SYSROOT CONFIG_USR_INCLUDE)
#endif
/*#define CONFIG_TCCDIR "."*/

/* library search paths */
# ifdef _WIN32
#  define CONFIG_TCC_LIBPATHS "{B}/lib_win32;{B}/libtcc;./lib_win32;./libtcc"
# else
#  define CONFIG_TCC_LIBPATHS \
        "{B}/lib:{B}/libtcc:./lib:./libtcc" \
    ":" ALSO_TRIPLET(CONFIG_SYSROOT "/usr/" CONFIG_LIBDIR) \
    ":" ALSO_TRIPLET(CONFIG_SYSROOT "/" CONFIG_LIBDIR) \
    ":" ALSO_TRIPLET(CONFIG_SYSROOT "/usr/local/" CONFIG_LIBDIR)
# endif

#endif 
