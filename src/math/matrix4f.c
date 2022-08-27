void MIdentity(MATRIX *m) {
  for (UDWORD i = 0; i < 16; i++) {
    (*m)[i] = 0;
  }

  (*m)[0] = 1.0f;
  (*m)[5] = 1.0f;
  (*m)[10] = 1.0f;
  (*m)[15] = 1.0f;
}

void MPerspective(MATRIX *m, float aspectratio, float fov, float nearz, float farz) {
  MIdentity(m);

  float s = 1.0f / (tanf(fov / 2.0f));

  (*m)[0] = s;
  (*m)[5] = s / aspectratio;
  (*m)[10] = ((-nearz - farz) / (nearz - farz));
  (*m)[11] = 2 * ((farz * nearz) / (nearz - farz));
  (*m)[14] = 1;
  (*m)[15] = 0;
}

void MTransform(MATRIX *m, float tx, float ty, float tz, float rx, float ry, float rz, float sx, float sy, float sz) {
  MIdentity(m);

  // Apply transformation
  (*m)[3] = tx;
  (*m)[7] = ty;
  (*m)[11] = tz;

  // Apply rotation
  float srx = sinf(rx);
  float crx = cosf(rx);
  float sry = sinf(ry);
  float cry = cosf(ry);
  float srz = sinf(rz);
  float crz = cosf(rz);

  (*m)[0] = (crz * cry);
  (*m)[1] = (crz * sry * srx) - (srz * crx);
  (*m)[2] = (crz * sry * crx) + (srz * srx);
  (*m)[4] = srz * cry;
  (*m)[5] = (srz * sry * srx) + (crz * crx);
  (*m)[6] = (srz * sry * crx) - (crz * srx);
  (*m)[8] = -sry;
  (*m)[9] = cry * srx;
  (*m)[10] = cry * crx;

  // Apply scaling
  (*m)[0] *= sx;
  (*m)[1] *= sy;
  (*m)[2] *= sz;
  (*m)[4] *= sx;
  (*m)[5] *= sy;
  (*m)[6] *= sz;
  (*m)[8] *= sx;
  (*m)[9] *= sy;
  (*m)[10] *= sz;
}

void MInverseTransform(MATRIX *m, float tx, float ty, float tz, float rx, float ry, float rz) {
  float srx = sinf(rx);
  float crx = cosf(rx);
  float sry = sinf(ry);
  float cry = cosf(ry);
  float srz = sinf(rz);
  float crz = cosf(rz);

  float d0 = crz * cry;
  float d1 = crz * sry * srx - srz * crx;
  float d2 = crz * sry * crx + srz * srx;
  float d4 = srz * cry;
  float d5 = srz * sry * srx + crz * crx;
  float d6 = srz * sry * crx - crz * srx;
  float d8 = -sry;
  float d9 = cry * srx;
  float d10 = cry * crx;

  float A1223 = d6 * tz - d10 * ty;
  float A0223 = d2 * tz - d10 * tx;
  float A0123 = d2 * ty - d6 * tx;
  float A1213 = d5 * tz - d9 * ty;
  float A1212 = d5 * d10 - d9 * d6;
  float A0213 = d1 * tz - d9 * tx;
  float A0212 = d1 * d10 - d9 * d2;
  float A0113 = d1 * ty - d5 * tx;
  float A0112 = d1 * d6 - d5 * d2;

  float det = d0 * (d5 * d10 - d9 * d6) - d4 * (d1 * d10 - d9 * d2) + d8 * (d1 * d6 - d5 * d2);
  det = 1 / det;

  (*m)[0] = det * (d5 * d10 - d9 * d6);
  (*m)[4] = -det * (d4 * d10 - d8 * d6);
  (*m)[8] = det * (d4 * d9 - d8 * d5);
  (*m)[12] = 0;
  (*m)[1] = -det * (d1 * d10 - d9 * d2);
  (*m)[5] = det * (d0 * d10 - d8 * d2);
  (*m)[9] = -det * (d0 * d9 - d8 * d1);
  (*m)[13] = 0;
  (*m)[2] = det * (d1 * d6 - d5 * d2);
  (*m)[6] = -det * (d0 * d6 - d4 * d2);
  (*m)[10] = det * (d0 * d5 - d4 * d1);
  (*m)[14] = 0;
  (*m)[3] = -det * (d1 * A1223 - d5 * A0223 + d9 * A0123);
  (*m)[7] = det * (d0 * A1223 - d4 * A0223 + d8 * A0123);
  (*m)[11] = -det * (d0 * A1213 - d4 * A0213 + d8 * A0113);
  (*m)[15] = det * (d0 * A1212 - d4 * A0212 + d8 * A0112);
}