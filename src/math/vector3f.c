void vec3_copy(vec3 *vec, vec3 source) {
  (*vec)[0] = source[0];
  (*vec)[1] = source[1];
  (*vec)[2] = source[2];
}

void vec3_add(vec3 *vec, vec3 source) {
  (*vec)[0] += source[0];
  (*vec)[1] += source[1];
  (*vec)[2] += source[2];
}

void vec3_mul(vec3 *vec, vec3 source) {
  (*vec)[0] *= source[0];
  (*vec)[1] *= source[1];
  (*vec)[2] *= source[2];
}

float vec3_magnitude(vec3 vec) {
  return sqrtf((vec[0] * vec[0]) + (vec[1] * vec[1]) + (vec[2] * vec[2]));
}

void vec3_normalise(vec3 *vec) {
  float magnitude = vec3_magnitude(*vec);

  if (magnitude != 0) {
    (*vec)[0] /= magnitude;
    (*vec)[1] /= magnitude;
    (*vec)[2] /= magnitude;
  }
}

void vec3_identity(vec3 *vec) {
  (*vec)[0] = 0;
  (*vec)[1] = 0;
  (*vec)[2] = 0;
}