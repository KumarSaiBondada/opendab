//
// see en 300 401
//
#include <stdio.h>


#define K 1536


//14.5 QPSK symbol mapper
int qpsk_symbol_mapper(unsigned char *P, unsigned char *q)
{
    int n;

    for(n=0; n<K; n++)
    {
        // complex
        q[(2*n)+0] = P[n];
        q[(2*n)+1] = P[n+K];
    }

    return 0;
}

//14.5 QPSK symbol mapper
int qpsk_symbol_demapper(unsigned char *q, unsigned char *P)
{
    int n;

    for(n=0; n<K; n++)
    {
        // complex
        P[n]   = q[(2*n)+0];
        P[n+K] = q[(2*n)+1];
    }

    return 0;
}

//Table 49: Frequency interleaving for transmission mode I
int table49[2048];

int init_table49()
{
    int i;
    int n;
    int KI[2048];

    KI[0] = 0;
    for (i=1; i<2048; i++)
    {
        KI[i] = (13 * KI[i-1] + 511) % 2048;
    }

    n = 0;
    for (i=0; i<2048; i++)
    {
        if ((KI[i]>=256) && (KI[i] <= 1792) && (KI[i]!= 1024))
        {
            table49[n] = KI[i] - 1024;
            n++;
        }
    }
//printf("init_table49()\n");
//for(i=0; i<1536; i++)
//{
//    printf("n=%d, k=%d\n",i,table49[i]);
//}
//printf("\n");
    return 0;
}

//14.6 Frequency interleaving
//14.6.1 Transmission mode I
int frequency_interleaver(unsigned char *q, unsigned char *y)
{
    int n,k;

    //... easier to just use a lookup table 49
    // ... need to move this outside the loop ...
//    init_table49();

    for(n=0; n<K; n++)
    {
        k = table49[n];     // -K/2 < k < K/2
        if (k < 0) k = K/2 + k;
        else if (k > 0) k = K/2 + k - 1;
        // now, 0 <= k  < 768

        y[(2*k)+0] = q[(2*n)+0];        // real
        y[(2*k)+1] = q[(2*n)+1];        // imag
    }

    return 0;
}

//14.6 Frequency interleaving
//14.6.1 Transmission mode I
int frequency_deinterleaver(unsigned char *y, unsigned char *q)
{
    int n,k;

    //... easier to just use a lookup table 49
    // ... need to move this outside the loop ...
//    init_table49();

    for(n=0; n<K; n++)
    {
        k = table49[n];     // -K/2 <= k <= K/2, k <>0
        if (k < 0) k = K/2 + k;
        else if (k > 0) k = K/2 + k - 1;
        // now, 0 <= k  < 768

        q[(2*n)+0] = y[(2*k)+0];      // real
        q[(2*n)+1] = y[(2*k)+1];      // imag
    }

    return 0;
}

