# ziptcc_dev
A self containing, zipenabled version of the TinyCC Compiler

This is a patch/addon to tcc (TinyCC, the tiny C compiler), that enables tcc to read includes and libs not only from file system, but from a zipfile or even from itself, if it is built as an sfx executable zip (see below)

## The idea:
tcc is a small and very fast C compiler. I thought about having a tcc.exe, that would not need any additional include directories, but could contain the neccessary includes and libs and so on. The result is a tcc, that can read from a zip file (includes.zip) or from itself without the need for additional include directories.

## How it works
The patch at it's heart consists of redefining the four fileio functions, that tcc in it's actual mob (2023-05-22) uses: open, close, read and lseek.
Those get deferred to a zipvfs style routine, that search for a proper zip and eventually reads the data from there. If there is no zip the regular fileio function is called. Thus, without any zip, ziptcc behaves like a standard tcc. I built upon kuba-zip (link) for the zipvfs and used some code from rosetta code.
All files are stored in the subdir zipvfs/

## How to build
First configure and build a normal tcc, to get the necessary config etc. 
Then you just include zipl_iomap.c on the commandline of gcc (ok, and before that we need tcc.h also)

    gcc (yourflags) -include tcc.h -include zipvfs/zipl_iomap.c -DHAVE_ZIP_H -I. tcc.c -ldl -lm -lpthread -o ztcc
    
or for win32

    gcc.exe (yourflags) -m32 -include tcc.h -include zipvfs/zipl_iomap.c -DHAVE_ZIP_H -I. tcc.c -o ztcc.exe

## Includes ZIP
Zip the necessary includes into a file named includes.zip. Put it in the same directory as ztcc and ztcc will find it.
If you want to use a different file, you can use ztcc -Bzip:myfile.zip

## Make ztcc selfcontaining
Simply append the zip to the exe, adjust-sfx index of zip.

    cat ztcc.exe MYZIPFILE.zip > fattcc.exe 
    zip -A fattcc.exe

## What about libtcc.dll?
You can build it zip enabled and give the zip name via tcc_set_lib_path("zip:path_to_my_zipfile")

## How compatible is it to older/newer tcc versions
It should work for all versions, that rely on only those four fileio functions.
    - tested backward to mob tinycc-ac9eeea   from Dez-06 2022: ok

I try to update to latest mob from time to time and make a release, when tcc 0.9.28 is final

## Are there prebuilt packages?
Yes, I upload source&binary (linx x86_64 and win32) packages on a regular basis, when tcc mob updates.

The source packages are derived from the normal tcc mob, so build with

    configure
    make
    make ziptcc
    make pkg

This will produce a subdir called zipvfs-0.40.0 with all necessary files

Win32:
use 
  win32/build-zipvfs-win32.bat -t 32 -c path-to-your-gcc\gcc.exe -i installdir (default: ziptcc-0.40.0)
This will produce a subdir called zipvfs-0.40.0 with all necessary files

