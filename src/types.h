#define AlignUp(x, alignment) (((x) + ((alignment) - 1)) & ~((alignment) - 1))

#define SizeofArray(x) (sizeof(x) / sizeof((x)[0]))

#define FATAL_ERROR(message) { MessageBoxA(0, message, "FATAL ERROR", MB_ICONERROR); ExitProcess(0); }

typedef signed char S8;
typedef unsigned char U8;
typedef signed short S16;
typedef unsigned short U16;
typedef signed int S32;
typedef unsigned int U32;
typedef signed long long S64;
typedef unsigned long long U64;