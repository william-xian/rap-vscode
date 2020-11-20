/*
 * rapc.h
 *
 * Author: Liar
 */

#ifndef _RAPC_H
#define _RAPC_H
#include <deed.h>
#define def_a_(T) typedef rap_array_t a_##T; 


#define log_d(...) printf("%s:%-4d D ",__FILE__,__LINE__);printf(__VA_ARGS__);printf("\n");
#define log_i(...) printf("%s:%-4d I ",__FILE__,__LINE__);printf(__VA_ARGS__);printf("\n");
#define log_w(...) printf("%s:%-4d W ",__FILE__,__LINE__);printf(__VA_ARGS__);printf("\n");
#define log_e(...) printf("\033[31m%s:%-4d E ",__FILE__,__LINE__);printf(__VA_ARGS__);printf("\033[0m\n");
 
#endif /* _RPC_H */

