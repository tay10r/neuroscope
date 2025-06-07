#include "tissue.h"

#include "core.h"

Tissue::Tissue()
{
  noise_.SetFrequency(0.001F);
  noise_.SetFractalOctaves(8);
  noise_.SetFractalType(FastNoiseLite::FractalType_FBm);
  noise_.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
}

void
Tissue::set_config(const TissueConfig& config)
{
  noise_.SetSeed(config.seed);

  config_ = config;
}

auto
Tissue::density(const Vec2f& position) const -> float
{
  float x = noise_.GetNoise(position[0], position[1]) + config_.coverage;

  if (x < 0.0F) {
    return 0.0F;
  }

  x = x * x;

  x = config_.max_density * (1.0F - expf(-x * 6.28F * 2.0F));

  return x;
}

void
Tissue::render(const ssize_t w, const ssize_t h, const float vertical_fov, uint8_t* buffer) const
{
  const auto num_pixels{ w * h };
  const auto x_scale{ 1.0F / static_cast<float>(w) };
  const auto y_scale{ 1.0F / static_cast<float>(h) };
  const auto aspect{ static_cast<float>(w) / static_cast<float>(h) };

#pragma omp parallel for

  for (ssize_t i = 0; i < num_pixels; i++) {

    const auto x{ i % w };
    const auto y{ i / w };

    const auto u{ (static_cast<float>(x) + 0.5F) * x_scale };
    const auto v{ (static_cast<float>(y) + 0.5F) * y_scale };

    const auto px{ (u * 2.0F - 1.0F) * aspect * vertical_fov * 0.5F };
    const auto py{ (v * 2.0F - 1.0F) * vertical_fov * 0.5F };

    const auto d = density(Vec2f{ px, py });

    buffer[i] = static_cast<uint8_t>(clamp(static_cast<int>(d * 255), 0, 255));
  }
}
