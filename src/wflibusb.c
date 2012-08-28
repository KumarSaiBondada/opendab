#include "opendab.h"

#define DEBUG 1


static void cb_xfr(struct libusb_transfer *xfr)
{
        int i;

        fprintf(stderr, "length:%u, actual_length:%u\n", xfr->length, xfr->actual_length);

        for (i = 0; i < xfr->actual_length; i++) {
                printf("%02x", xfr->buffer[i]);
                if (i % 16)
                        printf("\n");
                else if (i % 8)
                        printf("  ");
                else
                        printf(" ");
        }

        if (libusb_submit_transfer(xfr) < 0) {
                fprintf(stderr, "error re-submitting URB\n");
                exit(EXIT_FAILURE);
        }
}

struct wavefinder *wf_open(char *devname)
{
        int rc;
        struct wavefinder *wf = NULL;
        struct libusb_device_handle *devh = NULL;

        rc = libusb_init(NULL);
        if (rc < 0) {
                fprintf(stderr, "Error initializing libusb: %s\n", libusb_error_name(rc));
                return NULL;
        }

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

        wf->xfr = libusb_alloc_transfer(32);
        if (!wf->xfr)
                return NULL;

        libusb_fill_iso_transfer(wf->xfr, wf->devh, WAVEFINDER_ISOPIPE, wf->buf,
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

int wf_read(struct wavefinder *wf, unsigned char *rdbuf, unsigned int* len)
{
        int rc;

        rc = libusb_submit_transfer(wf->xfr);
        if (rc != LIBUSB_SUCCESS) {
                fprintf(stderr, "libusb_submit_transfer: %s\n", libusb_error_name(rc));
                exit(EXIT_FAILURE);
        }

        rc = libusb_handle_events(NULL);
        if (rc != LIBUSB_SUCCESS) {
                fprintf(stderr, "libusb_handle_events: %s\n", libusb_error_name(rc));
                exit(EXIT_FAILURE);
        }

        return 0;
}

int wf_usb_ctrl_msg(struct wavefinder *wf, int request,
                    int value, int index, unsigned char *bytes, int size)
{
        int rc;

        rc = libusb_control_transfer(wf->devh,
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

        return 0;
}

