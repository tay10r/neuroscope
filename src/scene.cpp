#include "scene.h"

#include "swc.h"

Scene::Scene(RTCDevice device)
  : scene_(rtcNewScene(device))
  , soma_spherical_(rtcNewGeometry(device, RTC_GEOMETRY_TYPE_SPHERE_POINT))
  , soma_composite_(rtcNewGeometry(device, RTC_GEOMETRY_TYPE_ROUND_LINEAR_CURVE))
  , neurites_(rtcNewGeometry(device, RTC_GEOMETRY_TYPE_ROUND_LINEAR_CURVE))
{
}

Scene::~Scene()
{
  rtcReleaseGeometry(soma_spherical_);
  rtcReleaseGeometry(soma_composite_);
  rtcReleaseGeometry(neurites_);
  rtcReleaseScene(scene_);
}

auto
Scene::from_swc_model(const SWCModel& model, const Transform& t) -> bool
{
  size_t num_neurites = 0;

  size_t num_somas = 0;

  const size_t num_nodes = model.num_nodes();

  /* First we count the number of neurites (stem like structures extending from the soma)
   * and the number of soma nodes. The geometric model of a soma is dependent on the number
   * of nodes found. When only one node is found, it is modeled as a sphere. When more than
   * one node is found, it is modeled the same as a neurite.
   */

  for (size_t i = 0; i < num_nodes; i++) {
    const auto* node = model.find_node(i + 1);
    if (!node) {
      continue;
    }

    switch (node->type) {
      case SWCType::UNDEFINED:
      case SWCType::CUSTOM:
        break;
      case SWCType::GLIA_PROCESSES:
        // TODO
        break;
      case SWCType::SOMA:
        num_somas++;
        break;
      case SWCType::BASAL_DENDRITE:
      case SWCType::APICAL_DENDRITE:
      case SWCType::AXON:
      case SWCType::UNSPECIFIED_NEURITE:
        if (model.find_node(node->parent) != nullptr) {
          // only count it if it has a valid parent
          num_neurites++;
        }
        break;
    }
  }

  Vec4f* soma_buffer = nullptr;

  unsigned int* soma_indices = nullptr;

  if (num_somas == 1) {
    soma_buffer = static_cast<Vec4f*>(rtcSetNewGeometryBuffer(
      soma_spherical_, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT4, sizeof(float) * 4, num_somas));
  } else {
    soma_buffer = static_cast<Vec4f*>(rtcSetNewGeometryBuffer(
      soma_composite_, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT4, sizeof(float) * 4, num_somas * 2));
    soma_indices = static_cast<unsigned int*>(rtcSetNewGeometryBuffer(
      soma_composite_, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT, sizeof(unsigned int), num_somas));
  }

  auto* neurites_indices = static_cast<unsigned int*>(
    rtcSetNewGeometryBuffer(neurites_, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT, sizeof(unsigned int), num_neurites));

  auto* neurites_buffer = static_cast<Vec4f*>(rtcSetNewGeometryBuffer(
    neurites_, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT4, sizeof(float) * 4, num_neurites * 2));

  if (!neurite_types_.resize(num_neurites)) {
    return false;
  }

  size_t soma_offset = 0;

  size_t neurites_offset = 0;

  for (size_t i = 0; i < num_nodes; i++) {
    const auto* node = model.find_node(i + 1);
    if (!node) {
      continue;
    }

    const auto* parent = model.find_node(node->parent);

    switch (node->type) {
      case SWCType::UNDEFINED:
      case SWCType::CUSTOM:
        break;
      case SWCType::GLIA_PROCESSES:
        // TODO
        break;
      case SWCType::SOMA:
        if (num_somas == 1) {
          soma_buffer[soma_offset] = Vec4f{ node->position[0], node->position[1], node->position[2], node->radius };
        } else if (parent != nullptr) {
          const auto p0 = t.apply(parent->position);
          const auto p1 = t.apply(node->position);
          soma_buffer[soma_offset * 2 + 0] = Vec4f{ p0[0], p0[1], p0[2], parent->radius };
          soma_buffer[soma_offset * 2 + 1] = Vec4f{ p1[0], p1[1], p1[2], node->radius };
          soma_indices[soma_offset] = soma_offset * 2;
        }
        soma_offset++;
        break;
      case SWCType::BASAL_DENDRITE:
      case SWCType::APICAL_DENDRITE:
      case SWCType::AXON:
      case SWCType::UNSPECIFIED_NEURITE:
        if (parent) {
          const Vec3f p0 = t.apply(parent->position);
          const Vec3f p1 = t.apply(node->position);
          neurites_buffer[neurites_offset * 2 + 0] = Vec4f{ p0[0], p0[1], p0[2], parent->radius };
          neurites_buffer[neurites_offset * 2 + 1] = Vec4f{ p1[0], p1[1], p1[2], node->radius };
          neurites_indices[neurites_offset] = neurites_offset * 2;
          neurite_types_[neurites_offset] = static_cast<uint8_t>(node->type);
          neurites_offset++;
        }
        break;
    }
  }

  if (num_somas == 1) {
    rtcCommitGeometry(soma_spherical_);
    soma_id_ = rtcAttachGeometry(scene_, soma_spherical_);
  } else if (num_somas > 0) {
    rtcCommitGeometry(soma_composite_);
    soma_id_ = rtcAttachGeometry(scene_, soma_composite_);
  }

  if (num_neurites > 0) {
    rtcCommitGeometry(neurites_);
    neurites_id_ = rtcAttachGeometry(scene_, neurites_);
  }

  rtcCommitScene(scene_);

  return true;
}
