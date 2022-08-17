typedef float MATRIX[16];
typedef float VECTOR2F[2];
typedef float VECTOR3F[3];

#define PIf 3.1415926535897932384626f
#define DegreesToRadians(x) ((x * PIf) / 180.0f)

typedef struct {
  VECTOR3F position;
  VECTOR3F rotation;
  VECTOR3F scale;
} TRANSFORM;
