#pragma once

#include <math.h>
#include <stdlib.h>

template<typename T>
[[nodiscard]] constexpr auto
clamp(T x, const T min_x, const T max_x) -> T
{
  x = (x < min_x) ? min_x : x;
  x = (x > max_x) ? max_x : x;
  return x;
}

template<typename T, size_t N>
struct Vec final
{
  T data[N];

  [[nodiscard]] auto operator+(const Vec& other) const -> Vec
  {
    Vec result;
    for (size_t i = 0; i < N; i++) {
      result.data[i] = data[i] + other.data[i];
    }
    return result;
  }

  [[nodiscard]] auto operator-(const Vec& other) const -> Vec
  {
    Vec result;
    for (size_t i = 0; i < N; i++) {
      result.data[i] = data[i] - other.data[i];
    }
    return result;
  }

  [[nodiscard]] auto operator*(const float k) const -> Vec
  {
    Vec result;
    for (size_t i = 0; i < N; i++) {
      result.data[i] = data[i] * k;
    }
    return result;
  }

  [[nodiscard]] auto operator[](size_t i) const -> const T& { return data[i]; }

  [[nodiscard]] auto operator[](size_t i) -> T& { return data[i]; }
};

template<size_t N>
[[nodiscard]] auto
squared_length(const Vec<float, N>& v) -> float
{
  float sum{ 0.0F };
  for (size_t i = 0; i < N; i++) {
    sum += v.data[i] * v.data[i];
  }
  return sum;
}

template<size_t N>
[[nodiscard]] auto
length(const Vec<float, N>& v) -> float
{
  return sqrtf(squared_length(v));
}

using Vec2f = Vec<float, 2>;

using Vec3f = Vec<float, 3>;

using Vec4f = Vec<float, 4>;

template<typename T, size_t Max>
class SmallArray final
{
  T elements_[Max];

  size_t size_{};

public:
  [[nodiscard]] auto append() -> T*
  {
    if (size_ < Max) {
      size_++;
      return &elements_[size_ - 1];
    }
    return nullptr;
  }
};

template<typename T>
class Array final
{
  T* elements_{};

  size_t size_{};

public:
  Array() = default;

  Array(Array&& other)
    : elements_(other.elements_)
    , size_(other.size_)
  {
    other.elements_ = nullptr;
    other.size_ = 0;
  }

  ~Array() { free(elements_); }

  [[nodiscard]] auto resize(const size_t size) -> bool
  {
    void* result = realloc(elements_, size * sizeof(T));
    if (!result) {
      return false;
    }
    elements_ = static_cast<T*>(result);
    size_ = size;
    return true;
  }

  [[nodiscard]] auto data() -> T* { return elements_; }

  [[nodiscard]] auto data() const -> const T* { return elements_; }

  [[nodiscard]] auto size() const -> size_t { return size_; }

  [[nodiscard]] auto operator[](const size_t i) const -> const T& { return elements_[i]; }

  [[nodiscard]] auto operator[](const size_t i) -> T& { return elements_[i]; }
};

struct Transform final
{
  Vec3f position;

  Vec3f rotation;

  [[nodiscard]] auto apply(const Vec3f& p) const -> Vec3f;
};
