top=.
include common.mak

all: libs $(TARGETS)

libs:
	$(MAKE) -C libs/ntdll

disk.img:
	#dd if=/dev/zero of=disk.img bs=1M count=0 seek=20480
	#dd if=/dev/zero of=disk.img bs=1M count=0 seek=4096
	#dd if=/dev/zero of=disk.img bs=1M count=128
	dd if=/dev/zero of=disk.img bs=1 count=0 seek=2072738304
	$(MKNTFS) -Ff -p 63 -L Native -H 255 -s 512 -S 63 disk.img

clean:
	rm -f *.o *.d nloader$(EXE) autochk volumeinfo$(EXE) lznt1$(EXE)
	$(MAKE) -C libs/ntdll clean

nloader: nloader.o loader.o stubs.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

autochk_blob.o: autochk.exe
	objcopy -B i386 -I binary -O elf32-i386 $^ $@

autochk: autochk.o loader.o stubs.o autochk_blob.o | libs
	$(CC) $(CFLAGS) $(LDALONE) $(LDFLAGS) -o $@ $^

lznt1: libs/ntdll/lznt1.c
	$(CC) -DMAIN $(CFLAGS) $(LDFLAGS) -o $@ $^

volumeinfo: volumeinfo.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ -lntdll

-include *.d
.PHONY: all libs clean
