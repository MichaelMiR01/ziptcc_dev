ziptcc:	tcc.c
	$(CC) $(CPPFLAGS) -s -DCONFIG_TRIPLET="\"x86_64-linux-gnu\"" -include tcc.h -include zipvfs/zipl_iomap.c -DHAVE_ZIP_H -I. tcc.c -ldl -lm -lpthread -o ztcc
	$(CC) $(CPPFLAGS) -s -DCONFIG_TRIPLET="\"x86_64-linux-gnu\"" -include tcc.h -include zipvfs/zipl_iomap.c -DHAVE_ZIP_H -I. libtcc.c -ldl -lm -lpthread -shared -Wl,-soname -fPIC -o zlibtcc.so
	$(CC) $(CPPFLAGS) -s -DCONFIG_TRIPLET="\"x86_64-linux-gnu\"" -include tcc.h -include zipvfs/zipl_iomap.c -DHAVE_ZIP_H -I. -c libtcc.c -ldl -lm -lpthread -o zlibtcc.o
	$S$(AR) rcs zlibtcc.a zlibtcc.o
	
#$(CC) $(CPPFLAGS) -s zipvfs/zzipsetstub.c -o zzipsetstub
		
# create package tarball from *current* git branch (including tcc-doc.html
# and converting two files to CRLF)
ZIPVFS-VERSION = 0.40.0
ZIPVFS_SRC = zipvfs
ZIPVFS_TRG = zipvfs-$(ZIPVFS-VERSION)
ZIPVFS_TRGLIB = "$(ZIPVFS_TRG)/lib"
ZIPVFS_TRGINC = "$(ZIPVFS_TRG)/include"
ZIPVFS_TRGWIN = "$(ZIPVFS_TRG)/win32"

INSTALL = install -m755
INSTALLD = install -dm755
INSTALLBIN = install -m755 $(STRIP_$(CONFIG_strip))
STRIP_yes = -s

LIBTCC1_W = $(filter %-win32-libtcc1.a %-wince-libtcc1.a,$(LIBTCC1_CROSS))
LIBTCC1_U = $(filter-out $(LIBTCC1_W),$(LIBTCC1_CROSS))
IB = $(if $1,$(IM) mkdir -p $2 && $(INSTALLBIN) $1 $2)
IBw = $(call IB,$(wildcard $1),$2)
IF = $(if $1,$(IM) mkdir -p $2 && $(INSTALL) $1 $2)
IFd = $(if $1,$(IM) $(INSTALLD) $1 $2)
IFw = $(call IF,$(wildcard $1),$2)
IR = $(IM) mkdir -p $2 && cp -r $1/. $2
IC = $(IM) mkdir -p $2 && cp -r $1 $2
IM = $(info -> $2 : $1)@

B_O = bcheck.o bt-exe.o bt-log.o bt-dll.o

# install progs & libs
#make  ZIPVFS package
pkg: ztcc
	@-if [ ! -d $(ZIPVFS_TRG) ]; then mkdir $(ZIPVFS_TRG); fi
	@-cp ztcc $(ZIPVFS_TRG)

	@$(call IBw,$(PROGS) $(PROGS_CROSS),"$(ZIPVFS_TRG)")
	@$(call IFw,$(LIBTCC1) $(B_O) $(LIBTCC1_U),"$(ZIPVFS_TRG)/lib")
	@
	@$(call IC,$(TOPSRC)/include/*, "$(ZIPVFS_TRG)/include")
	@$(call IC,$(TOPSRC)/include/*.h $(TOPSRC)/tcclib.h,"$(ZIPVFS_TRG)/include/stdinc")
	@$(call IC,$(ZIPVFS_SRC)/doc/*, "$(ZIPVFS_TRG)/doc")
	@$(call $(if $(findstring .so,$(LIBTCC)),IBw,IFw),$(LIBTCC),"$(ZIPVFS_TRG)")
	@$(call IF,$(TOPSRC)/libtcc.h,"$(ZIPVFS_TRG)/include/libtcc")
	@$(call IC,$(TOPSRC)/win32/examples/*, "$(ZIPVFS_TRG)/examples")
ifneq "$(wildcard $(LIBTCC1_W))" ""
	@$(call IC,$(TOPSRC)/win32/*, "$(ZIPVFS_TRG)/win32")
	@$(call IFw,$(TOPSRC)/win32/lib/*.def $(LIBTCC1_W),"$(ZIPVFS_TRG)/win32/lib")
	@$(call IR,$(TOPSRC)/win32/include,"$(ZIPVFS_TRG)/win32/include")
	@$(call IF,$(TOPSRC)/include/*.h $(TOPSRC)/tcclib.h,"$(ZIPVFS_TRG)/win32/include")
endif
	#@find $(ZIPVFS_TRG)/include -maxdepth 1 -type f -delete

