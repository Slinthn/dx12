void VECCopy2f(VECTOR2F *vec, VECTOR2F source) {
  (*vec)[0] = source[0];
  (*vec)[1] = source[1];
}

void VECAdd2f(VECTOR2F *vec, VECTOR2F source) {
  (*vec)[0] += source[0];
  (*vec)[1] += source[1];
}

void VECMul2f(VECTOR2F *vec, VECTOR2F source) {
  (*vec)[0] *= source[0];
  (*vec)[1] *= source[1];
}

float VECMagnitude2f(VECTOR2F vec) {
  return sqrtf((vec[0] * vec[0]) + (vec[1] * vec[1]));
}

void VECNormalise2f(VECTOR2F *vec) {
  float magnitude = VECMagnitude2f(*vec);

  if (magnitude != 0) {
    (*vec)[0] /= magnitude;
    (*vec)[1] /= magnitude;
  }
}

void VECIdentity2f(VECTOR2F *vec) {
  (*vec)[0] = 0;
  (*vec)[1] = 0;
}