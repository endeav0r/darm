#include "darm.h"

#include <inttypes.h>

#include <stdio.h>
#include <stdlib.h>

int main (int argc, char * argv[])
{
    FILE * fh;
    size_t filesize;
    unsigned char * buf;

    fh = fopen(argv[1], "rb");
    if (fh == NULL) {
        fprintf(stderr, "could not open file %s\n", argv[1]);
        return -1;
    }

    fseek(fh, 0, SEEK_END);
    filesize = ftell(fh);
    fseek(fh, 0, SEEK_SET);

    printf("filesize: %d\n", filesize);

    buf = (unsigned char *) malloc(filesize);

    fread(buf, 1, filesize, fh);

    fclose(fh);

    size_t i;
    for (i = 0; i < (filesize / 4) * 4; i += 4) {
        uint32_t ins;
        ins  = buf[i    ] << 0;
        ins |= buf[i + 1] << 8;
        ins |= buf[i + 2] << 16;
        ins |= buf[i + 3] << 24;

        struct _darm darm;
        if (darm_dis(&darm, ins)) {
            fprintf(stderr, "darm error\n");
            return -1;
        }

        printf("%04x %08x %s\n", i, ins, darm_str(&darm, i));
    }

    return -1;
}
