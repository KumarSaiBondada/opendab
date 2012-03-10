#include "opendab.h"
#include "wfbyteops.h"

struct pad_state *init_pad(void)
{
        struct pad_state *pad;

        pad = (struct pad_state *)malloc(sizeof(struct pad_state));
        pad->ci = 0;
        pad->dls_length = 0;
        pad->bitrate = 0;
        pad->sampling_freq = 0;
        
        return pad;
}

void wfpad(struct pad_state *pad, unsigned char *buf, int bytes)
{
        struct f_pad p;
        struct f_pad_00 p00;
        struct dls_pad dls;
        unsigned char short_x_pad[5];
        short fpad;
        int scf_words;

        /* set the number of SCF CRC words for this channel */
        /* pretty much always 4 */

        if (pad->sampling_freq == 48) {
                /* for 48kHz, if bitrate/channel is >= 56kbit/s, 4 SCF words are used, else 2 */
                scf_words = (pad->bitrate >= 56) ? 4 : 2;
        }
        else {
                /* for 24kHz, always 4 SCF words */
                scf_words = 4;
        }

        fpad = spack(&buf[bytes-2]);
        memcpy(&p, &fpad, 2);
        //fprintf(stderr, "scf_words: %d FType: %d CIFlag: %d Z: %d\n", scf_words, p.FType, p.CIFlag, p.Z);

        if (p.FType == 0) {
                memcpy(&p00, &buf[bytes-2], 1);

                //fprintf(stderr, " FType: %d XPadInd: %d ByteLInd: %d\n", p00.FType, p00.XPadInd, p00.ByteLInd);

                if (p00.XPadInd == 1) {
                        if (p.CIFlag == 1) {
                                pad->ci = buf[bytes-(1 + scf_words + 2)];
                                for (int i = 0; i <= 2; i++)
                                        short_x_pad[i] = (unsigned char)buf[bytes-(i + 2 + scf_words + 2)];
                                short_x_pad[3] = '\0';
                                short_x_pad[4] = '\0';

                                if (pad->ci > 0) {
                                        //fprintf(stderr, "  CI: %d\n", pad->ci);
                                }

                                if (pad->ci == 2) {
                                        fpad = spack(&buf[bytes-(2 + scf_words + 2)]);
                                        memcpy(&dls, &fpad, 2);
                                        
                                        //fprintf(stderr, "  short x-pad: %s\n", short_x_pad);
                                        //fprintf(stderr, "   DLS: f1: %d f2: %d cmd: %d first: %d toggle: %d\n", dls.f1, dls.f2, dls.cmd, dls.first, dls.toggle);
                                        
                                        pad->dls_length = dls.f1;

                                        if (dls.first == 2) { // first segment
                                                fprintf(stderr, "\n%c", short_x_pad[2]);
                                        }
                                        if (dls.first == 0) { // intermediate segment
                                                fprintf(stderr, "%c", short_x_pad[2]);
                                        }
                                        if (dls.first == 1) { // last segment
                                                fprintf(stderr, "%c", short_x_pad[2]);
                                        }
                                }
                        }
                        else {
                                for (int i = 0; i <= 3; i++)
                                        short_x_pad[i] = (unsigned char)buf[bytes-(i + 1 + scf_words + 2)];
                                short_x_pad[4] = '\0';

                                if (pad->ci > 0) {
                                        //fprintf(stderr, "  short x-pad: %s\n", short_x_pad);
                                }
                                
                                if (pad->ci == 2) {
                                        for (int i = 0; i < 4; i++) {
                                                if (pad->dls_length-- > 0) {
                                                        fprintf(stderr, "%c", short_x_pad[i]);
                                                }
                                        }
                                }
                        }
                }
        }
}
