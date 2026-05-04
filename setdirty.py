#!/usr/bin/env python3
"""Set the NTFS dirty flag (bit 0x01 of $VOLUME_INFORMATION.Flags) on
disk.img by patching $Volume MFT entry 3 in BOTH $MFT and $MFTMirr."""
import struct, sys

if len(sys.argv) != 2:
    sys.exit("usage: setdirty.py <image>")

path = sys.argv[1]
with open(path, "r+b") as f:
    boot = f.read(512)
    bps = struct.unpack_from("<H", boot, 0x0B)[0]
    spc = boot[0x0D]
    mft_lcn = struct.unpack_from("<Q", boot, 0x30)[0]
    mftmirr_lcn = struct.unpack_from("<Q", boot, 0x38)[0]
    cs = bps * spc
    rec_size = 1024  # default for typical NTFS

    def patch(start_off):
        # MFT entry 3 = $Volume
        rec_off = start_off + 3 * rec_size
        f.seek(rec_off)
        rec = bytearray(f.read(rec_size))
        attr_off = struct.unpack_from("<H", rec, 0x14)[0]
        while attr_off < rec_size - 4:
            atype = struct.unpack_from("<I", rec, attr_off)[0]
            if atype == 0xFFFFFFFF:
                break
            alen = struct.unpack_from("<I", rec, attr_off + 4)[0]
            if alen == 0:
                break
            nonres = rec[attr_off + 8]
            if atype == 0x70 and nonres == 0:  # $VOLUME_INFORMATION resident
                value_off = struct.unpack_from("<H", rec, attr_off + 0x14)[0]
                # NTFS_VOLUME_INFORMATION: 8 reserved, 1 major, 1 minor,
                # 2 bytes flags  -> Flags WORD at offset 10 of the value.
                flags_off = attr_off + value_off + 0x0A
                cur = struct.unpack_from("<H", rec, flags_off)[0]
                rec[flags_off:flags_off+2] = struct.pack("<H", cur | 0x0001)
                f.seek(rec_off)
                f.write(rec)
                print(f"patched dirty bit @ {rec_off:#x}")
                return True
            attr_off += alen
        return False

    if not patch(mft_lcn * cs):
        sys.exit("could not find $VOLUME_INFORMATION in $MFT")
    if not patch(mftmirr_lcn * cs):
        sys.exit("could not find $VOLUME_INFORMATION in $MFTMirr")
