#include "microscope.h"

#include "random.h"
#include "scene.h"
#include "swc.h"
#include "tissue.h"

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
      (void)what;
      // TODO : log this
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
MicroscopeBase::capture(const SWCModel& model, const Tissue& tissue, const Transform& t) -> bool
{
  Scene scene(device());

  if (!scene.from_swc_model(model, t)) {
    return false;
  }

  capture_impl(scene, tissue);

  return true;
}

SegmentationMicroscope::SegmentationMicroscope(const size_t image_width,
                                               const size_t image_height,
                                               const float vertical_fov)
  : sensor_(image_width, image_height)
  , vertical_fov_(vertical_fov)
{
}

void
SegmentationMicroscope::capture_impl(const Scene& scene, const Tissue&)
{
  const auto w = sensor_.width();
  const auto h = sensor_.height();
  auto* pixels = sensor_.get_array_data();

  const auto x_scale{ 1.0F / static_cast<float>(w) };
  const auto y_scale{ 1.0F / static_cast<float>(h) };
  const auto aspect{ static_cast<float>(w) / static_cast<float>(h) };
  const auto fov{ vertical_fov_ * 0.5F };
  const auto max_spp{ 16 };
  const auto elevation{ 1.0e6F };

#pragma omp parallel for

  for (ssize_t y = 0; y < static_cast<ssize_t>(h); y++) {

    auto* row = pixels + y * w * 3;

    for (size_t x = 0; x < w; x++) {

      int r{ 0 };
      int g{ 0 };
      int b{ 255 };

      Random rng(y * w + x);

      for (int i = 0; i < max_spp; i++) {

        const auto u = (static_cast<float>(x) + rng.next_float()) * x_scale;
        const auto v = (static_cast<float>(y) + rng.next_float()) * y_scale;

        const auto px = (u * 2.0F - 1.0F) * fov * aspect;
        const auto py = (v * 2.0F - 1.0F) * fov;

        const auto isect = scene.intersect1(Vec3f{ px, py, elevation }, Vec3f{ 0, 0, -1 });

        if (isect.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
          b = 0;
          if (scene.is_neurite(isect.hit.geomID)) {
            g = 255;
          } else {
            r = 255;
          }
          break;
        }
      }

      auto* pixel = row + x * 3;
      pixel[0] = r;
      pixel[1] = g;
      pixel[2] = b;
    }
  }
}

FluorescenceMicroscope::FluorescenceMicroscope(size_t image_width, size_t image_height, float vertical_fov)
  : sensor_(image_width, image_height)
  , vertical_fov_(vertical_fov)
{
  fluorescence_.SetFrequency(0.1F);
  fluorescence_.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
  fluorescence_.SetFractalType(FastNoiseLite::FractalType_Ridged);
  fluorescence_.SetFractalOctaves(4);
}

void
FluorescenceMicroscope::set_config(const FluorescenceConfig& config)
{
  config_ = config;
}

void
FluorescenceMicroscope::capture_impl(const Scene& scene, const Tissue& tissue)
{
  const auto w = sensor_.width();
  const auto h = sensor_.height();
  auto* pixels = sensor_.get_array_data();

  const auto x_scale{ 1.0F / static_cast<float>(w) };
  const auto y_scale{ 1.0F / static_cast<float>(h) };
  const auto aspect{ static_cast<float>(w) / static_cast<float>(h) };
  const auto fov{ vertical_fov_ * 0.5F };
  constexpr auto spp{ 16 };

  const auto bounds = scene.get_bounds();
  const auto z_scale = 1.0F / (bounds.upper_z - bounds.lower_z);

#pragma omp parallel for

  for (ssize_t y = 0; y < static_cast<ssize_t>(h); y++) {

    auto* row = pixels + y * w;

    for (size_t x = 0; x < w; x++) {

      Random rng(x + y * w);

      float intensity_sum{ 0.0F };

      for (int j = 0; j < spp; ++j) {

        const float u = (static_cast<float>(x) + rng.next_float()) * x_scale;
        const float v = (static_cast<float>(y) + rng.next_float()) * y_scale;

        const float px = (u * 2.0F - 1.0F) * fov * aspect;
        const float py = (v * 2.0F - 1.0F) * fov;

        const Vec3f ray_org{ px, py, bounds.upper_z };
        const Vec3f ray_dir{ 0, 0, -1 };

        const auto isect = scene.intersect1(ray_org, ray_dir);

        if (isect.hit.geomID == RTC_INVALID_GEOMETRY_ID) {
          intensity_sum += tissue.density(Vec2f{ px, py });
          continue;
        }

        const Vec3f hit_pos = ray_org + ray_dir * isect.ray.tfar;

        const float distance_intensity = 1.0F - isect.ray.tfar * z_scale;

        const float emission = fluorescence_.GetNoise(hit_pos[0], hit_pos[1], hit_pos[2]) * 0.5F + 0.5F;

        intensity_sum += distance_intensity * clamp(emission, config_.min_emission, config_.max_emission);
      }

      const float intensity_avg = intensity_sum * (1.0F / static_cast<float>(spp));

      row[x] = static_cast<int>(intensity_avg * 255);
    }
  }
}
