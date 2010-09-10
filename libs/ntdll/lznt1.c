/*
 * Copyright (c) 2010, Sherpya <sherpya@netfarm.it>, aCaB <acab@0xacab.net>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the copyright holders nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ntdll.h"

static NTSTATUS RtlDecompressBufferLZNT1(PUCHAR UncompressedBuffer, ULONG UncompressedBufferSize,
    PUCHAR CompressedBuffer, ULONG CompressedBufferSize, PULONG FinalUncompressedSize)
{
    UCHAR *c = CompressedBuffer, *d = UncompressedBuffer;

    if (!c || !d || !FinalUncompressedSize || !CompressedBufferSize)
        return STATUS_ACCESS_VIOLATION;

    *FinalUncompressedSize = UncompressedBufferSize;

    while (CompressedBufferSize)
    {
        uint16_t blocksz, blockpos = 0;
        unsigned int i, j;
        uint8_t mask;

        /* Block header */
        if (CompressedBufferSize < 2)
            return STATUS_INVALID_PARAMETER;

        blocksz = ((uint16_t) c[1] << 8) + *c;
        c += 2;
        CompressedBufferSize -= 2;

        if (!(blocksz & 0x8000)) // uncompressed chunk
        {
            blocksz = 4096;

            if(blocksz > UncompressedBufferSize)
                return STATUS_BAD_COMPRESSION_BUFFER;

            if(blocksz > CompressedBufferSize)
                return STATUS_BAD_COMPRESSION_BUFFER;

            memcpy(d, c, blocksz);
            d += blocksz;
            c += blocksz;
            UncompressedBufferSize -= blocksz;
            CompressedBufferSize   -= blocksz;
            continue;
        }

        blocksz &= 0xfff;
        blocksz++;

        while(blocksz)
        {
            /* Run mask */
            if (!CompressedBufferSize)
                return STATUS_BAD_COMPRESSION_BUFFER;

            mask = *c;
            c++;
            CompressedBufferSize--;
            blocksz--;

            for (i = 0; i < 8 && blocksz; i++)
            {
                if (mask & 1)
                {
                    uint16_t runlen, len = 0xfff, back = 12;
                    if (CompressedBufferSize < 2 || blocksz < 2)
                        return STATUS_BAD_COMPRESSION_BUFFER;

                    runlen = ((uint16_t) c[1] << 8) + *c;
                    c += 2;
                    CompressedBufferSize -= 2;
                    blocksz -= 2;

                    if (!blockpos)
                        return STATUS_BAD_COMPRESSION_BUFFER;

                    for (j = blockpos - 1; j >= 0x10; j >>= 1)
                    {
                        len >>= 1;
                        back--;
                    }

                    len = (runlen & len) + 3;
                    back = (runlen >> back) + 1;

                    if (UncompressedBufferSize < len)
                        return STATUS_BAD_COMPRESSION_BUFFER;

                    UncompressedBufferSize -= len;
                    blockpos += len;

                    while (len--)
                    {
                        *d = d[-back];
                        d++;
                    }
                }
                else
                {
                    if (!CompressedBufferSize || ! blocksz)
                        return STATUS_BAD_COMPRESSION_BUFFER;

                    if (!UncompressedBufferSize)
                        return STATUS_BAD_COMPRESSION_BUFFER;

                    *d++ = *c++;
                    UncompressedBufferSize--;
                    CompressedBufferSize--;
                    blockpos++;
                    blocksz--;
                }
                mask >>= 1;
            }
        }
    }

    *FinalUncompressedSize -= UncompressedBufferSize;
    return STATUS_SUCCESS;
}

#define COMPRESSION_FORMAT_LZNT1 2
NTSTATUS NTAPI RtlDecompressBuffer(USHORT CompressionFormat, PUCHAR UncompressedBuffer, ULONG UncompressedBufferSize,
    PUCHAR CompressedBuffer, ULONG CompressedBufferSize, PULONG FinalUncompressedSize)
{
    if (CompressionFormat != COMPRESSION_FORMAT_LZNT1)
        return STATUS_INVALID_PARAMETER;

    return RtlDecompressBufferLZNT1(UncompressedBuffer, UncompressedBufferSize, CompressedBuffer, CompressedBufferSize, FinalUncompressedSize);
}

int main(int argc, char *argv[])
{
    NTSTATUS res;
    struct stat st;
    ULONG outsize, final, fsize;
    unsigned char *inbuff, *outbuff;
    FILE *fin, *fout;

    if (argc != 3)
    {
        printf("Usage: %s input output\n", argv[0]);
        exit(1);
    }

    if (stat(argv[1], &st) < 0)
    {
        perror("stat");
        exit(1);
    }

    fsize = (ULONG) st.st_size;

    fin = fopen(argv[1], "rb");
    inbuff = malloc(fsize);
    fread(inbuff, 1, fsize, fin);
    fclose(fin);

    printf("Insize: %d\n", fsize);
    outsize = fsize * 2;
    outbuff = malloc(outsize);

    while ((res = RtlDecompressBuffer(COMPRESSION_FORMAT_LZNT1, outbuff, outsize, inbuff, fsize, &final)) == STATUS_BAD_COMPRESSION_BUFFER)
    {
        outsize *= 2;
        printf("reallocating %d\n", outsize);
        outbuff = realloc(outbuff, outsize);
    }

    printf("0x%08x Done %d\n", res, final);

    fout = fopen(argv[2], "wb");
    fwrite(outbuff, 1, final, fout);
    fclose(fout);

    free(inbuff);
    free(outbuff);
    return 0;
}
