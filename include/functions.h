#ifndef HELPER_FUNCTIONS
#define HELPER_FUNCTIONS

float clamp(float f, float min, float max) {
    f = f > min ? f : min;
    f = f < max ? f : max;
    return f;
}

#endif /* HELPER_FUNCTIONS */
