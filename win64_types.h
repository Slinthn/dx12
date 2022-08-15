// APP

//#define AlignUp(x, alignment) ((x) - ((x) % (alignment)) + (alignment))
#define AlignUp(x, alignment) (((x) + ((alignment) - 1)) & ~((alignment) - 1))

#define SizeofArray(x) (sizeof(x) / sizeof((x)[0]))

typedef signed char SBYTE;
typedef unsigned char UBYTE;
typedef signed short SWORD;
typedef unsigned short UWORD;
typedef signed int SDWORD;
typedef unsigned int UDWORD;
typedef signed long long SQWORD;
typedef unsigned long long UQWORD;