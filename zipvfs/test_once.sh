./ztcc -vv -c test_once.c > result.txt 2>&1
./ztcc.exe -vv -c test_once.c >> result.txt 2>&1

./ztcc.exe -vv -shared examples/tclParser.c -Iinclude/generic -Iinclude/generic/win -ltclstub86elf >> result.txt 2>&1    
./ztcc.exe -vv -shared examples/dll.c >> result.txt 2>&1
./ztcc.exe -vv examples/hello_dll.c -ldll -L. >> result.txt 2>&1

./ztcc.exe -vv examples/libtcc_test.c -ltcc -L. -Ilibtcc -run >> result.txt 2>&1
./ztcc.exe -vv examples/libtcc_test.c -ltcc -L. -Ilibtcc >> result.txt 2>&1
./libtcc_test.exe >> result.txt 2>&1

./ztcc -vv examples/libtcc_test.c -ltcc -L. -Ilibtcc -o libtcc_test -Wl,-rpath=. >> result.txt 2>&1
./libtcc_test >> result.txt 2>&1
