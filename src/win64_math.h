typedef float MATRIX[16];

#define PIf 3.1415926535897932384626f
#define DToR(x) ((x * PIf) / 180.0f)

void MIdentity(MATRIX *m) {
  for (unsigned int i = 0; i < 15; i++) {
    (*m)[i] = 0;
  }

  (*m)[0] = 1.0f;
  (*m)[5] = 1.0f;
  (*m)[10] = 1.0f;
  (*m)[15] = 1.0f;
}

void MPerspective(MATRIX *m, float fov, float nearz, float farz) {
  MIdentity(m);
  
  float s = 1.0f / (tanf(fov / 2.0f));

  (*m)[0] = s;
  (*m)[5] = s;
  (*m)[10] = ((farz + nearz) / (farz - nearz));
  (*m)[14] = 1.0f;
  (*m)[11] = -2 * ((farz * nearz) / (farz - nearz));
  (*m)[15] = 0.0f;
}

void MTransform(MATRIX *m, float tx, float ty, float tz, float rx, float ry, float rz) {
  MIdentity(m);
  (*m)[3] = tx;
  (*m)[7] = ty;
  (*m)[11] = tz;

  float srx = sinf(rx);
  float crx = cosf(rx);
  float sry = sinf(ry);
  float cry = cosf(ry);
  float srz = sinf(rz);
  float crz = cosf(rz);
  
  (*m)[0] = crz*cry;
  (*m)[1] = crz*sry*srx - srz*crx;
  (*m)[2] = crz*sry*crx + srz*srx;
  (*m)[4] = srz*cry;
  (*m)[5] = srz*sry*srx + crz*crx;
  (*m)[6] = srz*sry*crx - crz*srx;
  (*m)[8] = -sry;
  (*m)[9] = cry*srx;
  (*m)[10] = cry*crx;
}
