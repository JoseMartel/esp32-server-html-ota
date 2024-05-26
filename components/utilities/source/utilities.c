#include "utilities.h"
#include <stdio.h>

void printHex(const char *buf, size_t len) {
    for (size_t i = 0; i < len; i++) {
        printf("%02X ", (unsigned char)buf[i]);
    }
    printf("\n");
}
