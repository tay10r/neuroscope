#pragma once

#include <embree4/rtcore.h>

#include <stdint.h>
#include <stdlib.h>

class SWCModel;
class Scene;

template<typename T, size_t C>
class ImageSensor
{
  T* data_{};

  size_t width_{};

  size_t height_{};

public:
  ImageSensor(const size_t w, const size_t h)
  {
    data_ = static_cast<T*>(malloc(w * h * C));
    if (data_) {
      width_ = w;
      height_ = h;
    }
  }

  ImageSensor(ImageSensor&& other)
    : data_(other.data_)
    , width_(other.width_)
    , height_(other.height_)
  {
    other.data_ = nullptr;
    other.width_ = 0;
    other.height_ = 0;
  }

  ~ImageSensor() { free(data_); }

  [[nodiscard]] auto get_array_data() -> uint8_t* { return data_; }

  [[nodiscard]] auto get_array_data() const -> const uint8_t* { return data_; }

  [[nodiscard]] auto get_array_size() const -> size_t { return width_ * height_ * C; }

  [[nodiscard]] auto width() const -> size_t { return width_; }

  [[nodiscard]] auto height() const -> size_t { return height_; }

  ImageSensor(const ImageSensor&) = delete;

  auto operator=(const ImageSensor&) -> ImageSensor& = delete;

  auto operator=(ImageSensor&&) -> ImageSensor& = delete;
};

class Microscope
{
public:
  virtual ~Microscope() = default;

  [[nodiscard]] virtual auto capture(const SWCModel&) -> bool = 0;
};

class MicroscopeBase : public Microscope
{
  RTCDevice device_{};

public:
  MicroscopeBase();

  MicroscopeBase(MicroscopeBase&&);

  ~MicroscopeBase();

  MicroscopeBase(const MicroscopeBase&) = delete;

  auto operator=(const MicroscopeBase&) -> Microscope& = delete;

  auto operator=(MicroscopeBase&&) -> Microscope& = delete;

  auto capture(const SWCModel& model) -> bool override;

protected:
  [[nodiscard]] auto device() -> RTCDevice;

  virtual void capture_impl(const Scene& scene) = 0;
};

class DebugMicroscope : public MicroscopeBase
{
  ImageSensor<uint8_t, 3> sensor_;

  float vertical_fov_{ 100.0F };

  float elevation_{ 100.0F };

public:
  DebugMicroscope(size_t image_width, size_t image_height, float vertical_fov, float elevation);

  [[nodiscard]] auto get_sensor() const -> const ImageSensor<uint8_t, 3>& { return sensor_; }

protected:
  void capture_impl(const Scene& scene) override;
};

class GenericFluorescentMicroscope : public MicroscopeBase
{
  ImageSensor<uint8_t, 1> sensor_;

  float vertical_fov_{ 100.0F };

  float distance_per_slice_{ 2.0F };

  float axial_fwhm_{ 2.8F };

public:
  GenericFluorescentMicroscope(size_t image_width,
                               size_t image_height,
                               float vertical_fov,
                               float distance_per_slice,
                               float axial_fwhm);

  [[nodiscard]] auto get_sensor() const -> const ImageSensor<uint8_t, 1>& { return sensor_; }

protected:
  void capture_impl(const Scene& scene) override;
};
