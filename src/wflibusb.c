#include "opendab.h"
#include <sys/time.h>

#define DEBUG 1


static void cb_ctrl_xfr(struct libusb_transfer *ctrl_xfr)
{
        if (ctrl_xfr->status != LIBUSB_TRANSFER_COMPLETED) {
                fprintf(stderr, "transfer status %d\n", ctrl_xfr->status);
                libusb_free_transfer(ctrl_xfr);
                exit(3);
        }
}

static void cb_xfr(struct libusb_transfer *xfr)
{
        int i;
        struct wavefinder *wf = xfr->user_data;
        struct timeval t;
        static unsigned char last_sym;
        unsigned char sym;

        if (xfr->status != LIBUSB_TRANSFER_COMPLETED) {
                fprintf(stderr, "transfer status %d\n", xfr->status);
                libusb_free_transfer(xfr);
                exit(3);
        }

        gettimeofday(&t, NULL);
        //fprintf(stderr, "%06ld.%06ld received data\n", (long int) t.tv_sec, (long int)  t.tv_usec);

        for (i = 0; i < xfr->num_iso_packets; i++) {
                struct libusb_iso_packet_descriptor *pack = &xfr->iso_packet_desc[i];
                unsigned char *buf = libusb_get_iso_packet_buffer_simple(xfr, i);

                //fprintf(stderr, "packet buffer length: %d actual_length: %d\n", pack->length, pack->actual_length);

                if (pack->status != LIBUSB_TRANSFER_COMPLETED) {
                        fprintf(stderr, "Error: pack %u status %d\n", i, pack->status);
                        exit(5);
                }

                /* skip duplicate symbols */
                sym = *(buf+2);

                if (!wf->sync->locked || sym != last_sym) {
                        last_sym = sym;
                        (wf->process_func)(wf, buf);
                }
        }

        xfr->user_data = wf;

        if (libusb_submit_transfer(xfr) < 0) {
                fprintf(stderr, "error re-submitting URB\n");
                exit(EXIT_FAILURE);
        }
}

struct wavefinder *wf_open(char *devname,
                           int (*process_func)(struct wavefinder *, unsigned char *))
{
        int rc;
        struct wavefinder *wf = NULL;
        struct libusb_device_handle *devh = NULL;

        rc = libusb_init(NULL);
        if (rc < 0) {
                fprintf(stderr, "Error initializing libusb: %s\n", libusb_error_name(rc));
                return NULL;
        }
        libusb_set_debug(NULL, LIBUSB_LOG_LEVEL_INFO);

        devh = libusb_open_device_with_vid_pid(NULL, VENDOR, PRODUCT);
        if (!devh) {
                fprintf(stderr, "Error finding USB device\n");
                return NULL;
        }

        rc = libusb_claim_interface(devh, 0);
        if (rc < 0) {
                fprintf(stderr, "Error claiming interface: %s\n", libusb_error_name(rc));
                return NULL;
        }

        if ((wf = malloc(sizeof (struct wavefinder))) == NULL)
                return NULL;

        wf->devh = devh;
        wf->bufptr = wf->buf;
        wf->process_func = process_func;

        wf->xfr = libusb_alloc_transfer(32);
        if (!wf->xfr)
                return NULL;

        libusb_fill_iso_transfer(wf->xfr, wf->devh, WAVEFINDER_ISOPIPE, wf->bufptr,
                                 PIPESIZE, 32, cb_xfr, NULL, 0);
        libusb_set_iso_packet_lengths(wf->xfr, 524);

        return wf;
}

int wf_close(struct wavefinder *wf)
{
        libusb_release_interface(wf->devh, 0);
        libusb_close(wf->devh);
        libusb_exit(NULL);

        return 0;
}

int wf_read(struct wavefinder *wf)
{
        int rc;


        wf->xfr->user_data = wf;

        rc = libusb_submit_transfer(wf->xfr);
        if (rc != LIBUSB_SUCCESS) {
                fprintf(stderr, "libusb_submit_transfer: %s\n", libusb_error_name(rc));
                exit(EXIT_FAILURE);
        }

        for (;;) {
                rc = libusb_handle_events(NULL);
                if (rc != LIBUSB_SUCCESS) {
                        fprintf(stderr, "libusb_handle_events: %s\n", libusb_error_name(rc));
                        exit(EXIT_FAILURE);
                }
        }

        return 0;
}

int wf_usb_ctrl_msg(struct wavefinder *wf, int request,
                    int value, int index, unsigned char *bytes, int size)
{
        if (wf->init == 0) {
                struct libusb_transfer *ctrl_xfr = libusb_alloc_transfer(0);
                if (!ctrl_xfr) {
                        fprintf(stderr, "failed to alloc transfer\n");
                        exit(EXIT_FAILURE);
                }

                unsigned char* buf = malloc((sizeof(unsigned char)) * (size + LIBUSB_CONTROL_SETUP_SIZE));
                if (!buf) {
                        fprintf(stderr, "failed to allocate control transfer buffer\n");
                        exit(EXIT_FAILURE);
                }

                libusb_fill_control_setup(buf,
                                          LIBUSB_REQUEST_TYPE_VENDOR,
                                          request,
                                          value,
                                          index,
                                          size);

                memcpy(buf + LIBUSB_CONTROL_SETUP_SIZE, bytes, size);

                libusb_fill_control_transfer(ctrl_xfr,
                                             wf->devh,
                                             buf,
                                             cb_ctrl_xfr,
                                             wf,
                                             0);

                int rc = libusb_submit_transfer(ctrl_xfr);
                if (rc != LIBUSB_SUCCESS) {
                        fprintf(stderr, "libusb_submit_transfer, ctrl, async: %s\n", libusb_error_name(rc));
                        exit(EXIT_FAILURE);
                }
        }
        else {
                int rc = libusb_control_transfer(wf->devh,
                                             LIBUSB_REQUEST_TYPE_VENDOR,
                                             request,
                                             value,
                                             index,
                                             bytes,
                                             size,
                                             0);
                if (rc < 0) {
                        fprintf(stderr, "libusb_control_transfer: %s\n", libusb_error_name(rc));
                        exit(EXIT_FAILURE);
                }
        }
        return 0;
}
