#pragma once

#include <stdint.h>

class Random final
{
  static constexpr uint32_t multiplier = 48271u;

  static constexpr uint32_t increment = 0u;

  static constexpr uint32_t modulus = 2147483647u;

  uint32_t state_{};

public:
  Random(uint32_t seed = 1) noexcept
    : state_(seed % modulus)
  {
    if (state_ == 0) {
      state_ = 1;
    }
  }

  [[nodiscard]] auto next() noexcept -> uint32_t
  {
    state_ = static_cast<uint32_t>(static_cast<uint64_t>(state_) * multiplier % modulus);
    return state_;
  }

  [[nodiscard]] auto next_float() noexcept -> float { return static_cast<float>(next()) / static_cast<float>(modulus); }
};
