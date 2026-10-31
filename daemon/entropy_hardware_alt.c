#include <stddef.h>
#include <stdio.h>

/* MBEDTLS_ENTROPY_HARDWARE_ALT implementation: reads /dev/urandom */
int mbedtls_hardware_poll(void *data, unsigned char *output, size_t len, size_t *olen)
{
    (void)data;
    FILE *f = fopen("/dev/urandom", "rb");
    if (!f) return -1;
    size_t n = fread(output, 1, len, f);
    fclose(f);
    *olen = n;
    return (n == len) ? 0 : -1;
}
