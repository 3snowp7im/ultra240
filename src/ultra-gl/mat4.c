#include <cstdlib>
#include <cstdio>

typedef GLfloat mat4[16];

static const mat4 identity = {
  1, 0, 0, 0,
  0, 1, 0, 0,
  0, 0, 1, 0,
  0, 0, 0, 1,
};

static void mat4_identity(
  mat4 r
) {
  memcpy(r, identity, sizeof(identity));
}

static void mat4_from_mat3(
  mat4 r,
  const GLfloat mat3[9]
) {
  mat4 transform = {
    mat3[0], mat3[1], mat3[2], 0,
    mat3[3], mat3[4], mat3[5], 0,
    mat3[6], mat3[7], mat3[8], 0,
    0, 0, 0, 1,
  };
  memcpy(r, transform, sizeof(transform));
}

static void mat4_translate(
  mat4 r,
  GLfloat x,
  GLfloat y,
  GLfloat z
) {
  mat4 translate = {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    x, y, z, 1,
  };
  memcpy(r, translate, sizeof(translate));
}

static void mat4_scale(
  mat4 r,
  GLfloat x,
  GLfloat y,
  GLfloat z
) {
  mat4 scale = {
    x, 0, 0, 0,
    0, y, 0, 0,
    0, 0, z, 0,
    0, 0, 0, 1,
  };
  memcpy(r, scale, sizeof(scale));
}

static void mat4_mult_mat4(
  mat4 r,
  const mat4 a,
  const mat4 b
) {
  mat4 c = {
    a[0]  *  b[0]  +  a[1]  *  b[4]  +  a[2]  *  b[8]  +  a[3]  *  b[12],
    a[0]  *  b[1]  +  a[1]  *  b[5]  +  a[2]  *  b[9]  +  a[3]  *  b[13],
    a[0]  *  b[2]  +  a[1]  *  b[6]  +  a[2]  *  b[10] +  a[3]  *  b[14],
    a[0]  *  b[3]  +  a[1]  *  b[7]  +  a[2]  *  b[11] +  a[3]  *  b[15],
    a[4]  *  b[0]  +  a[5]  *  b[4]  +  a[6]  *  b[8]  +  a[7]  *  b[12],
    a[4]  *  b[1]  +  a[5]  *  b[5]  +  a[6]  *  b[9]  +  a[7]  *  b[13],
    a[4]  *  b[2]  +  a[5]  *  b[6]  +  a[6]  *  b[10] +  a[7]  *  b[14],
    a[4]  *  b[3]  +  a[5]  *  b[7]  +  a[6]  *  b[11] +  a[7]  *  b[15],
    a[8]  *  b[0]  +  a[9]  *  b[4]  +  a[10] *  b[8]  +  a[11] *  b[12],
    a[8]  *  b[1]  +  a[9]  *  b[5]  +  a[10] *  b[9]  +  a[11] *  b[13],
    a[8]  *  b[2]  +  a[9]  *  b[6]  +  a[10] *  b[10] +  a[11] *  b[14],
    a[8]  *  b[3]  +  a[9]  *  b[7]  +  a[10] *  b[11] +  a[11] *  b[15],
    a[12] *  b[0]  +  a[13] *  b[4]  +  a[14] *  b[8]  +  a[15] *  b[12],
    a[12] *  b[1]  +  a[13] *  b[5]  +  a[14] *  b[9]  +  a[15] *  b[13],
    a[12] *  b[2]  +  a[13] *  b[6]  +  a[14] *  b[10] +  a[15] *  b[14],
    a[12] *  b[3]  +  a[13] *  b[7]  +  a[14] *  b[11] +  a[15] *  b[15],
  };
  memcpy(r, c, sizeof(c));
}

static void mat4_mult_vec4(
  mat4 r,
  const mat4 a,
  const mat4 b
) {
  GLfloat c[4] = {
    a[0]  *  b[0]  +  a[4]  *  b[1]  +  a[8]  *  b[2]  +  a[12] *  b[3],
    a[1]  *  b[0]  +  a[5]  *  b[1]  +  a[9]  *  b[2]  +  a[13] *  b[3],
    a[2]  *  b[0]  +  a[6]  *  b[1]  +  a[10] *  b[2]  +  a[14] *  b[3],
    a[3]  *  b[0]  +  a[7]  *  b[1]  +  a[11] *  b[2]  +  a[15] *  b[3],
  };
  memcpy(r, c, sizeof(c));
}

static void debug_vec4(GLfloat vec[4]) {
  printf("%f %f %f %f\n", vec[0], vec[1], vec[2], vec[3]);
}

static void debug_mat4(mat4 mat) {
  for (int i = 0; i < 4; i++) {
    debug_vec4(&mat[4 * i]);
  }
}
