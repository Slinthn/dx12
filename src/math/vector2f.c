void vec2_copy(struct vector2 *vec, struct vector2 source) {
  (*vec).x = source.x;
  (*vec).y = source.y;
}

void vec2_add(struct vector2 *vec, struct vector2 source) {
  (*vec).x += source.x;
  (*vec).y += source.y;
}

void vec2_mul(struct vector2 *vec, struct vector2 source) {
  (*vec).x *= source.x;
  (*vec).y *= source.y;
}

float vec2_magnitude(struct vector2 vec) {
  return sqrtf((vec.x * vec.x) + (vec.y * vec.y));
}

void vec2_normalise(struct vector2 *vec) {
  float magnitude = vec2_magnitude(*vec);

  if (magnitude != 0) {
    (*vec).x /= magnitude;
    (*vec).y /= magnitude;
  }
}

void vec2_identity(struct vector2 *vec) {
  (*vec).x = 0;
  (*vec).y = 0;
}