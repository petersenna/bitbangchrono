/* This program is distributed under the GPL, version 2 */
/* From: https://github.com/legege/libftdi/blob/master/examples/bitbang.c */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <ftdi.h>

#define USB_VENDOR_ID 0x0403
#define USB_PRODUCT_ID 0x6001

// Function declarations
int initialize_ftdi(struct ftdi_context **ftdi, int vendor, int product, bool verbose);
void set_bitbang_mode(struct ftdi_context *ftdi, bool verbose);
void write_data(struct ftdi_context *ftdi, unsigned char data, bool verbose);
void toggle_bits(struct ftdi_context *ftdi, bool verbose);
void cleanup(struct ftdi_context *ftdi, bool verbose);

int main(int argc, char **argv) {
    struct ftdi_context *ftdi;
    int retval;

    bool verbose = false;

    retval = initialize_ftdi(&ftdi, USB_VENDOR_ID, USB_PRODUCT_ID, verbose);
    if (retval != 0)
        return EXIT_FAILURE;

    set_bitbang_mode(ftdi, verbose);
    write_data(ftdi, 0x00, verbose); // Turn everything off

    toggle_bits(ftdi, verbose);

    cleanup(ftdi, verbose);

    return EXIT_SUCCESS;
}

int initialize_ftdi(struct ftdi_context **ftdi, int vendor, int product, bool verbose) {
    int f;

    if ((*ftdi = ftdi_new()) == 0) {
        if (verbose)
            fprintf(stderr, "ftdi_new failed\n");
        return 1;
    }

    f = ftdi_usb_open(*ftdi, vendor, product);

    if (f < 0 && f != -5) {
        if (verbose)
            fprintf(stderr, "unable to open ftdi device: %d (%s)\n", f, ftdi_get_error_string(*ftdi));
        ftdi_free(*ftdi);
        return 1;
    }

    if (verbose)
        printf("ftdi open succeeded: %d\n", f);

    return 0;
}

void set_bitbang_mode(struct ftdi_context *ftdi, bool verbose) {
    if (verbose)
        printf("enabling bitbang mode\n");

    ftdi_set_bitmode(ftdi, 0xFF, BITMODE_BITBANG);
}

void write_data(struct ftdi_context *ftdi, unsigned char data, bool verbose) {
    int f = ftdi_write_data(ftdi, &data, 1);

    if (f < 0) {
        fprintf(stderr, "write failed for 0x%x, error %d (%s)\n", data, f, ftdi_get_error_string(ftdi));
    } else {
        if (verbose)
            printf("Data 0x%02x written successfully\n", data);
    }
}

void toggle_bits(struct ftdi_context *ftdi, bool verbose) {
    unsigned char buf;
    for (int i = 0; i < 32; i++) {
        buf = 1 << (i % 8);

        if (verbose) {
            if (i > 0 && (i % 8) == 0)
                printf("\n");

            printf("%02hhx ", buf);
        }

        fflush(stdout);
        write_data(ftdi, buf, verbose);
        sleep(1);
    }

    if (verbose)
        printf("\n");
}

void cleanup(struct ftdi_context *ftdi, bool verbose) {
    if (verbose)
        printf("disabling bitbang mode\n");

    ftdi_disable_bitbang(ftdi);
    ftdi_usb_close(ftdi);
    ftdi_free(ftdi);
}