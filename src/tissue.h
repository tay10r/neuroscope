#pragma once

#include "core.h"

#include "FastNoiseLite.h"

#include <stddef.h>
#include <stdint.h>

struct TissueConfig final
{
  int seed{ 1337 };

  float max_density{ 0.1F };

  float coverage{ 0.9F };
};

class Tissue final
{
  FastNoiseLite noise_;

  TissueConfig config_;

public:
  Tissue();

  void set_config(const TissueConfig& config);

  [[nodiscard]] auto density(const Vec2f& position) const -> float;

  void render(ssize_t w, ssize_t h, const float vertical_fov, uint8_t* buffer) const;
};
