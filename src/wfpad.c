#include "opendab.h"
#include "wfbyteops.h"
#include "pad.h"

void wfpad(unsigned char *buf, int bytes, int bitrate)
{
        struct f_pad p;
        struct f_pad_00 p00;
        unsigned char short_x_pad[5];
        short fpad;

        static int current_ci;

        /* if bitrate/channel is >= 56kbit/s, 4 SCF words are used, else 2 */
        int scf_words = (bitrate >= 112) ? 4 : 2;

        fpad = spack(&buf[bytes-2]);
        memcpy(&p, &fpad, 2);
        fprintf(stderr, "scf_words: %d FType: %d CIFlag: %d Z: %d\n", scf_words, p.FType, p.CIFlag, p.Z);

        if (p.FType == 0) {
                memcpy(&p00, &buf[bytes-2], 1);

                fprintf(stderr, " FType: %d XPadInd: %d ByteLInd: %d\n", p00.FType, p00.XPadInd, p00.ByteLInd);

                if (p00.XPadInd == 1) {
                        if (p.CIFlag == 1) {
                                current_ci = buf[bytes-(1 + scf_words + 2)];
                                for (int i = 0; i <= 2; i++)
                                        short_x_pad[i] = (unsigned char)buf[bytes-(i + 2 + scf_words + 2)];
                                short_x_pad[3] = '\0';
                                short_x_pad[4] = '\0';

                                fprintf(stderr, "  CI: %d\n", current_ci);
                                fprintf(stderr, "  short x-pad: %s\n", short_x_pad);
                        }
                        else {
                                for (int i = 0; i <= 3; i++)
                                        short_x_pad[i] = (unsigned char)buf[bytes-(i + 1 + scf_words + 2)];
                                short_x_pad[4] = '\0';

                                fprintf(stderr, "  short x-pad: %s\n", short_x_pad);
                        }
                }
        }
}
