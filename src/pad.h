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
