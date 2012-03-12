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
        pad->ptr = pad->segment;
        
        return pad;
}

void wfpad(struct pad_state *pad, unsigned char *buf, int bytes)
{
        struct f_pad p;
        struct f_pad_00 p00;
        struct dls_pad dls;
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
                        /* offset in buf of the start of the x-pad data */
                        int xpadoff = bytes - (1 + scf_words + 2);
                        
                        if (p.CIFlag == 1) {
                                pad->ci = buf[xpadoff];

                                if (pad->ci == 2) {
                                        short prefix = spack(&buf[xpadoff - 1]);
                                        memcpy(&dls, &prefix, 2);
                                        
                                        //fprintf(stderr, "   DLS: f1: %d f2: %d cmd: %d first: %d toggle: %d\n", dls.f1, dls.f2, dls.cmd, dls.first, dls.toggle);
                                        
                                        if (dls.first == 2) { // first segment
                                                if (dls.toggle != pad->toggle) {
                                                        pad->toggle = dls.toggle;
                                                        fprintf(stderr, "\nnew DLS:\n");
                                                }

                                                /* reset "next byte" pointer */
                                                pad->ptr = pad->segment;
                                                
                                                /* reset label length */
                                                pad->dls_length = 0;
                                        }

                                        /* add the prefix and first character from this x-pad segment */
                                        for (int i = 1; i < 4; i++) {
                                                *pad->ptr = (unsigned char)buf[xpadoff - i];
                                                pad->ptr++;
                                        }

                                        /* store the count of remaining characters (inc crc) */
                                        pad->left = dls.f1 + 2;

                                        /* store the full length of the segment (inc prefix) */
                                        pad->seglen = pad->left + 1;

                                        /* add the text length of the segment to the label length */
                                        pad->dls_length += (dls.f1 + 1);

                                        /* store the first/last flag */
                                        pad->first = dls.first;
                                }
                        }
                        else {
                                //fprintf(stderr, "   cont: CI: %d first: %d seglen: %d left: %d dls_length: %d\n", pad->ci, pad->first, pad->seglen, pad->left, pad->dls_length);

                                if (pad->ci == 2) {
                                        for (int i = 0; i < 4; i++) {
                                                if (pad->left > 2) {
                                                        *pad->ptr = (unsigned char)buf[xpadoff - i];
                                                        pad->ptr++;
                                                        pad->left--;
                                                        if (pad->left == 2)
                                                                pad->ptr = pad->crc;
                                                }
                                                else if (pad->left > 0) {
                                                        *pad->ptr = (unsigned char)buf[xpadoff - i];
                                                        pad->ptr++;
                                                        pad->left--;
                                                }
                                        }
                                        /* if we've got the last character of any segment, do a crc check */
                                        if (pad->left == 0) {
                                                //unsigned short crc = spack(pad->crc);
                                                //fprintf(stderr, "crc should be: %x\n", crc);

                                                crc16check(pad->segment, pad->seglen);

                                                //for (int i = 0; i < pad->seglen; i++)
                                                //       fprintf(stderr, "0x%02x ", pad->segment[i]);
                                                //fprintf(stderr, "\n");

                                                pad->segment[pad->seglen] = '\0';

                                                /* assume crc ok; should check */
                                                /* copy segment's text into the label */
                                                memcpy(pad->label + (pad->dls_length - (pad->seglen - 2)), pad->segment + 2, pad->seglen - 2);
                                                
                                                /* if we've got the last character of a last DLS segment, display it */
                                                if (pad->first == 1) { // last segment
                                                        pad->label[pad->dls_length] = '\0';
                                                        fprintf(stderr, "DLS: %s\n", pad->label);
                                                }
                                        }
                                }
                        }
                }
        }
}
