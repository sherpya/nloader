top=.
include common.mak
include libs/ntdll/Makefile

all:: $(LIBS) $(STATIC_LIBS)

clean:
	rm -f *.o *.d *.dSYM nloader$(EXE) autochk lznt1$(EXE) volumeinfo.exe
	rm -f $(LIBS) $(STATIC_LIBS) $(CLEAN_TARGETS)

disk.img:
	#dd if=/dev/zero of=disk.img bs=1M count=0 seek=20480
	#dd if=/dev/zero of=disk.img bs=1M count=0 seek=4096
	#dd if=/dev/zero of=disk.img bs=1M count=128
	dd if=/dev/zero of=disk.img bs=1 count=0 seek=2072738304
	$(MKNTFS) -Ff -p 63 -L Native -H 255 -s 512 -S 63 disk.img

nloader$(EXE): nloader.o loader.o stubs.o $(LIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

autochk_blob.o: autochk.exe
	objcopy -B i386 -I binary -O elf32-i386 $^ $@

autochk: autochk.o loader.o stubs.o autochk_blob.o $(STATIC_LIBS)
	$(CC) $(CFLAGS) $(LDALONE) $(LDFLAGS) -o $@ $^
	$(STRIP) $@

lznt1$(EXE): libs/ntdll/lznt1.c
	$(CC) -DMAIN $(CFLAGS) $(LDFLAGS) -o $@ $^

volumeinfo.exe: volumeinfo.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ -lntdll

-include *.d
.PHONY: all clean
