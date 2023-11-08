@rem ------------------------------------------------------
@rem batch file to build tcc using mingw, msvc or tcc itself
@rem ------------------------------------------------------
rem  .\build-zipvfs-win32.bat -t 32 -c ..\..\tcc_0.9.27-bin\gcc\bin\gcc.exe -i ..\zipvfsinst
@echo off
setlocal
if "%1"=="-clean" goto :cleanup
set CC=gcc
set INST=
set DOC=no
set EXES_ONLY=no
set HAS_WIN32_DIR=yes
set topdir=..
set tccdir=..
set zipvfsdir=..\zipvfs
set win32=.
set BIN=
set /p VERSION= <%tccdir%\VERSION
set OPTIONS=
goto :a0
:a2
shift
:a3
shift
:a0
if not _%1_==_-c_ goto :a1
set CC=%~2
if _%2_==_cl_ set CC=@call :cl
goto :a2
:a1
if _%1_==_-t_ set T=%2&& goto :a2
if _%1_==_-v_ set VERSION=%~2&& goto :a2
if _%1_==_-i_ set INST=%2&& goto :a2
if _%1_==_-b_ set BIN=%2&& goto :a2
if _%1_==_-d_ set DOC=yes&& goto :a3
if _%1_==_-x_ set EXES_ONLY=yes&& goto :a3
if _%1_==_-u_ set HAS_WIN32_DIR=no&& goto :a3
if _%1_==_-o_ set OPTIONS=%OPTIONS% -%2&& goto :a2
if _%1_==__ goto :p1
:usage
echo usage: build-zipvfs-win32.bat [ options ... ]
echo options:
echo   -c prog              use prog (gcc/tcc/cl) to compile zipvfs
echo   -c "prog options"    use prog with options to compile zipvfs
echo   -t 32/64             force 32/64 bit default target
echo   -v "version"         set tcc version
echo   -i tccdir            install tcc into tccdir
echo   -b bindir            optionally install binaries into bindir elsewhere
echo   -d                   create tcc-doc.html too (needs makeinfo)
echo   -x                   just create the executables
echo   -u                   use unified include dir (else will install two include paths include and win32
echo   -clean               delete all previously produced files and directories
exit /B 1

@rem ------------------------------------------------------
@rem sub-routines

:cleanup
echo Cleanup 
set LOG=echo
%LOG% removing files (partially outcommented MiR):
rem for %%f in (*tcc.exe libtcc.dll lib\*.a) do call :del_file %%f
for %%f in (%tccdir%\config.h %tccdir%\config.texi) do call :del_file %%f
rem for %%f in (include\*.h) do @if exist ..\%%f call :del_file %%f
rem for %%f in (include\tcclib.h examples\libtcc_test.c) do call :del_file %%f
for %%f in (*.o *.obj *.def *.pdb *.lib *.exp *.ilk) do call :del_file %%f
goto :the_end
%LOG% removing directories:
for %%f in (doc libtcc) do call :del_dir %%f
%LOG% done.
exit /B 0
:del_file
if exist %1 del %1 && %LOG%   %1
exit /B 0
:del_dir
if exist %1 rmdir /Q/S %1 && %LOG%   %1
exit /B 0

:cl
@echo off
set CMD=cl
:c0
set ARG=%1
set ARG=%ARG:.dll=.lib%
if "%1"=="-shared" set ARG=-LD
if "%1"=="-o" shift && set ARG=-Fe%2
set CMD=%CMD% %ARG%
shift
if not "%1"=="" goto :c0
echo on
%CMD% -O1 -W2 -Zi -MT -GS- -nologo -link -opt:ref,icf
@exit /B %ERRORLEVEL%

@rem ------------------------------------------------------
@rem main program

:p1
if not %T%_==_ goto :p2
set T=32
if %PROCESSOR_ARCHITECTURE%_==AMD64_ set T=64
if %PROCESSOR_ARCHITEW6432%_==AMD64_ set T=64
:p2
if "%CC:~-3%"=="gcc" set CC=%CC% -Os -s -static
set D32=-DTCC_TARGET_PE -DTCC_TARGET_I386 -D_WIN%T% -m%T%
set D64=-DTCC_TARGET_PE -DTCC_TARGET_X86_64 -D_WIN%T% -m%T%
set P32=i386-win32
set P64=x86_64-win32
if %T%==64 goto :t64
set D=%D32%
set DX=%D64%
set PX=%P64%
set TX=64
goto :p3
:t64
set D=%D64%
set DX=%D32%
set PX=%P32%
set TX=32
goto :p3

:p3
rem @echo off
echo starting
echo Version %VERSION%
echo T %T%
echo HAS_WIN32_DIR %HAS_WIN32_DIR%

for %%f in (*tcc.exe *tcc.dll) do @del %%f

:compiler
echo start building %D% 
rem @echo off

set CC2="%CC% -include conf_win32.h -O2 -s -m32"

echo calling ./buildtcc.bat  -t %T% -c %CC2%
call ./build-tcc.bat -t %T% -c %CC2%
@if errorlevel 1 goto :the_end
echo Ok, now building zipvfs
rem hack to keep the win libtcc1.a, so user can decide later wich to use
rem since grischka mob 0.9.28 Wed, 6 Sep 2023 22:21:15 +0200 (6 22:21 +0200)
rem there is no different naming to libtcc1 in win/linux
rem and we don't want to hack on the original build process, do we?
copy .\lib\libtcc1.a .\lib\libtcc1-%T%.a

:config.h
echo building zipvfs config
echo>%tccdir%\config.h #define TCC_VERSION "%VERSION%"
echo>>%tccdir%\config.h #ifdef TCC_TARGET_X86_64
echo>>%tccdir%\config.h #define TCC_LIBTCC1 "libtcc1-64.a"
echo>>%tccdir%\config.h #else
echo>>%tccdir%\config.h #define TCC_LIBTCC1 "libtcc1-32.a"
echo>>%tccdir%\config.h #endif
echo>>%tccdir%\config.h #ifdef _WIN32
echo>>%tccdir%\config.h #define CONFIG_WIN32 1
echo>>%tccdir%\config.h #define TCC_TARGET_PE 1
echo>>%tccdir%\config.h #endif
echo>>%tccdir%\config.h #define HOST_I386 1
echo>>%tccdir%\config.h #ifndef TCC_TARGET_X86_64
echo>>%tccdir%\config.h #define TCC_TARGET_I386 1
echo>>%tccdir%\config.h #endif
echo>>%tccdir%\config.h #ifdef _WIN32
echo>>%tccdir%\config.h #define _USE_32BIT_TIME_T 1
echo>>%tccdir%\config.h #endif

set STATICLIBC="-static-libgcc"
if "%filename%"=="tcc.exe" (
    set STATICLIBC=
)

echo compiling ziptcc
%CC% -o ztcc.exe %tccdir%/tcc.c %D% -include %tccdir%/tcc.h -include %zipvfsdir%/zipl_iomap.c -DHAVE_ZIP_H -DONE_SOURCE"=1" -I%tccdir% -O2 -s -m32
%CC% -o libtcc.dll -include %tccdir%/tcc.h -include %zipvfsdir%/zipl_iomap.c -shared %tccdir%/libtcc.c %D% -DLIBTCC_AS_DLL -DHAVE_ZIP_H -I%tccdir% -O2 -s -m32
rem %CC% -o zzipsetstub.exe %zipvfsdir%/zzipsetstub.c -O2 -s

@if "%EXES_ONLY%"=="yes" goto :files-done

@if errorlevel 1 goto :the_end
echo ready...

:tcc-doc.html
@if not "%DOC%"=="yes" goto :doc-done
echo>..\config.texi @set VERSION %VERSION%
cmd /c makeinfo --html --no-split ../tcc-doc.texi -o doc/tcc-doc.html
:doc-done

:files-done
for %%f in (*.o *.def) do @del %%f

:copy-install
@if "%INST%"=="" set INST=%topdir%\zipvfs-0.40.0
echo copy files to %INST%
if not exist %INST% mkdir %INST%
@if "%BIN%"=="" set BIN=%INST%
if not exist %BIN% mkdir %BIN%
for %%f in (*tcc.exe *tcc.dll *.dll) do @copy>nul %%~ff %BIN%\%%~nf%%~xf
copy %tccdir%\zipvfs\*.tcl %BIN%\
copy %topdir%\zipvfs\*.tcl %BIN%\
rem copy .\*.tcl %BIN%\

@if not exist %INST%\lib mkdir %INST%\lib

copy %zipvfsdir%\lib\*.a %INST%\lib\
copy %topdir%\lib\*.a %INST%\lib\
copy %win32%\lib\*.* %INST%\lib\
copy .\lib\lib\*.a %INST%\lib\

copy %zipvfsdir%\lib\*.def %INST%\lib\
copy %topdir%\lib\*.def %INST%\lib\
copy %win32%\lib\*.def %INST%\lib\
copy .\lib\lib\*.def %INST%\lib\

@if not exist %INST%\doc mkdir %INST%\doc
echo copy %zipvfsdir%\doc\* %INST%\doc\
copy %zipvfsdir%\doc\* %INST%\doc\

for %%f in (examples libtcc) do xcopy /s/i/q/y %win32%\%%f\ %INST%\%%f\
echo copying %topdir%\include %INST%\include
if not exist %INST%\include mkdir %INST%\include
xcopy /s/i/q/y %topdir%\include %INST%\include
rem echo deleting surplus files
rem del %INST%\include\*.*

set INSTWIN32=%INST%\include
if "%HAS_WIN32_DIR%"=="yes" set INSTWIN32=%INST%\win32

echo copying %win32%\include %INSTWIN32%
if not exist %INSTWIN32% mkdir %INSTWIN32%
xcopy /s/i/q/y %win32%\include %INSTWIN32%

:the_end
exit /B %ERRORLEVEL%

:getPath
rem call :getPath %Fullpath% filename folder
set "%2=%~nx1"
set "%3=%~dp1"
exit /b
