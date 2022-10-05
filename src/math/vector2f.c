void vec2_copy(vec2 *vec, vec2 source) {
  (*vec)[0] = source[0];
  (*vec)[1] = source[1];
}

void VECAdd2f(vec2 *vec, vec2 source) {
  (*vec)[0] += source[0];
  (*vec)[1] += source[1];
}

void vec2_mul(vec2 *vec, vec2 source) {
  (*vec)[0] *= source[0];
  (*vec)[1] *= source[1];
}

float VECMagnitude2f(vec2 vec) {
  return sqrtf((vec[0] * vec[0]) + (vec[1] * vec[1]));
}

void vec2_normalise(vec2 *vec) {
  float magnitude = VECMagnitude2f(*vec);

  if (magnitude != 0) {
    (*vec)[0] /= magnitude;
    (*vec)[1] /= magnitude;
  }
}

void vec2_identity(vec2 *vec) {
  (*vec)[0] = 0;
  (*vec)[1] = 0;
}