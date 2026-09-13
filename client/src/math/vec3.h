#ifndef VEC3_H
#define VEC3_H

typedef struct {
    float v[3];
} Vec3;

static inline Vec3 vec3(float x, float y, float z) {
    return (Vec3){x, y, z};
}

static inline Vec3 vec3_add(Vec3 a, Vec3 b) {
    return (Vec3){
        a.v[0] + b.v[0],
        a.v[1] + b.v[1],
        a.v[2] + b.v[2]
    };
}

static inline Vec3 vec3_sub(Vec3 a, Vec3 b) {
    return (Vec3){
        a.v[0] - b.v[0],
        a.v[1] - b.v[1],
        a.v[2] - b.v[2]
    };
}

static inline Vec3 vec3_mul(Vec3 v, float s) {
    return (Vec3){
        v.v[0] * s,
        v.v[1] * s,
        v.v[2] * s
    };
}

static inline float vec3_dot(Vec3 a, Vec3 b) {
    return a.v[0] * b.v[0] +
           a.v[1] * b.v[1] +
           a.v[2] * b.v[2];
}

static inline Vec3 vec3_cross(Vec3 a, Vec3 b) {
    return (Vec3){
        a.v[1] * b.v[2] - a.v[2] * b.v[1],
        a.v[2] * b.v[0] - a.v[0] * b.v[2],
        a.v[0] * b.v[1] - a.v[1] * b.v[0]
    };
}

static inline float vec3_length_squared(Vec3 v) {
    return vec3_dot(v, v);
}

#endif
