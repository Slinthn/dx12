void VECCopy3f(VECTOR3F *vec, VECTOR3F source) {
  (*vec)[0] = source[0];
  (*vec)[1] = source[1];
  (*vec)[2] = source[2];
}

void VECAdd3f(VECTOR3F *vec, VECTOR3F source) {
  (*vec)[0] += source[0];
  (*vec)[1] += source[1];
  (*vec)[2] += source[2];
}

void VECMul3f(VECTOR3F *vec, VECTOR3F source) {
  (*vec)[0] *= source[0];
  (*vec)[1] *= source[1];
  (*vec)[2] *= source[2];
}

float VECMagnitude3f(VECTOR3F vec) {
  return sqrtf((vec[0] * vec[0]) + (vec[1] * vec[1]) + (vec[2] * vec[2]));
}

void VECNormalise3f(VECTOR3F *vec) {
  float magnitude = VECMagnitude3f(*vec);

  if (magnitude != 0) {
    (*vec)[0] /= magnitude;
    (*vec)[1] /= magnitude;
    (*vec)[2] /= magnitude;
  }
}

void VECIdentity3f(VECTOR3F *vec) {
  (*vec)[0] = 0;
  (*vec)[1] = 0;
  (*vec)[2] = 0;
}