#include "opendab.h"
#include "wfbyteops.h"

#define DEBUG 0

static unsigned short
crc16(unsigned char *msg, size_t size)
{
        int i = 0;
        unsigned short crc = 0xffffU;

        while(size--) {
                crc ^= (unsigned short) *msg++ << 8;

                for(i = 0; i < 8; ++i)
                        crc = crc << 1 ^ (crc & 0x8000U ? 0x1021U : 0U);
        }
        return(crc & 0xffffU);
}

struct packet {
        unsigned               : 8; // pad for alignment
        unsigned UsefulDataLen : 7;
        unsigned Command       : 1;
        unsigned Address       : 10;
        unsigned FirstLast     : 2;
        unsigned ContIndex     : 2;
        unsigned PacketLen     : 2;
};

struct data_group_header {
        unsigned RepIndex   : 4;
        unsigned ContIndex  : 4;
        unsigned DGType     : 4;
        unsigned UserAccess : 1;
        unsigned Segment    : 1;
        unsigned CRC        : 1;
        unsigned Extension  : 1;
};

struct data_group_session {
        unsigned Segment : 15;
        unsigned Last    : 1;
};

struct data_group_user {
        unsigned             : 8; // alignment
        unsigned TransportId : 16;
        unsigned Length      : 4;
        unsigned TIdFlag     : 1;
        unsigned             : 3; // Rfa
};

int wfdata(struct data_state *d, unsigned char *buf, int pktlen, FILE *dest) {
        struct packet p;
        struct data_group_header dg;
        struct data_group_session dgs;
        struct data_group_user dgu;
        int f;
        short s;
        unsigned short crc, computed;
        size_t len;

        f = ipack(buf);
	memcpy(&p, &f, sizeof(struct packet));

#if DEBUG > 0
        fprintf(stderr, "packetlen=%d datalen=%d cmd=%d addr=%d firstlast=%d contindex=%d\n",
               p.PacketLen, p.UsefulDataLen, p.Command, p.Address, p.FirstLast, p.ContIndex);
#endif

        switch (p.PacketLen) {
        case 0:
                len = 22;
                break;
        case 1:
                len = 46;
                break;
        case 2:
                len = 70;
                break;
        case 3:
                len = 94;
                break;
        }

        crc = ~spack(buf + len) & 0xffff;
        computed = crc16(buf, len);
#if DEBUG > 0
        fprintf(stderr, "crc: 0x%04x computed: 0x%04x match: %d\n", crc, computed, (crc == computed));
#endif
        if (crc != computed)
                return 1;

        if (p.Address == d->addr) {

                if (p.FirstLast == 2) {
                        s = spack(buf + 3);
                        memcpy(&dg, &s, sizeof(struct data_group_header));
                        fprintf(stderr, "repindex: %d contindex: %d dgtype: %d useraccess %d segment: %d crc: %d extension: %d\n", dg.RepIndex, dg.ContIndex, dg.DGType, dg.UserAccess, dg.Segment, dg.CRC, dg.Extension);
                        s = spack(buf + 5); // assuming no extension field
                        memcpy(&dgs, &s, sizeof(struct data_group_session));
                        fprintf(stderr, "segment: %d last: %d\n", dgs.Segment, dgs.Last);

                        f = ipack(buf + 7); // as before
                        memcpy(&dgu, &f, sizeof(struct data_group_user));
                        fprintf(stderr, "transportid flag: %d transportid: %d length: %d\n", dgu.TIdFlag, dgu.TransportId, dgu.Length);

                        d->dgtype = dg.DGType;
                }

                if (d->dgtype == 6) {
                        fwrite(buf + 3, p.UsefulDataLen, 1, dest);
                }

        }

        return 0;
}

struct data_state *
init_data(int pktaddr)
{
        struct data_state *data;
        data = (struct data_state *)malloc(sizeof(struct data_state));

        data->addr = pktaddr;
        data->dgtype = 0;

        return data;
}
