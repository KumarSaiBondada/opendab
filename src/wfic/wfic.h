#ifndef WFIC_HEADER
#define WFIC_HEADER 1
int fic_decode(unsigned char*, unsigned char*, unsigned char*, unsigned char*);
int byte_to_bit(char *, int, unsigned char*, int, unsigned char*, int*);
int bit_to_byte(char*, int, unsigned char*, int, unsigned char*, int*);
int bitswap16(unsigned char*, unsigned char*, int);
int frequency_deinterleaver(unsigned char*, unsigned char*);
int qpsk_symbol_demapper(unsigned char*, unsigned char*);
int init_viterbi();
int k_viterbi(unsigned int*,unsigned char*,unsigned char*,unsigned int,int[][256],unsigned int,unsigned int);
int scramble(char*, char*, char*, int);

extern int mettab[2][256];
#endif
