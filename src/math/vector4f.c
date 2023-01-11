void vec4_copy(struct vector4 *vec, struct vector4 source) {
  (*vec).x = source.x;
  (*vec).y = source.y;
  (*vec).z = source.z;
  (*vec).w = source.w;
}

void VECAdd4f(struct vector4 *vec, struct vector4 source) {
  (*vec).x += source.x;
  (*vec).y += source.y;
  (*vec).z += source.z;
  (*vec).w += source.w;
}

void VECMul4f(struct vector4 *vec, struct vector4 source) {
  (*vec).x *= source.x;
  (*vec).y *= source.y;
  (*vec).z *= source.z;
  (*vec).w *= source.w;
}

float vec4_magnitude(struct vector4 vec) {
  return sqrtf((vec.x * vec.x) + (vec.y * vec.y) + (vec.z * vec.z) + (vec.w * vec.w));
}

void vec4_normalise(struct vector4 *vec) {
  float magnitude = vec4_magnitude(*vec);

  if (magnitude != 0) {
    (*vec).x /= magnitude;
    (*vec).y /= magnitude;
    (*vec).z /= magnitude;
    (*vec).w /= magnitude;
  }
}

void VECIdentity4f(struct vector4 *vec) {
  (*vec).x = 0;
  (*vec).y = 0;
  (*vec).z = 0;
  (*vec).w = 0;
}