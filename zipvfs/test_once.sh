#!/bin/bash

tcc=""
wintcc=""

zipinc="-Bzip=includes.zip"

filenames=('tcc' 'ztcc' 'ntcc')
#filenames=('_ztcc')

for filename in ${filenames[@]}; do
    if [ -f ./$filename ]; then
        echo "$filename exists."
		tcc="./$filename"
    else
        echo "$filename does not exist."
    fi
done

filenames=('tcc.exe' 'ztcc.exe' 'ntcc.exe')
#filenames=('_ztcc.exe')

for filename in ${filenames[@]}; do
    if [ -f ./$filename ]; then
        echo "$filename exists."
		wintcc="./$filename"
    else
        echo "$filename does not exist."
    fi
done

echo "Starting with $tcc // $wintcc"
echo "Starting with $tcc // $wintcc ($zipinc)" > result.txt 2>&1

$tcc -vv $zipinc -c test_once.c >> result.txt 2>&1
$wintcc -vv $zipinc -c test_once.c >> result.txt 2>&1

echo "compiling win32 examples" >> result.txt 2>&1
$wintcc -vv $zipinc -shared examples/tclParser.c -Iinclude/generic -Iinclude/generic/win -ltclstub86elf >> result.txt 2>&1    
$wintcc -vv $zipinc -shared examples/dll.c >> result.txt 2>&1
$wintcc -vv $zipinc examples/hello_dll.c -ldll -L. >> result.txt 2>&1

echo "compiling libtcc_text win32" >> result.txt 2>&1
$wintcc -vv $zipinc examples/libtcc_test.c -ltcc -L. -Ilibtcc -run >> result.txt 2>&1
$wintcc -vv $zipinc examples/libtcc_test.c -ltcc -L. -Ilibtcc >> result.txt 2>&1
echo "testing libtcc_test.exe win32" >> result.txt 2>&1
./libtcc_test.exe $zipinc >> result.txt 2>&1

echo "compiling libtcc_test lin64" >> result.txt 2>&1
$tcc -vv $zipinc examples/libtcc_test.c -Iinclude -ltcc -L. -Llib -Ilibtcc -o libtcc_test -Wl,-rpath=. >> result.txt 2>&1
echo "testing libtcc_test lin64" >> result.txt 2>&1
./libtcc_test $zipinc >> result.txt 2>&1
echo "ready." >> result.txt 2>&1
