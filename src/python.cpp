#include <pybind11/pybind11.h>

#include "microscope.h"
#include "swc.h"

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

  py::class_<Microscope>(m, "Microscope").def("capture", &Microscope::capture);

  py::class_<DebugMicroscope, Microscope>(m, "DebugMicroscope")
    .def(py::init<size_t, size_t, float, float>(),
         py::arg("image_width") = 640,
         py::arg("image_height") = 480,
         py::arg("vertical_fov") = 500,
         py::arg("elevation") = 100)
    .def("image_size",
         [](const DebugMicroscope& self) -> py::tuple {
           const auto& sensor = self.get_sensor();
           return py::make_tuple(sensor.width(), sensor.height());
         })
    .def("copy_rgb_buffer", [](const DebugMicroscope& self) -> py::bytes {
      auto& sensor = self.get_sensor();
      auto* data = sensor.get_array_data();
      auto size = sensor.get_array_size();
      return py::bytes(reinterpret_cast<const char*>(data), size);
    });

  py::class_<GenericFluorescentMicroscope, Microscope>(m, "GenericFluorescentMicroscope")
    .def(py::init<size_t, size_t, float, float, float>(),
         py::arg("image_width") = 640,
         py::arg("image_height") = 480,
         py::arg("vertical_fov") = 500,
         py::arg("distance_per_slice") = 2.5F,
         py::arg("axial_fwhm") = 2.0F)
    .def("image_size",
         [](const GenericFluorescentMicroscope& self) -> py::tuple {
           const auto& sensor = self.get_sensor();
           return py::make_tuple(sensor.width(), sensor.height());
         })
    .def("copy_buffer", [](const GenericFluorescentMicroscope& self) -> py::bytes {
      auto& sensor = self.get_sensor();
      auto* data = sensor.get_array_data();
      auto size = sensor.get_array_size();
      return py::bytes(reinterpret_cast<const char*>(data), size);
    });
  ;
}
