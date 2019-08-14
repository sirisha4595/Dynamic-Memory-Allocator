#include <stdio.h>
#include <errno.h>
#include "sfmm.h"
#include "debug.h"
#define MIN_BLOCK_SIZE (32)
int main(int argc, char const *argv[]) {
    sf_mem_init();
    double* ptr = sf_malloc(sizeof(double));

    *ptr = 320320320e-320;

    printf("%e\n", *ptr);

    sf_free(ptr);
    sf_mem_fini();

    return EXIT_SUCCESS;

}

