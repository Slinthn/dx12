typedef float mat4[16];
typedef float vec2[2];
typedef float vec3[3];
typedef float vec4[4];

#define PI32 3.1415926535897932384626f
#define DEGREESTORADIANS(x) ((x * PI32) / 180.0f)

typedef struct t_transformation {
  vec3 position;
  vec3 rotation;
  vec3 scale;
  u8 unused0[4];
} transformation;
