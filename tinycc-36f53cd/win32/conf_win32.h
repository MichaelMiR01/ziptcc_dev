#ifdef _WIN32  
#define CONFIG_TCC_SYSINCLUDEPATHS "{B}/include;{B}/include/stdinc;{B}/include/winapi;{B}/win32;{B}/win32/winapi;"
#else
#define CONFIG_TCC_SYSINCLUDEPATHS "{B}/include/stdinc/:{B}/include/"
#endif

