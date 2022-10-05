#define ALIGN_UP(x, alignment) (((x) + ((alignment) - 1)) & ~((alignment) - 1))

#define SIZE_OF_ARRAY(x) (sizeof(x) / sizeof((x)[0]))

#define FATAL_ERROR(message) { MessageBoxA(0, message, "FATAL ERROR", MB_ICONERROR); ExitProcess(0); }

typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;
typedef signed long long s64;
typedef unsigned long long u64;

#include "math.h"
#include "dx12.h"
#include "rawinput.h"
#include "app.h"
#include "resources.h"