cp ./ztcc _ztcc
#mv ./includes.zip ztcc
cat _ztcc includes.zip > ztcc
zip -A ztcc

cp ./ztcc.exe _ztcc.exe
#mv ./includes.zip ztcc.exe

cat _ztcc.exe includes.zip > ztcc.exe
zip -A ztcc.exe

