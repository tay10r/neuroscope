#include "swc.h"

#include "core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace {

template<typename Callback>
auto
scan(FILE* file, Callback cb) -> bool
{
  if (fseek(file, 0, SEEK_SET) != 0) {
    return false;
  }

  char line_buffer[256];

  while (!feof(file)) {

    memset(line_buffer, 0, sizeof(line_buffer));

    fgets(line_buffer, sizeof(line_buffer), file);

    if ((line_buffer[0] == '#') || (line_buffer[0] == 0)) {
      continue;
    }

    int id{};
    int t{};
    float x{};
    float y{};
    float z{};
    float r{};
    int parent{};

    if (sscanf(line_buffer, "%d %d %f %f %f %f %d", &id, &t, &x, &y, &z, &r, &parent) != 7) {
      return false;
    }

    cb(id, t, x, y, z, r, parent);
  }

  return true;
}

} // namespace

auto
SWCModel::find_node(const int32_t id) const -> const SWCNode*
{
  auto cmp_key = [](const void* key, const void* node) -> int {
    return (*static_cast<const int32_t*>(key)) - static_cast<const SWCNode*>(node)->id;
  };

  const void* result = bsearch(&id, nodes_.data(), nodes_.size(), sizeof(SWCNode), cmp_key);

  return static_cast<const SWCNode*>(result);
}

auto
SWCModel::load_from_file(const char* path) -> bool
{
  auto* file = fopen(path, "r");
  if (!file) {
    return false;
  }

  size_t num_nodes{};

  auto count_nodes = [&num_nodes](int, int, float, float, float, float, int) { num_nodes++; };

  if (!scan(file, count_nodes)) {
    return false;
  }

  if (!nodes_.resize(num_nodes)) {
    return false;
  }

  size_t index{};

  auto init_nodes = [this, &index](int id, int type, float x, float y, float z, float r, int parent) {
    nodes_[index] = SWCNode{ id, static_cast<SWCType>(type), Vec3f{ x, y, z }, r, parent };
    index++;
  };

  if (!scan(file, init_nodes)) {
    return false;
  }

  auto cmp = [](const void* a, const void* b) -> int {
    return static_cast<const SWCNode*>(a)->id - static_cast<const SWCNode*>(b)->id;
  };

  qsort(nodes_.data(), nodes_.size(), sizeof(SWCNode), cmp);

  return true;
}

auto
SWCModel::num_nodes() const -> size_t
{
  return nodes_.size();
}
