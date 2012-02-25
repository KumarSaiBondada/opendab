#include <stdio.h>
#include <string.h>

#include "wfic.h"

// interleaver.c
// G(x) = x^16 + x^12 + x^5 + 1 (ITU-T Recommendation X.25 [6]).
#define CRC_POLY    0x8408
#define CRC_GOOD    0xf0b8

int crc16(unsigned char *buf, int len, int width)
{
    unsigned short crc;
    int c15;
    int i,j;

    crc = 0xffff;
    for(i=0; i<len; i++)
    {
        for(j=0; j<width; j++)
        {
            c15 = (crc & 1) ^ ((buf[i]>>(width-1-j))&1);
            crc = crc >> 1;
            if (c15) crc ^= CRC_POLY;
        }
    }
    //printf("crc=%x\n",crc);

    return (crc == CRC_GOOD) ? 0 : 1;
}




// endian = 0 - little endian
// endian = 1 - big endian
int byte_to_bit(char *logfile, int endian, unsigned char *ibuf, int ilen, unsigned char *obuf, int *olen)
{
    FILE *Op = 0;
    int i,j;

    if (logfile) Op = fopen(logfile,"wt");
    for (i=0; i<ilen; i++)
    {
        for (j=0; j<8; j++)
        {
            if (endian == 0)
            {
                obuf[(i*8) + j] = (ibuf[i]>>j) & 1;             //le
            }
            else
            {
                obuf[(i*8) + j] = (ibuf[i]>>(7-j)) & 1;         //be
            }

            if (Op) fprintf(Op,"%d ",obuf[(i*8) + j]);
        }
    }
    if (Op) fprintf(Op,"\n");
    if (Op) fclose(Op);

    *olen = ilen * 8;
    return 0;
}

// endian = 0 - little endian
// endian = 1 - big endian
int bit_to_byte(char *logfile, int endian, unsigned char *ibuf, int ilen, unsigned char *obuf, int *olen)
{
    FILE *Op = 0;
    int i,j;

    if (logfile) Op = fopen(logfile,"wt");
    j = 0;
    for (i=0; i<ilen; i+=8)
    {
            if (endian == 0)
            {
                obuf[j] = (ibuf[i+0]<<0) + (ibuf[i+1]<<1) + (ibuf[i+2]<<2) + (ibuf[i+3]<<3) +       //le
                    (ibuf[i+4]<<4) + (ibuf[i+5]<<5) + (ibuf[i+6]<<6) + (ibuf[i+7]<<7);
            }
            else
            {
                obuf[j] = (ibuf[i+0]<<7) + (ibuf[i+1]<<6) + (ibuf[i+2]<<5) + (ibuf[i+3]<<4) +       //be
                    (ibuf[i+4]<<3) + (ibuf[i+5]<<2) + (ibuf[i+6]<<1) + (ibuf[i+7]<<0);
            }
        if (Op) fprintf(Op,"%02x ",obuf[j]);
        j++;
    }
    if (Op) fprintf(Op,"\n");
    if (Op) fclose(Op);

    *olen = j;
    return 0;
}





int bitswap16(unsigned char *a, unsigned char *b, int len)
{
    int i;

    for(i=0; i<len; i+=16)
    {
        b[i+0] = a[i+15];
        b[i+1] = a[i+14];
        b[i+2] = a[i+13];
        b[i+3] = a[i+12];
        b[i+4] = a[i+11];
        b[i+5] = a[i+10];
        b[i+6] = a[i+9];
        b[i+7] = a[i+8];

        b[i+8] = a[i+7];
        b[i+9] = a[i+6];
        b[i+10] = a[i+5];
        b[i+11] = a[i+4];
        b[i+12] = a[i+3];
        b[i+13] = a[i+2];
        b[i+14] = a[i+1];
        b[i+15] = a[i+0];
    }
    return 0;
}

int dump_buffer(char *name, char *buf, int blen)
{
    int i;

    printf("%s = ",name);
    for(i=0; i<blen; i++)
    {
        printf("%02x ",(unsigned char)buf[i]);
    }
    printf("\n");
    return 0;
}

int dump_ascii(char *name, unsigned char *buf, int blen)
{
    int i;

    printf("%s = ",name);
    for (i=0; i<blen; i++)
    {
        printf("%c",((buf[i]>0x20)&&(buf[i]<0x80)) ? buf[i] : ' ');
    }
    printf("\n");
    return 0;
}

// .. keep off the stack

// raw data
int dlen_a;
unsigned char data2a[384];
unsigned char data3a[384];
unsigned char data4a[384];

int dlen_b;
unsigned char data2b[3072];
unsigned char data3b[3072];
unsigned char data4b[3072];
int dlen_c;
unsigned char data2c[3072];
unsigned char data3c[3072];
unsigned char data4c[3072];
int dlen_d;
unsigned char data2d[3072];
unsigned char data3d[3072];
unsigned char data4d[3072];
int dlen_e;
unsigned char data2e[3072];
unsigned char data3e[3072];
unsigned char data4e[3072];

unsigned char data9216[9216];

int flen_a;
unsigned char fic_a[4][2304];
int flen_b;
unsigned char fic_b[4][3096];
int flen_c;
unsigned char fic_c[4][768];
int flen_d;
unsigned char fic_d[4][768];

int fblen_a;
unsigned char fib_a[12][256];
int fblen_b;
unsigned char fib_b[12][32];

int fic_decode(unsigned char* sym2, unsigned char* sym3, unsigned char* sym4, unsigned char* obuf)
{
  int i;
  int err;
  unsigned char* optr = obuf;
  int okfibs = 0;

  init_viterbi();
  init_table49();

  //        dump_buffer("data2", data2a, dlen_a);
  //        dump_buffer("data3", data3a, dlen_a);
  //        dump_buffer("data4", data4a, dlen_a);


  //
  // the data from the wavefinder is 16 bit big endian
  // i.e d15 = the first bit
  // we want this in a bit vector
  // so do some mungeing

  // byte_to_bit, 384->3702

  dlen_a = 384;

  byte_to_bit(NULL, 0, sym2, dlen_a, data2b, &dlen_b);
  byte_to_bit(NULL, 0, sym3, dlen_a, data3b, &dlen_b);
  byte_to_bit(NULL, 0, sym4, dlen_a, data4b, &dlen_b);
  //        dump_buffer("bit2", data2b, 0x50);
  //        dump_buffer("bit3", data3b, 0x50);
  //        dump_buffer("bit4", data4b, 0x50);

  // bitswap16, 3072
  bitswap16(data2b, data2c, dlen_b);
  bitswap16(data3b, data3c, dlen_b);
  bitswap16(data4b, data4c, dlen_b);
  dlen_c = dlen_b;
  //        dump_buffer("bit2", data2c, 0x50);
  //        dump_buffer("bit3", data3c, 0x50);
  //        dump_buffer("bit4", data4c, 0x50);


  //
  // the data is ordered as qpsk symbols; i.e 1536x(real,complex) pairs
  // the deinterelaver operates on these pairs

  // deinterleave, 3072
  frequency_deinterleaver(data2c, data2d);
  frequency_deinterleaver(data3c, data3d);
  frequency_deinterleaver(data4c, data4d);
  dlen_d = dlen_c;
  //        dump_buffer("frequency_deinterleaver", data2d, 0x50);
  //        dump_buffer("frequency_deinterleaver", data3d, 0x50);
  //        dump_buffer("frequency_deinterleaver", data4d, 0x50);


  //
  // the demapper simply separates the (real,complex) pairs to give
  // 1536xreals, 1536xcomplex

  // demapper, 3072
  qpsk_symbol_demapper(data2d, data2e);
  qpsk_symbol_demapper(data3d, data3e);
  qpsk_symbol_demapper(data4d, data4e);
  dlen_e = dlen_d;
  //        dump_buffer("frequency_demapper", data2e, 0x50);
  //        dump_buffer("frequency_demapper", data3e, 0x50);
  //        dump_buffer("frequency_demapper", data4e, 0x50);




  // 3x3072 bit symbols are used for 4 FICS
  //
  // 3 x 3072 = 9216      - symbols: 2,3,4
  // 4 x 2304 = 9216      - 4 x FICs
  // then depuncture
  //      2304 -> 3096
  // then viterbi
  //      3096 -> 768
  // 3 x 256 = 768        - 3 FIBs per FIC
  // descramble


  // now merge into 1x9216 bit vectors
  for (i=0; i<3072; i++)
    {
      data9216[     i] = data2e[i];
      data9216[3072+i] = data3e[i];
      data9216[6144+i] = data4e[i];
    }
  // now split into 4 2304 bit vectors
  for (i=0; i<2304; i++)
    {
      fic_a[0][i] = data9216[     i];
      fic_a[1][i] = data9216[2304+i];
      fic_a[2][i] = data9216[4608+i];
      fic_a[3][i] = data9216[6912+i];
    }
  flen_a = 2304;

  // depuncture, 2304->3096
  depuncture(NULL, fic_a[0], flen_a, fic_b[0], &flen_b);
  depuncture(NULL, fic_a[1], flen_a, fic_b[1], &flen_b);
  depuncture(NULL, fic_a[2], flen_a, fic_b[2], &flen_b);
  depuncture(NULL, fic_a[3], flen_a, fic_b[3], &flen_b);
  //        dump_buffer("depuncture", fic_b[0], 0x50);
  //        dump_buffer("depuncture", fic_b[1], 0x50);
  //        dump_buffer("depuncture", fic_b[2], 0x50);
  //        dump_buffer("depuncture", fic_b[3], 0x50);



  // viterbi, 3096->768
  viterbi(NULL, fic_b[0], flen_b, fic_c[0], &flen_c);
  viterbi(NULL, fic_b[1], flen_b, fic_c[1], &flen_c);
  viterbi(NULL, fic_b[2], flen_b, fic_c[2], &flen_c);
  viterbi(NULL, fic_b[3], flen_b, fic_c[3], &flen_c);
  //        dump_buffer("viterbi", fic_c[0], 0x50);
  //        dump_buffer("viterbi", fic_c[1], 0x50);
  //        dump_buffer("viterbi", fic_c[2], 0x50);
  //        dump_buffer("viterbi", fic_c[3], 0x50);



  // scramble, 768
  scramble(NULL, fic_c[0], fic_d[0], flen_c);
  scramble(NULL, fic_c[1], fic_d[1], flen_c);
  scramble(NULL, fic_c[2], fic_d[2], flen_c);
  scramble(NULL, fic_c[3], fic_d[3], flen_c);
  flen_d = flen_c;
  //        dump_buffer("scrambler", &fic_d[0][0], 0x50);
  //        dump_buffer("scrambler", fic_d[1], 0x50);
  //        dump_buffer("scrambler", fic_d[2], 0x50);
  //        dump_buffer("scrambler", fic_d[3], 0x50);


  // now split into FIBs, 3x256 bit vectors per FIC
  for (i=0; i<256; i++)
    {
      fib_a[0][i] = fic_d[0][    i];
      fib_a[1][i] = fic_d[0][256+i];
      fib_a[2][i] = fic_d[0][512+i];

      fib_a[3][i] = fic_d[1][    i];
      fib_a[4][i] = fic_d[1][256+i];
      fib_a[5][i] = fic_d[1][512+i];

      fib_a[6][i] = fic_d[2][    i];
      fib_a[7][i] = fic_d[2][256+i];
      fib_a[8][i] = fic_d[2][512+i];

      fib_a[9][i] = fic_d[3][    i];
      fib_a[10][i] = fic_d[3][256+i];
      fib_a[11][i] = fic_d[3][512+i];
    }
  fblen_a = 256;

  // scan through all 12 FIBs
  for(i=0; i<12; i++)
    {
      //
      // check the CRC for each FIB, dump any bad ones
      //
      err = crc16(fib_a[i], fblen_a, 1);
      //printf("crc %s",(err)?"error\n":"good, ");

      // bit_to_byte, big_endian, 256->32
      bit_to_byte(NULL, 1, fib_a[i], fblen_a, fib_b[i], &fblen_b);
      //            dump_buffer("fib_b", &fib_b[i][0], fblen_b);

      // ignore any FIBs with CRC errors
      if (!err)
	{
	  memcpy(optr, fib_b[i], 30*sizeof(unsigned char));
	  optr += 30*sizeof(unsigned char);
	  okfibs++;
	  //dump_ascii("fib_b", fib_b[i], fblen_b);

	  // write out the fibs - strip off the crcs
	  //fwrite(fib_b[i], 1, 30, Op);
	}
    }

  /* Return the number of good (crc correct) fibs stored */
  return okfibs;
}
