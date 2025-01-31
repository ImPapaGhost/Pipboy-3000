#include "pipboy.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <math.h>
#include "pipboy.h"
#include "state.h"



int file_exists(const char *path) {
    struct stat buffer;
    return (stat(path, &buffer) == 0);
}

// Easing subtab animation
float ease_out_cubic(float t) {
    return 1 - pow(1 - t, 3);
}
