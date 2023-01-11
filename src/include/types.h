#define ALIGN_UP(x, alignment) (((x) + ((alignment) - 1)) & ~((alignment) - 1))

#define SIZE_OF_ARRAY(x) (sizeof(x) / sizeof((x)[0]))

#include "math.h"
#include "dx12.h"
#include "rawinput.h"
#include "app.h"
#include "resources.h"
