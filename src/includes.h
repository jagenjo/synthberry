//global includes

#include <iostream>
#include <cmath>
#include <cstring>
#include <iostream>
#include <cstdlib>

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <math.h>

typedef int8_t Int8;
typedef int16_t Int16;
typedef int32_t Int32;
typedef uint8_t Uint8;
typedef uint16_t Uint16;
typedef uint32_t Uint32;

static const float PI = 3.1415926535f;
#define DEG2RAD 0.0174532925

inline float clamp(float x, float a, float b) { return x < a ? a : (x > b ? b : x); }
inline int clamp(int x, int a, int b) { return x < a ? a : (x > b ? b : x); }
