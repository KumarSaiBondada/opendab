//
// see en 300 401
//
// scrambling for FIC
//
#include <stdio.h>



//    P(X) = x9 + x5 + 1


//10 Energy dispersal
//10.1 General procedure
//10.2 Energy dispersal as applied in the Fast Information Channel
int scramble(char *logfile, unsigned char *ibuf, unsigned char *obuf, int len)
{
    FILE *Op = 0;
    unsigned short v;
    int v0;
    int i;

    if (logfile) Op = fopen(logfile,"wt");

    v = 0x1ff;
    for (i=0; i<len; i++)
    {
        v = v << 1;
        v0 = ((v>>9)&1) ^ ((v>>5)&1);
        v |= v0;

        obuf[i] = ibuf[i] ^ v0;

        if (Op) fprintf(Op,"%d ",(unsigned char)obuf[i]);
    }
    if (Op) fprintf(Op,"\n");
    if (Op) fclose(Op);

    return 0;
}
