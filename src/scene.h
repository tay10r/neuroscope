/**
 * @file scene.h
 *
 * @brief This file contains the visual modeling functions for the cells.
 * */

#pragma once

#include "core.h"

#include <embree4/rtcore.h>

#include <assert.h>
#include <math.h>
#include <stdint.h>

class SWCModel;

class Scene final
{
  RTCScene scene_;

  RTCGeometry soma_spherical_;

  RTCGeometry soma_composite_;

  RTCGeometry neurites_;

  unsigned int soma_id_{};

  unsigned int neurites_id_{};

  Array<uint8_t> neurite_types_;

public:
  Scene(RTCDevice device);

  ~Scene();

  [[nodiscard]] auto from_swc_model(const SWCModel& model, const Transform& t) -> bool;

  [[nodiscard]] auto is_neurite(const unsigned int geom_id) const -> bool { return geom_id == neurites_id_; }

  /**
   * @brief Gets the type of a neurite.
   *
   * @note The primitive id should be the field in RTCHit when the geometry ID indicates that it is in fact a neurite.
   * */
  auto find_neurite_type(const unsigned int primitive_id) const -> uint8_t
  {
    assert(primitive_id < neurite_types_.size());
    return (primitive_id < neurite_types_.size()) ? neurite_types_[primitive_id] : 0;
  }

  auto intersect1(const Vec3f& org, const Vec3f& dir) const -> RTCRayHit
  {
    RTCRayHit ray_hit{};

    ray_hit.ray.org_x = org[0];
    ray_hit.ray.org_y = org[1];
    ray_hit.ray.org_z = org[2];

    ray_hit.ray.dir_x = dir[0];
    ray_hit.ray.dir_y = dir[1];
    ray_hit.ray.dir_z = dir[2];

    ray_hit.ray.tfar = static_cast<float>(INFINITY);

    ray_hit.ray.mask = -1;
    ray_hit.ray.flags = 0;
    ray_hit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
    ray_hit.hit.primID = RTC_INVALID_GEOMETRY_ID;
    ray_hit.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;

    rtcIntersect1(scene_, &ray_hit);

    return ray_hit;
  }

  auto get_bounds() const -> RTCBounds
  {
    RTCBounds bounds{};
    rtcGetSceneBounds(scene_, &bounds);
    return bounds;
  }
};
