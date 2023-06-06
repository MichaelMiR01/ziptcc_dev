#./tcc.exe ./zzipsetstub.c
#./tcc -Llib -Iinclude ./zzipsetstub.c -o zzipsetstub

#read -p "Press [Enter] key..."

zip -0 tcczip.zip ztcc.exe
zip -9 -r tcczip.zip win32/* include/* examples/* lib/* libtcc/*

./zzipsetstub tcczip.zip ztcc.exe
cp ./ztcc.exe _ztcc.exe
mv ./tcczip.zip ztcc.exe

zip -0 tcczip.zip ztcc
zip -9 -r tcczip.zip win32/* include/* examples/* lib/* libtcc/*

./zzipsetstub tcczip.zip ztcc
cp ./ztcc _ztcc
mv ./tcczip.zip ztcc