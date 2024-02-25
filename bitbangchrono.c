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

typedef struct {
    struct ftdi_context *ftdi;
    bool verbose;
} app_context_t;

app_context_t *app_context = NULL;

// Function declarations
void signal_handler(int signum);
void display_help(void);
void parse_arguments(int argc, char **argv);
int initialize_ftdi(int vendor, int product);
void set_bitbang_mode();
void write_data(unsigned char data);
void toggle_bits();
void cleanup();

int main(int argc, char **argv) {
    int retval;

    app_context_t local_app_context;
    local_app_context.ftdi = NULL;
    local_app_context.verbose = false;

    // Set the global pointer for signal handling
    app_context = &local_app_context;

    signal(SIGINT, signal_handler);

    parse_arguments(argc, argv);

    retval = initialize_ftdi(USB_VENDOR_ID, USB_PRODUCT_ID);
    if (retval != 0){
        cleanup();
        return EXIT_FAILURE;
    }

    set_bitbang_mode();
    write_data(0x00);

    toggle_bits();

    cleanup();

    return EXIT_SUCCESS;
}

void signal_handler(int signum) {
   printf("\nCaught signal %d, cleaning up...\n", signum);

   cleanup();

   exit(signum);
}

void display_help(void) {
    printf("\nValid options are:\n");
    printf("  -v, --verbose\tEnable verbose output\n");
    printf("  -h, --help\tDisplay this help and exit\n");
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
        {0, 0, 0, 0}
    };

    int option_index = 0;
    int c;

    while ((c = getopt_long(argc, argv, "vh", long_options, &option_index)) != -1) {
        switch (c) {
            case 'v':
                app_context->verbose = true;
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
        cleanup();
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

void toggle_bits() {
    unsigned char buf;
    char *eightbits;

    if (app_context == NULL) {
        fprintf(stderr, "app_context is NULL\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < 32; i++) {
        buf = 1 << (i % 8);

        if (app_context->verbose) {
            if (i > 0 && (i % 8) == 0)
                printf("\n");

            eightbits = hex_to_8bit(buf);
            printf("0b%s ", eightbits);
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

    if (app_context->verbose)
        printf("disabling bitbang mode\n");

    ftdi_disable_bitbang(app_context->ftdi);
    ftdi_usb_close(app_context->ftdi);
    ftdi_free(app_context->ftdi);

    app_context->ftdi = NULL;
}