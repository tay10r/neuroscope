#pragma once

#include "core.h"

#include <stdint.h>

enum class SWCType : uint8_t
{
  UNDEFINED = 0,
  SOMA = 1,
  AXON = 2,
  BASAL_DENDRITE = 3,
  APICAL_DENDRITE = 4,
  CUSTOM = 5,
  UNSPECIFIED_NEURITE = 6,
  GLIA_PROCESSES = 7
};

struct SWCNode final
{
  int32_t id = 0;

  SWCType type{ SWCType::UNDEFINED };

  Vec3f position;

  float radius{ 0 };

  int32_t parent = 0;
};

class SWCModel final
{
  Array<SWCNode> nodes_;

public:
  [[nodiscard]] auto find_node(int32_t id) const -> const SWCNode*;

  [[nodiscard]] auto load_from_file(const char* path) -> bool;

  [[nodiscard]] auto num_nodes() const -> size_t;
};
