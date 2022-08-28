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

void MOrthographic(MATRIX *m, float left, float right, float top, float bottom, float nearz, float farz) {
  MIdentity(m);

  (*m)[0] = 2 / (right - left);
  (*m)[3] = -(right + left) / (right - left);
  (*m)[5] = 2 / (top - bottom);
  (*m)[7] = -(top + bottom) / (top - bottom);
  (*m)[10] = 1 / (farz - nearz);
  (*m)[11] = -(nearz) / (farz - nearz);
}

void MTransform(MATRIX *m, TRANSFORM transform) {
  MIdentity(m);

  // Apply transformation
  (*m)[3] = transform.position[0];
  (*m)[7] = transform.position[1];
  (*m)[11] = transform.position[2];

  // Apply rotation
  float srx = sinf(transform.rotation[0]);
  float crx = cosf(transform.rotation[0]);
  float sry = sinf(transform.rotation[1]);
  float cry = cosf(transform.rotation[1]);
  float srz = sinf(transform.rotation[2]);
  float crz = cosf(transform.rotation[2]);

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
  (*m)[0] *= transform.scale[0];
  (*m)[1] *= transform.scale[1];
  (*m)[2] *= transform.scale[2];
  (*m)[4] *= transform.scale[0];
  (*m)[5] *= transform.scale[1];
  (*m)[6] *= transform.scale[2];
  (*m)[8] *= transform.scale[0];
  (*m)[9] *= transform.scale[1];
  (*m)[10] *= transform.scale[2];
}

void MInverseTransform(MATRIX *m, TRANSFORM transform) {
  MATRIX sourcematrix;
  MTransform(&sourcematrix, transform);

  float Result[4][4];
  float tmp[12]; /* temp array for pairs */
  float src[16]; /* array of transpose source matrix */
  float det; /* determinant */
  /* transpose matrix */
  for (UINT i = 0; i < 4; i++)
  {
    src[i + 0] = (sourcematrix)[i * 4 + 0];
    src[i + 4] = (sourcematrix)[i * 4 + 1];
    src[i + 8] = (sourcematrix)[i * 4 + 2];
    src[i + 12] = (sourcematrix)[i * 4 + 3];
  }
  /* calculate pairs for first 8 elements (cofactors) */
  tmp[0] = src[10] * src[15];
  tmp[1] = src[11] * src[14];
  tmp[2] = src[9] * src[15];
  tmp[3] = src[11] * src[13];
  tmp[4] = src[9] * src[14];
  tmp[5] = src[10] * src[13];
  tmp[6] = src[8] * src[15];
  tmp[7] = src[11] * src[12];
  tmp[8] = src[8] * src[14];
  tmp[9] = src[10] * src[12];
  tmp[10] = src[8] * src[13];
  tmp[11] = src[9] * src[12];
  /* calculate first 8 elements (cofactors) */
  Result[0][0] = tmp[0] * src[5] + tmp[3] * src[6] + tmp[4] * src[7];
  Result[0][0] -= tmp[1] * src[5] + tmp[2] * src[6] + tmp[5] * src[7];
  Result[0][1] = tmp[1] * src[4] + tmp[6] * src[6] + tmp[9] * src[7];
  Result[0][1] -= tmp[0] * src[4] + tmp[7] * src[6] + tmp[8] * src[7];
  Result[0][2] = tmp[2] * src[4] + tmp[7] * src[5] + tmp[10] * src[7];
  Result[0][2] -= tmp[3] * src[4] + tmp[6] * src[5] + tmp[11] * src[7];
  Result[0][3] = tmp[5] * src[4] + tmp[8] * src[5] + tmp[11] * src[6];
  Result[0][3] -= tmp[4] * src[4] + tmp[9] * src[5] + tmp[10] * src[6];
  Result[1][0] = tmp[1] * src[1] + tmp[2] * src[2] + tmp[5] * src[3];
  Result[1][0] -= tmp[0] * src[1] + tmp[3] * src[2] + tmp[4] * src[3];
  Result[1][1] = tmp[0] * src[0] + tmp[7] * src[2] + tmp[8] * src[3];
  Result[1][1] -= tmp[1] * src[0] + tmp[6] * src[2] + tmp[9] * src[3];
  Result[1][2] = tmp[3] * src[0] + tmp[6] * src[1] + tmp[11] * src[3];
  Result[1][2] -= tmp[2] * src[0] + tmp[7] * src[1] + tmp[10] * src[3];
  Result[1][3] = tmp[4] * src[0] + tmp[9] * src[1] + tmp[10] * src[2];
  Result[1][3] -= tmp[5] * src[0] + tmp[8] * src[1] + tmp[11] * src[2];
  /* calculate pairs for second 8 elements (cofactors) */
  tmp[0] = src[2] * src[7];
  tmp[1] = src[3] * src[6];
  tmp[2] = src[1] * src[7];
  tmp[3] = src[3] * src[5];
  tmp[4] = src[1] * src[6];
  tmp[5] = src[2] * src[5];

  tmp[6] = src[0] * src[7];
  tmp[7] = src[3] * src[4];
  tmp[8] = src[0] * src[6];
  tmp[9] = src[2] * src[4];
  tmp[10] = src[0] * src[5];
  tmp[11] = src[1] * src[4];
  /* calculate second 8 elements (cofactors) */
  Result[2][0] = tmp[0] * src[13] + tmp[3] * src[14] + tmp[4] * src[15];
  Result[2][0] -= tmp[1] * src[13] + tmp[2] * src[14] + tmp[5] * src[15];
  Result[2][1] = tmp[1] * src[12] + tmp[6] * src[14] + tmp[9] * src[15];
  Result[2][1] -= tmp[0] * src[12] + tmp[7] * src[14] + tmp[8] * src[15];
  Result[2][2] = tmp[2] * src[12] + tmp[7] * src[13] + tmp[10] * src[15];
  Result[2][2] -= tmp[3] * src[12] + tmp[6] * src[13] + tmp[11] * src[15];
  Result[2][3] = tmp[5] * src[12] + tmp[8] * src[13] + tmp[11] * src[14];
  Result[2][3] -= tmp[4] * src[12] + tmp[9] * src[13] + tmp[10] * src[14];
  Result[3][0] = tmp[2] * src[10] + tmp[5] * src[11] + tmp[1] * src[9];
  Result[3][0] -= tmp[4] * src[11] + tmp[0] * src[9] + tmp[3] * src[10];
  Result[3][1] = tmp[8] * src[11] + tmp[0] * src[8] + tmp[7] * src[10];
  Result[3][1] -= tmp[6] * src[10] + tmp[9] * src[11] + tmp[1] * src[8];
  Result[3][2] = tmp[6] * src[9] + tmp[11] * src[11] + tmp[3] * src[8];
  Result[3][2] -= tmp[10] * src[11] + tmp[2] * src[8] + tmp[7] * src[9];
  Result[3][3] = tmp[10] * src[10] + tmp[4] * src[8] + tmp[9] * src[9];
  Result[3][3] -= tmp[8] * src[9] + tmp[11] * src[10] + tmp[5] * src[8];
  /* calculate determinant */
  det = src[0] * Result[0][0] + src[1] * Result[0][1] + src[2] * Result[0][2] + src[3] * Result[0][3];
  /* calculate matrix inverse */
  det = 1.0f / det;

  for (UINT i = 0; i < 4; i++)
  {
    for (UINT j = 0; j < 4; j++)
    {
      (*m)[i * 4 + j] = (Result[i][j] * det);
    }
  }
}

void MCopy(MATRIX *m, MATRIX source) {
  (*m)[0] = source[0];
  (*m)[1] = source[1];
  (*m)[2] = source[2];
  (*m)[3] = source[3];
  (*m)[4] = source[4];
  (*m)[5] = source[5];
  (*m)[6] = source[6];
  (*m)[7] = source[7];
  (*m)[8] = source[8];
  (*m)[9] = source[9];
  (*m)[10] = source[10];
  (*m)[11] = source[11];
  (*m)[12] = source[12];
  (*m)[13] = source[13];
  (*m)[14] = source[14];
  (*m)[15] = source[15];
}

void MTranspose(MATRIX *m) {
  MATRIX tmp;
  MCopy(&tmp, *m);

  (*m)[1] = tmp[4];
  (*m)[4] = tmp[1];

  (*m)[2] = tmp[8];
  (*m)[8] = tmp[2];

  (*m)[3] = tmp[12];
  (*m)[12] = tmp[3];

  (*m)[6] = tmp[9];
  (*m)[9] = tmp[6];

  (*m)[7] = tmp[13];
  (*m)[13] = tmp[7];

  (*m)[11] = tmp[14];
  (*m)[14] = tmp[11];
}