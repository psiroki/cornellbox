#pragma once

#if defined(__SSE4_1__) && !defined(BASIC_VECTORS)

#include <x86intrin.h>

inline float _f(const int &i) {
  return reinterpret_cast<const float&>(i);
}

struct Vec {
    __m128 data;

    Vec(const Vec &other) : data(other.data) { }

    Vec(float v = 0): data(_mm_set1_ps(v)) { }

    Vec(float a, float b, float c = 0) : data(_mm_setr_ps(a, b, c, 0.0f)) { }

    explicit Vec(__m128 d) : data(d) { }

    inline Vec operator+(const Vec &r) const { return Vec(_mm_add_ps(data, r.data)); }

    inline Vec operator-(const Vec &r) const { return Vec(_mm_sub_ps(data, r.data)); }

    inline Vec operator*(const Vec &r) const { return Vec(_mm_mul_ps(data, r.data)); }

    inline Vec operator/(const Vec &r) const { return Vec(_mm_div_ps(data, r.data)); }

    inline float operator|(const Vec &r) const { return _mm_cvtss_f32(_mm_dp_ps(data, r.data, 0x71)); }

    inline Vec sqrt() const {
        return Vec(_mm_sqrt_ps(data));
    }

    inline void normalize() {
        __m128 f = _mm_set1_ps(1.0f / sqrtf(_mm_cvtss_f32(_mm_dp_ps(data, data, 0x71))));
        data = _mm_mul_ps(data, f);
    }

    // normalize
    inline Vec operator!() const {
      return *this * (1 / sqrtf(*this | *this));
    }
    
    float length2() const {
      return *this | *this;
    }
    
    float length() const {
      return sqrtf(length2());
    }

    inline Vec min(const Vec &r) const { return Vec(_mm_min_ps(data, r.data)); }

    inline float x() const { return _mm_cvtss_f32(data); }
    inline float y() const { return _f(_mm_extract_ps(data, 1)); }
    inline float z() const { return _f(_mm_extract_ps(data, 2)); }

    void flatten(float *f) const { _mm_storeu_ps(f, data); }
};

#elif defined(__ARM_NEON) && !defined(BASIC_VECTORS)

#pragma message "Compiling for ARM NEON"

#include <arm_neon.h>

inline float32x4_t _makeFloat32x4(float a, float b, float c, float d) {
    float f[] = { a, b, c, d };
    return vld1q_f32(f);
}

struct Vec {
    float32x4_t data;

    Vec(const Vec &other) : data(other.data) { }

    Vec(float v = 0): data(vdupq_n_f32(v)) { }

    Vec(const float *f): data(vld1q_f32(f)) { }

    Vec(float a, float b, float c = 0) : data(_makeFloat32x4(a, b, c, 0.0f)) { }

    explicit Vec(float32x4_t d) : data(d) { }

    inline Vec operator+(const Vec &r) const { return Vec(vaddq_f32(data, r.data)); }

    inline Vec operator-(const Vec &r) const { return Vec(vsubq_f32(data, r.data)); }

    inline Vec operator*(const Vec &r) const { return Vec(vmulq_f32(data, r.data)); }

    inline Vec operator*(const float &r) const { return Vec(vmulq_n_f32(data, r)); }

    inline Vec operator/(const Vec &r) const {
        float f[4];
        r.flatten(f);
        for (int i = 0; i < 4; ++i) {
          f[i] = vgetq_lane_f32(data, i) / f[i];
        }
        return Vec(f);
    }

    inline float operator|(const Vec &r) const {
        float32x4_t m = vmulq_f32(data, r.data);
        return vgetq_lane_f32(m, 0) + vgetq_lane_f32(m, 1) + vgetq_lane_f32(m, 2) + vgetq_lane_f32(m, 3);
    }

    inline Vec sqrt() const {
        return Vec(vrsqrteq_f32(data));
    }

    inline void normalize() {
        float scale = 1.0f / length();
        data = vmulq_n_f32(data, scale);
    }

    // normalize
    inline Vec operator!() const {
      return *this * (1 / sqrtf(*this | *this));
    }
    
    float length2() const {
      return *this | *this;
    }
    
    float length() const {
      return sqrtf(length2());
    }

    inline Vec min(const Vec &r) const { return Vec(vminq_f32(data, r.data)); }

    inline float x() const { return vgetq_lane_f32(data, 0); }
    inline float y() const { return vgetq_lane_f32(data, 1); }
    inline float z() const { return vgetq_lane_f32(data, 2); }

    void flatten(float *f) const {
      for (int i = 0; i < 4; ++i)
        f[i] = vgetq_lane_f32(data, i);
    }
};


#else

inline float _min(float a, float b) {
  return a < b ? a : b;
}

struct Vec {
    float data[4];

    Vec(const Vec &other) : data{other.data[0], other.data[1], other.data[2], other.data[3]} { }

    Vec(float v = 0): data{v, v, v, v} { }

    Vec(float a, float b, float c = 0.0f, float d = 0.0f) : data { a, b, c, d } { }

    explicit Vec(const float *d) : data { d[0], d[1], d[2], d[3] } { }

    inline Vec operator+(const Vec &r) const {
      return Vec(data[0] + r.data[0], data[1] + r.data[1], data[2] + r.data[2], data[3] + r.data[3]);
    }

    inline Vec operator-(const Vec &r) const {
      return Vec(data[0] - r.data[0], data[1] - r.data[1], data[2] - r.data[2], data[3] - r.data[3]);
    }

    inline Vec operator*(const Vec &r) const {
      return Vec(data[0] * r.data[0], data[1] * r.data[1], data[2] * r.data[2], data[3] * r.data[3]);
    }

    inline Vec operator/(const Vec &r) const {
      return Vec(data[0] / r.data[0], data[1] / r.data[1], data[2] / r.data[2], data[3] / r.data[3]);
    }

    inline float operator|(const Vec &r) const {
      return data[0] * r.data[0] + data[1] * r.data[1] + data[2] * r.data[2] + data[3] * r.data[3];
    }

    inline Vec sqrt() const {
        return Vec(sqrtf(data[0]), sqrtf(data[1]), sqrtf(data[2]), sqrtf(data[3]));
    }

    // normalize
    inline Vec operator!() const {
      return *this * (1 / sqrtf(*this | *this));
    }
    
    float length2() const {
      return *this | *this;
    }
    
    float length() const {
      return sqrtf(length2());
    }

    inline void normalize() {
      float scale = 1.0f / length();
      data[0] = data[0] * scale;
      data[1] = data[1] * scale;
      data[2] = data[2] * scale;
      data[3] = data[3] * scale;
    }

    inline Vec min(const Vec &r) const {
      return Vec(
        _min(data[0], r.data[0]),
        _min(data[1], r.data[1]),
        _min(data[2], r.data[2]),
        _min(data[3], r.data[3])
      );
    }

    inline float x() const { return data[0]; }
    inline float y() const { return data[1]; }
    inline float z() const { return data[2]; }

    void flatten(float *f) const {
      f[0] = data[0];
      f[1] = data[1];
      f[2] = data[2];
      f[3] = data[3];
    }
};

#endif

