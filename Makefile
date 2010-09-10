top=.
include common.mak

all: $(TARGETS)

libs:
	$(MAKE) -C libs/ntdll

disk.img:
	#dd if=/dev/zero of=disk.img bs=1M count=0 seek=20480
	#dd if=/dev/zero of=disk.img bs=1M count=0 seek=4096
	#dd if=/dev/zero of=disk.img bs=1M count=128
	dd if=/dev/zero of=disk.img bs=1 count=0 seek=2072738304
	mkntfs -Ff -p 63 -L Native -H 255 -s 512 -S 63 disk.img

clean:
	rm -f *.o *.d nloader$(EXE) volumeinfo$(EXE) lznt1$(EXE)
	$(MAKE) -C libs/ntdll clean

nloader: nloader.o stubs.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

lznt1: libs/ntdll/lznt1.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

volumeinfo: volumeinfo.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ -lntdll

-include volumeinfo.d
-include nloader.d

.PHONY: all libs clean
