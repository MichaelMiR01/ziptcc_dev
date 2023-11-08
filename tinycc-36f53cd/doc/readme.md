This set of files tries to automate the process of adopting new versions of tcc to zipvfs

It consists of some scripts that try to modify tcc.c and hook in the zipl_iomap.c

Furthermore it tries to hook the configure/make process as far as possible without modifying the Makefile

Howto:

1 Download tcc and unpack it into DIR

2 Put zipvfs_altmake as a subdir into DIR

3       Prepare build files

        cd DIR/zipvfs_altmake
        prepare_build.sh

3a under windows run 

        prepare_build_win32.bat

then you have to manually adopt tcc.c
or run mod_tcc.tcl from a tclsh/tclkit
The batch will try to call mod_tcc.tcl directly, 
so if a tclsh is in your system path this may work

4  build under linux:

        cd DIR
        ./configure
        make

this will make the normal tcc and libtcc1.a

        make ziptcc

now the neccessary files should be compiled, especially zipvfs.so

        make pkg

will make a subdir zipvfs-0.40.0 and place all necessary files there to make a tcl package

4a build under windows:

        cd DIR/win32
        build-zipvfs-win32.bat (-t 32 -c PATH/TO/GCC/gcc.exe)

will make a subdir zipvfs-0.40.0-pkg (or use the existing from the linux-build :-)

test:

        cd zipvfs-0.40.0
        tclsh test.tcl

For wine users:

the normal build-tcc.bat uses if (%1)== ... wich, at least under my version of wine, fail.
to avoid this, use replace_bat_for_wine.tcl wich will replace all () with _ _ in build-tcc.bat
so it will work under wine (and still under windows also)

Modifications to tcc sources
-none, all gets included from the compiler commandline

