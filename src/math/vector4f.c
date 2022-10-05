void vec4_copy(vec4 *vec, vec4 source) {
  (*vec)[0] = source[0];
  (*vec)[1] = source[1];
  (*vec)[2] = source[2];
  (*vec)[3] = source[3];
}

void VECAdd4f(vec4 *vec, vec4 source) {
  (*vec)[0] += source[0];
  (*vec)[1] += source[1];
  (*vec)[2] += source[2];
  (*vec)[3] += source[3];
}

void VECMul4f(vec4 *vec, vec4 source) {
  (*vec)[0] *= source[0];
  (*vec)[1] *= source[1];
  (*vec)[2] *= source[2];
  (*vec)[3] *= source[3];
}

float VECMagnitude4f(vec4 vec) {
  return sqrtf((vec[0] * vec[0]) + (vec[1] * vec[1]) + (vec[2] * vec[2]) + (vec[3] * vec[3]));
}

void VECNormalise4f(vec4 *vec) {
  float magnitude = VECMagnitude4f(*vec);

  if (magnitude != 0) {
    (*vec)[0] /= magnitude;
    (*vec)[1] /= magnitude;
    (*vec)[2] /= magnitude;
    (*vec)[3] /= magnitude;
  }
}

void VECIdentity4f(vec4 *vec) {
  (*vec)[0] = 0;
  (*vec)[1] = 0;
  (*vec)[2] = 0;
  (*vec)[3] = 0;
}