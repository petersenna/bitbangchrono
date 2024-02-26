/*

SPDX-License-Identifier: GPL-2.0-only

    Based on: https://github.com/legege/libftdi/blob/master/examples/bitbang.c

    These are the mappings for my FT232RL board (red board with 3.3V/5V jumper):
        1 -> TX
        2 -> RX
        3 -> RTS
        4 -> CTS
        5 -> DTR
        6 -> RSD
        7 -> DCD
        8 -> RI
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <getopt.h>

#include <ftdi.h>

#define USB_VENDOR_ID 0x0403
#define USB_PRODUCT_ID 0x6001

typedef struct {
    struct ftdi_context *ftdi;
    bool verbose;
    unsigned char blink_bit;
} app_context_t;

app_context_t *app_context = NULL;

// Function declarations
void signal_handler(int signum);
void display_help(void);
void parse_arguments(int argc, char **argv);
int initialize_ftdi(int vendor, int product);
void set_bitbang_mode();
void write_data(unsigned char data);
int get_user_input();
void blink_bit();
void activate_bit(unsigned char bit);
void toggle_bits();
void cleanup();

int main(int argc, char **argv) {
    int retval;

    app_context_t local_app_context;
    local_app_context.ftdi = NULL;
    local_app_context.verbose = false;
    local_app_context.blink_bit = 0;

    // Set the global pointer for signal handling
    app_context = &local_app_context;

    parse_arguments(argc, argv);

    signal(SIGINT, signal_handler);

    retval = initialize_ftdi(USB_VENDOR_ID, USB_PRODUCT_ID);
    if (retval != 0){
        cleanup();
        return EXIT_FAILURE;
    }

    set_bitbang_mode();
    write_data(0x00);

    if (app_context->blink_bit != 0)
       blink_bit();
    else
        toggle_bits();

    cleanup();

    return EXIT_SUCCESS;
}

void signal_handler(int signum) {
    if ((app_context != NULL) && (app_context->verbose))
        printf("\nCaught signal %d, cleaning up...\n", signum);

    cleanup();

    exit(signum);
}

void display_help(void) {
    printf("\nValid options are:\n");
    printf("  -v, --verbose\tEnable verbose output\n");
    printf("  -h, --help\tDisplay this help and exit\n");
    printf("  -b, --blink\tPass a number between 1 and 8 to blink that address.\n");
    printf("\t\t1 -> 0x01, 2 -> 0x02, 3 -> 0x04, 4 -> 0x08\n");
    printf("\t\t5 -> 0x10, 6 -> 0x20, 7 -> 0x40, 8 -> 0x80\n");
    printf("\n");
}

void parse_arguments(int argc, char **argv) {

    if (app_context == NULL) {
        fprintf(stderr, "app_context is NULL\n");
        exit(EXIT_FAILURE);
    }

    static struct option long_options[] = {
        {"verbose", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {"blink", required_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int option_index = 0;
    int c;

    while ((c = getopt_long(argc, argv, "vhb:", long_options, &option_index)) != -1) {
        switch (c) {
            case 'v':
                app_context->verbose = true;
                break;
            case 'h':
                display_help();
                exit(EXIT_SUCCESS);
            case 'b':
                unsigned char bit;
                if ((sscanf(optarg, "%d", &bit) != 1) ||
                    (bit < 1) ||
                    (bit > 8)) {
                    fprintf(stderr, "Invalid input...\n");
                    exit(EXIT_FAILURE);
                }
                app_context->blink_bit = bit;
                break;
            default:
                display_help();
                exit(EXIT_FAILURE);
        }
    }
}

int initialize_ftdi(int vendor, int product) {
    int f;

    if (app_context == NULL) {
        fprintf(stderr, "app_context is NULL\n");
        exit(EXIT_FAILURE);
    }
    
    if ((app_context->ftdi = ftdi_new()) == 0) {
        if (app_context->verbose)
            fprintf(stderr, "ftdi_new failed\n");
        return 1;
    }

    f = ftdi_usb_open(app_context->ftdi, vendor, product);

    if (f < 0 && f != -5) {
        if (app_context->verbose)
            fprintf(stderr, "unable to open ftdi device: %d (%s)\n", f, ftdi_get_error_string(app_context->ftdi));
        return 1;
    }

    if (app_context->verbose)
        printf("ftdi open succeeded: %d\n", f);

    return 0;
}

void set_bitbang_mode() {

    if (app_context == NULL) {
        fprintf(stderr, "app_context is NULL\n");
        exit(EXIT_FAILURE);
    }

    if (app_context->verbose)
        printf("enabling bitbang mode\n");

    ftdi_set_bitmode(app_context->ftdi, 0xFF, BITMODE_BITBANG);
}

void write_data(unsigned char data) {
    int f;

    if (app_context == NULL) {
        fprintf(stderr, "app_context is NULL\n");
        exit(EXIT_FAILURE);
    }

    f = ftdi_write_data(app_context->ftdi, &data, 1);

    if (f < 0) {
        fprintf(stderr, "write failed for 0x%x, error %d (%s)\n", data, f, ftdi_get_error_string(app_context->ftdi));
    } else {
        if (app_context->verbose)
            printf("0x%02x written successfully\n", data);
    }
}

int get_user_input() {
    int bit;
    printf("Enter a bit (1-8) or 0 to exit: ");
    scanf("%d", &bit);
    return bit;
}

int hex_to_8bit(const unsigned char hex, char *eightbits) {

    if (eightbits == NULL) {
        if (app_context->verbose)
            fprintf(stderr, "binary is NULL\n");
        return 1;
    }

    for (int i = 0; i < 8; i++) {
        eightbits[i] = (hex & (1 << (7 - i))) ? '1' : '0';
    }

    return 0;
}

void blink_bit() {
    int bit = app_context->blink_bit;

    while(true) {
        unsigned char hex = 1 << (bit - 1);

        while (true) {
            write_data(hex);
            sleep(1);
            write_data(0x00);
            sleep(1);
        }
    }
}

void toggle_bits() {
    unsigned char buf, idx;
    char eightbits[9] = {0,0,0,0,0,0,0,0,'\0'};

    if (app_context == NULL) {
        fprintf(stderr, "app_context is NULL\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < 32; i++) {
        buf = 1 << (i % 8);

        if (app_context->verbose) {
            if (i > 0 && (i % 8) == 0)
                printf("\n");

            if((hex_to_8bit(buf, eightbits)) == 0)
                idx = (i%8) + 1;
                fprintf(stdout, "%d: 0b%s ", idx, eightbits);
        }

        fflush(stdout);
        write_data(buf);
        sleep(1);
    }

    if (app_context->verbose)
        printf("\n");
}

void cleanup() {
    if (app_context == NULL)
        return;

    ftdi_disable_bitbang(app_context->ftdi);
    ftdi_usb_close(app_context->ftdi);
    ftdi_free(app_context->ftdi);

    app_context->ftdi = NULL;

    fprintf(stderr, "\n");
}