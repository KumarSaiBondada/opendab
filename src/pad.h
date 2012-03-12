struct f_pad {
        unsigned Z      : 1;
        unsigned CIFlag : 1;
        unsigned ByteL  : 6;
        unsigned ByteL1 : 6;
        unsigned FType  : 2;
};

struct f_pad_00 {
        unsigned ByteLInd : 4;
        unsigned XPadInd  : 2;
        unsigned FType    : 2;
};

struct dls_pad {
        unsigned rfa    : 4;
        unsigned f2     : 4;
        unsigned f1     : 4;
        unsigned cmd    : 1;
        unsigned first  : 2;
        unsigned toggle : 1;
};

struct pad_state {
        int bitrate;
        int sampling_freq;
        int dls_length;
        int seglen;
        int left;
        int ci;
        int toggle;
        int first;
        unsigned char label[128];
        unsigned char crc[2];
        unsigned char segment[19];
        unsigned char *ptr;
};

