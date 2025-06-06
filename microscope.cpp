#include "microscope.h"

#include "random.h"
#include "scene.h"
#include "swc.h"

#include <random>

#include <stdio.h>

MicroscopeBase::MicroscopeBase()
  : device_(rtcNewDevice(""))
{
}

MicroscopeBase::MicroscopeBase(MicroscopeBase&& other)
  : device_(other.device_)
{
  rtcSetDeviceErrorFunction(
    device_,
    [](void*, RTCError, const char* what) {
      printf("embree error: %s\n", what);
      //
    },
    this);

  rtcRetainDevice(device_);
}

MicroscopeBase::~MicroscopeBase()
{
  if (device_) {
    rtcReleaseDevice(device_);
  }
}

auto
MicroscopeBase::device() -> RTCDevice
{
  return device_;
}

auto
MicroscopeBase::capture(const SWCModel& model) -> bool
{
  Scene scene(device());

  if (!scene.from_swc_model(model)) {
    return false;
  }

  capture_impl(scene);

  return true;
}

DebugMicroscope::DebugMicroscope(size_t image_width, size_t image_height, float vertical_fov, float elevation)
  : sensor_(image_width, image_height)
  , vertical_fov_(vertical_fov)
  , elevation_(elevation)
{
}

void
DebugMicroscope::capture_impl(const Scene& scene)
{
  const auto w = sensor_.width();
  const auto h = sensor_.height();
  auto* pixels = sensor_.get_array_data();

  const auto x_scale{ 1.0F / static_cast<float>(w) };
  const auto y_scale{ 1.0F / static_cast<float>(h) };
  const auto aspect{ static_cast<float>(w) / static_cast<float>(h) };
  const auto fov{ vertical_fov_ * 0.5F };

#pragma omp parallel for

  for (ssize_t y = 0; y < static_cast<ssize_t>(h); y++) {

    auto* row = pixels + y * w * 3;

    for (size_t x = 0; x < w; x++) {

      const auto u = static_cast<float>(x) * x_scale;
      const auto v = static_cast<float>(y) * y_scale;

      const auto px = (u * 2.0F - 1.0F) * fov * aspect;
      const auto py = (v * 2.0F - 1.0F) * fov;

      const auto isect = scene.intersect1(Vec3f{ px, py, elevation_ }, Vec3f{ 0, 0, -1 });

      auto* pixel = row + x * 3;

      if (isect.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
        if (scene.is_neurite(isect.hit.geomID)) {
          pixel[0] = 0;
          pixel[1] = 255;
          pixel[2] = 0;
        } else {
          pixel[0] = 255;
          pixel[1] = 0;
          pixel[2] = 0;
        }
      } else {
        pixel[0] = 0;
        pixel[1] = 0;
        pixel[2] = 255;
      }
    }
  }
}

GenericFluorescentMicroscope::GenericFluorescentMicroscope(size_t image_width,
                                                           size_t image_height,
                                                           float vertical_fov,
                                                           float distance_per_slice)
  : sensor_(image_width, image_height)
  , vertical_fov_(vertical_fov)
  , distance_per_slice_(distance_per_slice)
{
}

void
GenericFluorescentMicroscope::capture_impl(const Scene& scene)
{
  const auto w = sensor_.width();
  const auto h = sensor_.height();
  auto* pixels = sensor_.get_array_data();

  const auto x_scale{ 1.0F / static_cast<float>(w) };
  const auto y_scale{ 1.0F / static_cast<float>(h) };
  const auto aspect{ static_cast<float>(w) / static_cast<float>(h) };
  const auto fov{ vertical_fov_ * 0.5F };
  constexpr auto spp{ 32 };

  const auto bounds = scene.get_bounds();

  const int num_slices = static_cast<int>((bounds.upper_z - bounds.lower_z) / distance_per_slice_);
  constexpr float axial_FWHM = 2.8f; // µm (tweak!)
  const float sigma_z = axial_FWHM / (2.0f * sqrtf(2.0f * logf(2.0f)));

#pragma omp parallel for

  for (ssize_t y = 0; y < static_cast<ssize_t>(h); y++) {

    auto* row = pixels + y * w;

    for (size_t x = 0; x < w; x++) {

      std::seed_seq seed{ static_cast<int>(x), static_cast<int>(y) };

      std::minstd_rand rng(seed);

      float max_intensity{ 0.0F };

      for (int i = 0; i < num_slices; i++) {

        float intensity_sum{ 0.0F };

        const float elevation = bounds.upper_z - i * distance_per_slice_;

        for (auto j = 0; j < spp; j++) {

          std::normal_distribution<float> dist(0.5F, 1.0F);

          const auto u = (static_cast<float>(x) + dist(rng)) * x_scale;
          const auto v = (static_cast<float>(y) + dist(rng)) * y_scale;

          const auto px = (u * 2.0F - 1.0F) * fov * aspect;
          const auto py = (v * 2.0F - 1.0F) * fov;

          const Vec3f ray_org{ px, py, elevation };
          const Vec3f ray_dir{ 0, 0, -1 };

          const auto isect = scene.intersect1(ray_org, ray_dir);

          auto intensity{ 0.0F };

          if (isect.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
            const float hit_z = ray_org[2] + isect.ray.tfar * ray_dir[2];
            const float delta_z = hit_z - elevation;
            const float weight_axial = expf(-0.5F * (delta_z * delta_z) / (sigma_z * sigma_z));
            intensity = weight_axial;
          }

          intensity_sum += intensity;
        }

        const auto intensity = intensity_sum * (1.0F / static_cast<float>(spp));

        max_intensity = (intensity > max_intensity) ? intensity : max_intensity;
      }

      row[x] = static_cast<int>(max_intensity * 255);
    }
  }
}
