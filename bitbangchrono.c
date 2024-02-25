/* This program is distributed under the GPL, version 2 */
/* From: https://github.com/legege/libftdi/blob/master/examples/bitbang.c */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <getopt.h>

#include <ftdi.h>

#define USB_VENDOR_ID 0x0403
#define USB_PRODUCT_ID 0x6001

// Function declarations
void signal_handler(int signum);
void display_help(void);
void parse_arguments(int argc, char **argv);
int initialize_ftdi(struct ftdi_context **ftdi, int vendor, int product, bool verbose);
void set_bitbang_mode(struct ftdi_context *ftdi, bool verbose);
void write_data(struct ftdi_context *ftdi, unsigned char data, bool verbose);
void toggle_bits(struct ftdi_context *ftdi, bool verbose);
void cleanup(struct ftdi_context *ftdi, bool verbose);

struct ftdi_context *ftdi;
bool verbose = false;

int main(int argc, char **argv) {
    int retval;

    signal(SIGINT, signal_handler);

    parse_arguments(argc, argv);

    retval = initialize_ftdi(&ftdi, USB_VENDOR_ID, USB_PRODUCT_ID, verbose);
    if (retval != 0)
        return EXIT_FAILURE;

    set_bitbang_mode(ftdi, verbose);
    write_data(ftdi, 0x00, verbose); // Turn everything off

    toggle_bits(ftdi, verbose);

    cleanup(ftdi, verbose);

    return EXIT_SUCCESS;
}

void signal_handler(int signum) {
   printf("\nCaught signal %d, cleaning up...\n", signum);

   if (ftdi != NULL) {
       cleanup(ftdi, verbose);
   }
   exit(signum);
}

void display_help(void) {
    printf("\nValid options are:\n");
    printf("  -v, --verbose\tEnable verbose output\n");
    printf("  -h, --help\tDisplay this help and exit\n");
    printf("\n");
}

void parse_arguments(int argc, char **argv) {
    static struct option long_options[] = {
        {"verbose", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int option_index = 0;
    int c;

    while ((c = getopt_long(argc, argv, "vh", long_options, &option_index)) != -1) {
        switch (c) {
            case 'v':
                verbose = true;
                break;
            case 'h':
                display_help();
                exit(EXIT_SUCCESS);
            default:
                display_help();
                exit(EXIT_FAILURE);
        }
    }
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

char* hex_to_8bit(unsigned char hex) {
    char *binary = malloc(sizeof(char) * 9);

    for (int i = 0; i < 8; i++) {
        binary[i] = (hex & (1 << (7 - i))) ? '1' : '0';
    }

    binary[8] = '\0';
    return binary;

}

void toggle_bits(struct ftdi_context *ftdi, bool verbose) {
    unsigned char buf;
    char *eightbits;
    for (int i = 0; i < 32; i++) {
        buf = 1 << (i % 8);

        if (verbose) {
            if (i > 0 && (i % 8) == 0)
                printf("\n");

            eightbits = hex_to_8bit(buf);
            printf("0b%s ", eightbits);
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