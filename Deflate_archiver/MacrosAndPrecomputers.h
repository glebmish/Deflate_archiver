#pragma once

typedef unsigned int uint;

#define GETBIT(from, bit) ( (from >> (bit)) & 1)
#define SETBIT(to, bit, val)  ( (val) == 1 ? ( to |= (1 << (bit)) ) : ( to &= ~(1 << (bit)) ) )

extern int litExtraBits[];  //extra bits for literals 257..285
extern int dstExtraBits[];  //extra bits for distance 0..29
extern int litStartFrom[];  //length for each code starts from this number
extern int dstStartFrom[];  //distance for each code starts from this number
extern int hcOrder[];       //order of reading Code Length codes
