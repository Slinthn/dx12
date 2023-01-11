void vec3_copy(
  struct vector3 *vec,
  struct vector3 source)
{
  (*vec).x = source.x;
  (*vec).y = source.y;
  (*vec).z = source.z;
}

void vec3_add(
  struct vector3 *vec,
  struct vector3 source)
{
  (*vec).x += source.x;
  (*vec).y += source.y;
  (*vec).z += source.z;
}

void vec3_mul(
  struct vector3 *vec,
  struct vector3 source)
{
  (*vec).x *= source.x;
  (*vec).y *= source.y;
  (*vec).z *= source.z;
}

float vec3_magnitude(
  struct vector3 vec)
{
  return sqrtf((vec.x * vec.x) + (vec.y * vec.y) + (vec.z * vec.z));
}

void vec3_normalise(
  struct vector3 *vec)
{
  float magnitude = vec3_magnitude(*vec);

  if (magnitude != 0) {
    (*vec).x /= magnitude;
    (*vec).y /= magnitude;
    (*vec).z /= magnitude;
  }
}

void vec3_identity(
  struct vector3 *vec)
{
  (*vec).x = 0;
  (*vec).y = 0;
  (*vec).z = 0;
}

struct vector3 vec3_fromarray(
  float vec[3])
{
  struct vector3 ret;
  ret.x = vec[0];
  ret.y = vec[1];
  ret.z = vec[2];

  return ret;
}