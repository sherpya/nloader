top=.
include common.mak
include libs/ntdll/Makefile

all:: $(LIBS) $(STATIC_LIBS)

clean:
	rm -f *.o *.d *.dSYM nloader$(EXE) autochk lznt1$(EXE) volumeinfo.exe stubs_x64.o stubs_x64.d
	rm -f $(LIBS) $(STATIC_LIBS) $(CLEAN_TARGETS)

disk.img:
	#dd if=/dev/zero of=disk.img bs=1M count=0 seek=20480
	#dd if=/dev/zero of=disk.img bs=1M count=0 seek=4096
	#dd if=/dev/zero of=disk.img bs=1M count=128
	dd if=/dev/zero of=disk.img bs=1 count=0 seek=2072738304
	$(MKNTFS) -Ff -p 63 -L Native -H 255 -s 512 -S 63 disk.img

ifeq ($(ARCH),x86_64)
    STUBS_OBJ = stubs_x64.o
else
    STUBS_OBJ = stubs.o
endif

nloader$(EXE): nloader.o loader.o $(STUBS_OBJ) $(LIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

autochk_blob.o: autochk.exe
	objcopy -B $(OBJCOPY_BIN) -I binary -O $(OBJCOPY_FMT) $^ $@

autochk: autochk.o loader.o $(STUBS_OBJ) autochk_blob.o $(STATIC_LIBS)
	$(CC) $(CFLAGS) $(LDALONE) $(LDFLAGS) -o $@ $^
	$(STRIP) $@

lznt1$(EXE): libs/ntdll/lznt1.c
	$(CC) -DMAIN $(CFLAGS) $(LDFLAGS) -o $@ $^

volumeinfo.exe: volumeinfo.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ -lntdll

-include *.d
.PHONY: all clean
