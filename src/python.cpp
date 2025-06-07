#include <pybind11/pybind11.h>

#include "microscope.h"
#include "swc.h"
#include "tissue.h"

#include <stdlib.h>

namespace {

namespace py = pybind11;

} // namespace

PYBIND11_MODULE(neuroscope, m)
{
  py::class_<Vec3f>(m, "Vec3f")
    .def(py::init<float, float, float>(), py::arg("x") = 0, py::arg("y") = 0, py::arg("z"))
    .def("x", [](const Vec3f& self) -> float { return self[0]; })
    .def("y", [](const Vec3f& self) -> float { return self[1]; })
    .def("z", [](const Vec3f& self) -> float { return self[2]; });

  py::class_<Transform>(m, "Transform")
    .def(py::init<Vec3f, Vec3f>(), py::arg("position") = Vec3f{}, py::arg("rotation") = Vec3f{})
    .def_readwrite("position", &Transform::position)
    .def_readwrite("rotation", &Transform::rotation);

  py::enum_<SWCType>(m, "SWCType")
    .value("UNDEFINED", SWCType::UNDEFINED)
    .value("SOMA", SWCType::SOMA)
    .value("AXON", SWCType::AXON)
    .value("BASAL_DENDRITE", SWCType::BASAL_DENDRITE)
    .value("APICAL_DENDRITE", SWCType::APICAL_DENDRITE)
    .value("CUSTOM", SWCType::CUSTOM)
    .value("UNSPECIFIED_NEURITE", SWCType::UNSPECIFIED_NEURITE)
    .value("GLIA_PROCESSES", SWCType::GLIA_PROCESSES);

  py::class_<SWCNode>(m, "SWCNode")
    .def(py::init<>())
    .def_readwrite("id_", &SWCNode::id)
    .def_readwrite("type", &SWCNode::type)
    .def_readwrite("position", &SWCNode::position)
    .def_readwrite("radius", &SWCNode::radius)
    .def_readwrite("parent", &SWCNode::parent);

  py::class_<SWCModel>(m, "SWCModel")
    .def(py::init<>())
    .def("load_from_file", &SWCModel::load_from_file, py::arg("path"))
    .def("num_nodes", &SWCModel::num_nodes)
    .def("find_node", [](const SWCModel& model, const int32_t key) -> std::unique_ptr<SWCNode> {
      const auto* result = model.find_node(key);
      if (result) {
        return std::make_unique<SWCNode>(*result);
      } else {
        return nullptr;
      }
    });

  py::class_<Microscope>(m, "Microscope")
    .def("capture", &Microscope::capture, py::arg("model"), py::arg("tissue"), py::arg("transform") = Transform{});

  py::class_<SegmentationMicroscope, Microscope>(m, "SegmentationMicroscope")
    .def(py::init<size_t, size_t, float>(),
         py::arg("image_width") = 640,
         py::arg("image_height") = 480,
         py::arg("vertical_fov") = 500)
    .def("image_size",
         [](const SegmentationMicroscope& self) -> py::tuple {
           const auto& sensor = self.get_sensor();
           return py::make_tuple(sensor.width(), sensor.height());
         })
    .def("copy_rgb_buffer", [](const SegmentationMicroscope& self) -> py::bytes {
      auto& sensor = self.get_sensor();
      auto* data = sensor.get_array_data();
      auto size = sensor.get_array_size();
      return py::bytes(reinterpret_cast<const char*>(data), size);
    });

  py::class_<FluorescenceConfig>(m, "FluorescenceConfig")
    .def(py::init<>())
    .def_readwrite("seed", &FluorescenceConfig::seed)
    .def_readwrite("min_emission", &FluorescenceConfig::min_emission)
    .def_readwrite("max_emission", &FluorescenceConfig::max_emission);

  py::class_<FluorescenceMicroscope, Microscope>(m, "FluorescenceMicroscope")
    .def(py::init<size_t, size_t, float>(),
         py::arg("image_width") = 640,
         py::arg("image_height") = 480,
         py::arg("vertical_fov") = 500)
    .def("image_size",
         [](const FluorescenceMicroscope& self) -> py::tuple {
           const auto& sensor = self.get_sensor();
           return py::make_tuple(sensor.width(), sensor.height());
         })
    .def("copy_buffer",
         [](const FluorescenceMicroscope& self) -> py::bytes {
           auto& sensor = self.get_sensor();
           auto* data = sensor.get_array_data();
           auto size = sensor.get_array_size();
           return py::bytes(reinterpret_cast<const char*>(data), size);
         })
    .def("set_config", &FluorescenceMicroscope::set_config, py::arg("config"));

  py::class_<TissueConfig>(m, "TissueConfig")
    .def(py::init<>())
    .def_readwrite("seed", &TissueConfig::seed)
    .def_readwrite("coverage", &TissueConfig::coverage)
    .def_readwrite("max_density", &TissueConfig::max_density);

  py::class_<Tissue>(m, "Tissue")
    .def(py::init<>())
    .def("set_config", &Tissue::set_config, py::arg("config"))
    .def("density", &Tissue::density, py::arg("position"))
    .def(
      "render",
      [](const Tissue& self, const ssize_t w, const ssize_t h, const float vertical_fov) -> py::bytes {
        void* buffer = malloc(w * h);
        if (!buffer) {
          return py::none();
        }
        self.render(w, h, vertical_fov, static_cast<uint8_t*>(buffer));
        auto result = py::bytes(static_cast<const char*>(buffer), w * h);
        free(buffer);
        return result;
      },
      py::arg("image_width"),
      py::arg("image_height"),
      py::arg("vertical_fov"));
}
