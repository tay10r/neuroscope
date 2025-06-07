#include "core.h"

auto
Transform::apply(const Vec3f& p) const -> Vec3f
{
  const auto cx = cosf(rotation[0]);
  const auto sx = sinf(rotation[0]);
  const auto cy = cosf(rotation[1]);
  const auto sy = sinf(rotation[1]);
  const auto cz = cosf(rotation[2]);
  const auto sz = sinf(rotation[2]);

  const auto m00 = cy * cz;
  const auto m01 = cy * sz;
  const auto m02 = -sy;

  const auto m10 = sx * sy * cz - cx * sz;
  const auto m11 = sx * sy * sz + cx * cz;
  const auto m12 = sx * cy;

  const auto m20 = cx * sy * cz + sx * sz;
  const auto m21 = cx * sy * sz - sx * cz;
  const auto m22 = cx * cy;

  const Vec3f r{ m00 * p[0] + m01 * p[1] + m02 * p[2],
                 m10 * p[0] + m11 * p[1] + m12 * p[2],
                 m20 * p[0] + m21 * p[1] + m22 * p[2] };

  return r + position;
}
