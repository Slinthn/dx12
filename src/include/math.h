typedef float mat4[16];

#define PI32 3.1415926535897932384626f
#define DEGREESTORADIANS(x) ((x * PI32) / 180.0f)

struct vector2 {
  float x;
  float y;
};

struct vector3 {
  float x;
  float y;
  float z;
};

struct vector4 {
  float x;
  float y;
  float z;
  float w;
};

struct transformation {
  struct vector3 position;
  struct vector3 rotation;
  struct vector3 scale;
  uint8_t unused0[4];
};
