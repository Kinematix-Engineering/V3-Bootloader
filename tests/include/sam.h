#pragma once
#include <stdint.h>
#define FLASH_SIZE 0x00100000u
typedef struct { struct { unsigned TRNG_ : 1; } bit; } TestMask;
typedef struct { TestMask APBCMASK; } TestMclk;
typedef struct { struct { unsigned ENABLE : 1; } bit; } TestControl;
typedef struct { struct { unsigned DATARDY : 1; } bit; } TestFlag;
typedef struct { uint32_t reg; } TestData;
typedef struct { TestControl CTRLA; TestFlag INTFLAG; TestData DATA; } TestTrng;
extern TestMclk test_mclk;
extern TestTrng test_trng;
#define MCLK (&test_mclk)
#define TRNG (&test_trng)
