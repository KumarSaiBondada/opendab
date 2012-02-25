#ifndef WFIC_HEADER
#define WFIC_HEADER 1

int fic_decode(unsigned char* sym2, unsigned char* sym3, unsigned char* sym4, unsigned char* obuf);

int byte_to_bit(char *logfile, int endian, unsigned char *ibuf, int ilen, unsigned char *obuf, int *olen);
int bit_to_byte(char *logfile, int endian, unsigned char *ibuf, int ilen, unsigned char *obuf, int *olen);
int bitswap16(unsigned char *a, unsigned char *b, int len);
int frequency_deinterleaver(unsigned char *y, unsigned char *q);
int qpsk_symbol_demapper(unsigned char *q, unsigned char *P);

int init_viterbi();
int k_viterbi(unsigned int *metric, unsigned char *data, unsigned char *symbols, unsigned int nbits, int mettab[][256], unsigned int startstate, unsigned int endstate);
int viterbi(char *logfile, unsigned  char *ibuf, int ilen, unsigned char *obuf, int *olen);

int scramble(char *logfile, unsigned char *ibuf, unsigned char *obuf, int len);

int puncture(char *logfile, unsigned char *ibuf, int ilen, unsigned char *obuf, int *olen);
int depuncture(char *logfile, unsigned char *ibuf, int ilen, unsigned char *obuf, int *olen);

int qpsk_symbol_mapper(unsigned char *P, unsigned char *q);
int qpsk_symbol_demapper(unsigned char *q, unsigned char *P);
int init_table49();
int frequency_interleaver(unsigned char *q, unsigned char *y);
int frequency_deinterleaver(unsigned char *y, unsigned char *q);

extern int mettab[2][256];
#endif
