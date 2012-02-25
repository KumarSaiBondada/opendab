//
// see en 300 401
//
// puncture scheme for FIC, mode 1
//
#include <stdio.h>



//11.1.2 Puncturing procedure
//11.2 Coding in the Fast Information Channel
//11.2.1 Transmission modes I, II, and IV
int puncture(char *logfile, unsigned char *ibuf, int ilen, unsigned char *obuf, int *olen)
{
    FILE *Op = 0;
    int i,j,k;

    if (logfile) Op = fopen(logfile,"wt");

    // 21 blocks, using puncture 1110 1110 1110 1110 1110 1110 1110 1110
    //  3 blocks, using puncture 1110 1110 1110 1110 1110 1110 1110 1100
    // 24 bits,   using puncture 1100 1100 1100 1100 1100 1100
    k = 0;
    for (i=0; i<21*128; i+=32)
    {
        for (j=0; j<8; j++)
        {
            obuf[k+0] = ibuf[i+j*4+0];
            obuf[k+1] = ibuf[i+j*4+1];
            obuf[k+2] = ibuf[i+j*4+2];
            if (Op) fprintf(Op,"%d %d %d\n",obuf[k+0],obuf[k+1],obuf[k+2]);
            k+=3;
        }
    }
    for (i=21*128; i<24*128; i+=32)
    {
        for (j=0; j<7; j++)
        {
            obuf[k+0] = ibuf[i+j*4+0];
            obuf[k+1] = ibuf[i+j*4+1];
            obuf[k+2] = ibuf[i+j*4+2];
            if (Op) fprintf(Op,"%d %d %d\n",obuf[k+0],obuf[k+1],obuf[k+2]);
            k+=3;
        }
        obuf[k+0] = ibuf[i+j*4+0];
        obuf[k+1] = ibuf[i+j*4+1];
        if (Op) fprintf(Op,"%d %d\n",obuf[k+0],obuf[k+1]);
        k+=2;
    }

    for (j=0; j<6; j++)
    {
        obuf[k+0] = ibuf[i+j*4+0];
        obuf[k+1] = ibuf[i+j*4+1];
        if (Op) fprintf(Op,"%d %d\n",obuf[k+0],obuf[k+1]);
        k+=2;
    }
    if (Op) fclose(Op);

    *olen = k;
    return 0;
}

//11.1.2 Puncturing procedure
//11.2 Coding in the Fast Information Channel
//11.2.1 Transmission modes I, II, and IV
int depuncture(char *logfile, unsigned char *ibuf, int ilen, unsigned char *obuf, int *olen)
{
    FILE *Op = 0;
    int i,j,k;

    if (logfile) Op = fopen(logfile,"wt");

    // 21 blocks, using puncture 1110 1110 1110 1110 1110 1110 1110 1110
    //  3 blocks, using puncture 1110 1110 1110 1110 1110 1110 1110 1100
    // 24 bits,   using puncture 1100 1100 1100 1100 1100 1100

    k = 0;
    for (i=0; i<21*128; i+=32)
    {
        for (j=0; j<8; j++)
        {
            obuf[i+j*4+0] = ibuf[k+0];
            obuf[i+j*4+1] = ibuf[k+1];
            obuf[i+j*4+2] = ibuf[k+2];
            obuf[i+j*4+3] = 8;              // mark depunctured bit for soft decision
            if (Op) fprintf(Op,"%d %d %d %d ",obuf[i+j*4+0],obuf[i+j*4+1],obuf[i+j*4+2],obuf[i+j*4+3]);
            k+=3;
        }
    }
    for (i=21*128; i<24*128; i+=32)
    {
        for (j=0; j<7; j++)
        {
            obuf[i+j*4+0] = ibuf[k+0];
            obuf[i+j*4+1] = ibuf[k+1];
            obuf[i+j*4+2] = ibuf[k+2];
            obuf[i+j*4+3] = 8;
            if (Op) fprintf(Op,"%d %d %d %d ",obuf[i+j*4+0],obuf[i+j*4+1],obuf[i+j*4+2],obuf[i+j*4+3]);
            k+=3;
        }
        obuf[i+j*4+0] = ibuf[k+0];
        obuf[i+j*4+1] = ibuf[k+1];
        obuf[i+j*4+2] = 8;
        obuf[i+j*4+3] = 8;
        if (Op) fprintf(Op,"%d %d %d %d ",obuf[i+j*4+0],obuf[i+j*4+1],obuf[i+j*4+2],obuf[i+j*4+3]);
        k+=2;
    }
    for (j=0; j<6; j++)
    {
        obuf[i+j*4+0] = ibuf[k+0];
        obuf[i+j*4+1] = ibuf[k+1];
        obuf[i+j*4+2] = 8;
        obuf[i+j*4+3] = 8;
        if (Op) fprintf(Op,"%d %d %d %d ",obuf[i+j*4+0],obuf[i+j*4+1],obuf[i+j*4+2],obuf[i+j*4+3]);
        k+=2;
    }
    if (Op) fclose(Op);

    *olen = i+(j-1)*4+4;
    return 0;
}

