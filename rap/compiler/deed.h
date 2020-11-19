#ifndef DEED_H
#define DEED_H
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <pcre.h>

typedef unsigned char byte;
typedef unsigned char uchar;
typedef unsigned short uint2;
typedef unsigned int uint4;
typedef unsigned long uint8;
typedef short int2;
typedef int int4;
typedef long int8;


#define log_d(...) printf("%s:%-4d D ",__FILE__,__LINE__);printf(__VA_ARGS__);printf("\n");
#define log_i(...) printf("%s:%-4d I ",__FILE__,__LINE__);printf(__VA_ARGS__);printf("\n");
#define log_w(...) printf("%s:%-4d W ",__FILE__,__LINE__);printf(__VA_ARGS__);printf("\n");
#define log_e(...) printf("\033[31m%s:%-4d E ",__FILE__,__LINE__);printf(__VA_ARGS__);printf("\033[0m\n");

#endif
