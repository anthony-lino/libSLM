#include <string>

#include <pybind11/pybind11.h>

#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>
#include <pybind11/functional.h>
#include <pybind11/complex.h>
#include <pybind11/eigen.h>

#include <tuple>

#include <App/Header.h>
#include <App/Layer.h>
#include <App/Model.h>
#include <App/Reader.h>
#include <App/Writer.h>

#include "utils.h"

namespace py = pybind11;

using namespace slm;

PYBIND11_MAKE_OPAQUE(std::vector<slm::LayerGeometry::Ptr>)
PYBIND11_MAKE_OPAQUE(std::vector<slm::BuildStyle::Ptr>)

PYBIND11_MODULE(slm, m) {

    m.doc() = R"pbdoc(
        PySLM Support Module
        -----------------------
        .. currentmodule:: slm
        .. autosummary::
           :toctree: _generate

    )pbdoc";

#if 1
    /*
     * Create a trampolise class for slm::Reader as this contains a pure virtual function
     */
    class PyReader : public slm::base::Reader {
    public:
        /* Inherit the constructors */
        using slm::base::Reader::Reader;

        /* Trampoline (need one for each virtual function) */
        int parse() override {
            PYBIND11_OVERRIDE_PURE(
                int, /* Return type */
                Reader,      /* Parent class */
                parse        /* Name of function in C++ (must match Python name) */
                             /* Argument(s) */
            );
        }

        double getLayerThickness() const override {
            PYBIND11_OVERRIDE_PURE(
                double, /* Return type */
                Reader,      /* Parent class */
                getLayerThickness  /* Name of function in C++ (must match Python name) */
                /* Argument(s) */
            );
        }
    };

    class PyWriter : public slm::base::Writer {
    public:
        /* Inherit the constructors */
        using slm::base::Writer::Writer;

        /* Trampoline (need one for each virtual function) */
        void write(const Header &header,
                   const std::vector<Model::Ptr> &models,
                   const std::vector<Layer::Ptr> &layers) override {
            PYBIND11_OVERRIDE_PURE(
                void, /* Return type */
                Writer,      /* Parent class */
                write,       /* Name of function in C++ (must match Python name) */
                header, models, layers              /* Argument(s) */
            );
        }

    };

    py::class_<slm::base::Reader, PyReader>(m, "Reader")
        .def(py::init())
        .def("setFilePath", &slm::base::Reader::setFilePath, py::arg("filename"))
        .def("getFilePath", &slm::base::Reader::getFilePath)
        .def("parse", &slm::base::Reader::parse)
        .def("getFileSize", &slm::base::Reader::getFileSize)
        .def("getLayerThickness", &slm::base::Reader::getLayerThickness)
        .def("getModelById", &slm::base::Reader::getModelById, py::arg("mid"))
        .def_property_readonly("layers", &slm::base::Reader::getLayers)
        .def_property_readonly("models", &slm::base::Reader::getModels);

    py::class_<slm::base::Writer, PyWriter>(m, "Writer")
        .def(py::init())
        .def(py::init<std::string>())
        .def("setFilePath", &slm::base::Writer::setFilePath)
        .def("getFilePath", &slm::base::Writer::getFilePath)
        .def("getLayerMinMax", &slm::base::Writer::getLayerMinMax)
        .def("getTotalNumHatches", &slm::base::Writer::getTotalNumHatches)
        .def("getTotalNumContours", &slm::base::Writer::getTotalNumContours)
        .def("getBoundingBox", &slm::base::Writer::getBoundingBox)
        .def_property("sortLayers", &slm::base::Writer::isSortingLayers, &slm::base::Writer::setSortLayers)
        .def("write", &slm::base::Writer::write, py::arg("header"), py::arg("models"), py::arg("layers"));

#endif

    py::enum_<slm::LaserMode>(m, "LaserMode")
        .value("Default", LaserMode::PULSE)
        .value("CW", LaserMode::CW)
        .value("Pulse", LaserMode::PULSE)
        .export_values();

    py::enum_<slm::ScanMode>(m, "ScanMode")
        .value("Default", ScanMode::NONE)
        .value("ContourFirst", ScanMode::CONTOUR_FIRST)
        .value("HatchFirst", ScanMode::HATCH_FIRST)
        .export_values();

    py::class_<slm::LayerGeometry, std::shared_ptr<slm::LayerGeometry>> layerGeomPyType(m, "LayerGeometry", py::dynamic_attr());

    layerGeomPyType.def(py::init())
        .def_readwrite("bid", &LayerGeometry::bid)
        .def_readwrite("mid", &LayerGeometry::mid)
        .def_property("coords", 
            [](LayerGeometry& self) {
                // Return as numpy array with float32 dtype
                return py::array_t<float>(
                    {self.coords.rows(), self.coords.cols()},  // shape
                    {sizeof(float), sizeof(float) * self.coords.rows()},  // strides (column-major)
                    self.coords.data(),  // data pointer
                    py::cast(self)  // parent object to keep alive
                );
            },
            [](LayerGeometry& self, const py::array_t<float>& arr) {
                // Convert input to float32 if needed and check shape
                py::array_t<float> arr_f32 = py::array_t<float>::ensure(arr);
                if (arr_f32.ndim() != 2)
                    throw std::runtime_error("coords must be a 2D array");
                
                // Resize and copy data
                self.coords.resize(arr_f32.shape(0), arr_f32.shape(1));
                std::memcpy(self.coords.data(), arr_f32.data(), arr_f32.size() * sizeof(float));
            })
        .def_property("type", &LayerGeometry::getType, nullptr)
        .def(py::pickle(
                [](py::object self) { // __getstate__
                    /* Return a tuple that fully encodes the state of the object */
                    return py::make_tuple(self.attr("bid"), self.attr("mid"), self.attr("coords"), self.attr("type"), self.attr("__dict__"));
                }, [](const py::tuple &t) {
                    if (t.size() != 5)
                        throw std::runtime_error("Invalid state!");

                    auto p = std::make_shared<LayerGeometry>();
                    p->bid = t[0].cast<int>();
                    p->mid = t[1].cast<int>();
                    
                    // Ensure float32 when unpickling
                    py::array_t<float> arr = py::array_t<float>::ensure(t[2]);
                    p->coords.resize(arr.shape(0), arr.shape(1));
                    std::memcpy(p->coords.data(), arr.data(), arr.size() * sizeof(float));
                    
                    auto py_state = t[3].cast<py::dict>();
                    return std::make_pair(p, py_state);
            }
            ));
            
    py::class_<slm::ContourGeometry, slm::LayerGeometry, std::shared_ptr<slm::ContourGeometry>>(m, "ContourGeometry", py::dynamic_attr())
        .def(py::init())
        .def(py::init<uint32_t, uint32_t>(), py::arg("mid"), py::arg("bid"))
        .def_property("type", &ContourGeometry::getType, nullptr)
        .def(py::pickle(
                [](py::object self) { // __getstate__
                    /* Return a tuple that fully encodes the state of the object */
                    return py::make_tuple(self.attr("bid"), self.attr("mid"), self.attr("coords"), self.attr("__dict__"));
                }, [](const py::tuple &t) {
                    if (t.size() != 4)
                        throw std::runtime_error("Invalid state!");

                    auto p = std::make_shared<slm::ContourGeometry>();
                    p->bid = t[0].cast<int>();
                    p->mid = t[1].cast<int>();
                    p->coords = t[2].cast<Eigen::MatrixXf>();
                    auto py_state = t[3].cast<py::dict>();
                    return std::make_pair(p, py_state);

               }
            ));

    py::class_<slm::HatchGeometry, slm::LayerGeometry, std::shared_ptr<HatchGeometry>>(m, "HatchGeometry", py::dynamic_attr())
         .def(py::init())
         .def(py::init<uint32_t, uint32_t>(), py::arg("mid"), py::arg("bid"))
         .def_property("type", &HatchGeometry::getType, nullptr)
         .def(py::pickle(
                [](py::object self) { // __getstate__
                    /* Return a tuple that fully encodes the state of the object */
                    return py::make_tuple(self.attr("bid"), self.attr("mid"), self.attr("coords"), self.attr("__dict__"));
                }, [](const py::tuple &t) {
                    if (t.size() != 4)
                        throw std::runtime_error("Invalid state!");

                    auto p = std::make_shared<slm::HatchGeometry>();
                    p->bid = t[0].cast<int>();
                    p->mid = t[1].cast<int>();
                    p->coords = t[2].cast<Eigen::MatrixXf>();
                    auto py_state = t[3].cast<py::dict>();
                    return std::make_pair(p, py_state);

               }
            ));

    py::class_<slm::PntsGeometry, slm::LayerGeometry, std::shared_ptr<slm::PntsGeometry>>(m, "PointsGeometry", py::dynamic_attr())
         .def(py::init())
         .def(py::init<uint32_t, uint32_t>(), py::arg("mid"), py::arg("bid"))
         .def_property("type", &PntsGeometry::getType, nullptr)
         .def(py::pickle(
                   [](py::object self) { // __getstate__
                       /* Return a tuple that fully encodes the state of the object */
                       return py::make_tuple(self.attr("bid"), self.attr("mid"), self.attr("coords"), self.attr("__dict__"));
                   }, [](const py::tuple &t) {
                       if (t.size() != 4)
                           throw std::runtime_error("Invalid state!");

                       auto p = std::make_shared<slm::PntsGeometry>();
                       p->bid = t[0].cast<int>();
                       p->mid = t[1].cast<int>();
                       p->coords = t[2].cast<Eigen::MatrixXf>();
                       auto py_state = t[3].cast<py::dict>();
                       return std::make_pair(p, py_state);
                  }
               ));

    py::enum_<slm::LayerGeometry::TYPE>(layerGeomPyType, "LayerGeometryType")
        .value("Invalid", slm::LayerGeometry::TYPE::INVALID)
        .value("Pnts", slm::LayerGeometry::TYPE::PNTS)
        .value("Polygon", slm::LayerGeometry::TYPE::POLYGON)
        .value("Hatch", slm::LayerGeometry::TYPE::HATCH)
        .export_values();

     slm::bind_my_vector<std::vector<slm::BuildStyle::Ptr>>(m, "VectorBuildStyle");
     slm::bind_my_vector<std::vector<slm::LayerGeometry::Ptr>>(m, "VectorLayerGeometry");

     py::implicitly_convertible<py::list, std::vector<slm::BuildStyle::Ptr>>();
     py::implicitly_convertible<py::list, std::vector<slm::LayerGeometry::Ptr>>();


#if 0
    py::class_<std::vector<slm::LayerGeometry::Ptr> >(m, "LayerGeometryVector")
        .def(py::init<>())
        .def("clear", &std::vector<LayerGeometry::Ptr>::clear)
        //.def("append",[](std::vector<LayerGeometry::Ptr> &v, LayerGeometry::Ptr &value) { v.push_back(value); },
          //          py::keep_alive<2,1>())
        .def("append", (void (LayerGeomList::*)(const slm::LayerGeometry::Ptr &)) &LayerGeomList::push_back,  py::keep_alive<1, 2>())
        //.def("append", (void (std::vector<slm::LayerGeometry::Ptr>::*push_back)(const std::vector<slm::LayerGeometry::Ptr> &)(&std::vector<LayerGeometry::Ptr>::push_back))
        .def("pop_back", &std::vector<LayerGeometry::Ptr>::pop_back)
        .def("__len__", [](const std::vector<LayerGeometry::Ptr> &v) { return v.size(); })
        .def("__iter__", [](std::vector<LayerGeometry::Ptr> &v) {
           return py::make_iterator(v.begin(), v.end());
        }, py::keep_alive<0, 1>()) /* Keep vector alive while iterator is used */
        .def("__bool__", &std::vector<LayerGeometry::Ptr>::empty)
        .def("__contains__",
                [](const std::vector<LayerGeometry::Ptr> &v, const LayerGeometry::Ptr &x) {
                    return std::find(v.begin(), v.end(), x) != v.end();
                },
                py::arg("x"),
                "Return true the container contains ``x``"
        );
#endif


    py::class_<slm::Header>(m, "Header", py::dynamic_attr())
        .def(py::init())
        .def_readwrite("filename", &Header::fileName)
        .def_readwrite("creator",  &Header::creator)
        .def_property("version",   &Header::version, &Header::setVersion)
        .def_readwrite("zUnit",    &Header::zUnit)
        .def(py::pickle(
                [](py::object self) { // __getstate__
                    return py::make_tuple(self.attr("filename"), self.attr("version"), self.attr("zUnit"), self.attr("__dict__"));
                },
                 [](const py::tuple &t) {
                     if (t.size() != 4)
                         throw std::runtime_error("Invalid state!");

                     auto p = slm::Header();
                     p.fileName = t[0].cast<std::string>();
                     py::tuple version = t[1].cast<py::tuple>();
                     p.vMajor = version[0].cast<int>();
                     p.vMinor = version[1].cast<int>();
                     p.zUnit  = t[2].cast<int>();

                     return p;

                }
            ));

    py::class_<slm::BuildStyle, std::shared_ptr<slm::BuildStyle>>(m, "BuildStyle", py::dynamic_attr())
        .def(py::init())
        .def_readwrite("bid",               &BuildStyle::id)
        .def_readwrite("name",              &BuildStyle::name)
        .def_readwrite("description",       &BuildStyle::description)
        .def_readwrite("laserPower",        &BuildStyle::laserPower)
        .def_readwrite("laserSpeed",        &BuildStyle::laserSpeed)
        .def_readwrite("laserFocus",        &BuildStyle::laserFocus)
        .def_readwrite("pointDistance",     &BuildStyle::pointDistance)
        .def_readwrite("pointExposureTime", &BuildStyle::pointExposureTime)
        .def_readwrite("laserId",   &BuildStyle::laserId)
        .def_readwrite("laserMode", &BuildStyle::laserMode)
        .def_readwrite("pointDelay", &BuildStyle::pointDelay)
        .def_readwrite("jumpDelay", &BuildStyle::jumpDelay)
        .def_readwrite("jumpSpeed", &BuildStyle::jumpSpeed)
        .def("setStyle", &BuildStyle::setStyle, "Sets the paramters of the buildstyle",
                         py::arg("bid"),
                         py::arg("focus"),
                         py::arg("power"),
                         py::arg("pointExposureTime"),
                         py::arg("pointExposureDistance"),
                         py::arg("speed") = 0.0,
                         py::arg("laserId") = 1,
                         py::arg("laserMode") = slm::LaserMode::PULSE)
        .def(py::pickle(
                [](py::object self) { // __getstate__
                    /* Return a tuple that fully encodes the state of the object */
                    return py::make_tuple(self.attr("bid"),
                                          self.attr("laserPower"), self.attr("laserSpeed"), self.attr("laserFocus"),
                                          self.attr("pointDistance"), self.attr("pointExposureTime"),
                                          self.attr("laserId"), self.attr("laserMode"),
                                          self.attr("name"), self.attr("description"),
                                          self.attr("pointDelay"),
                                          self.attr("jumpDelay"), self.attr("jumpSpeed"),
                                          self.attr("description"),
                                          self.attr("__dict__"));
                },
                 [](const py::tuple &t) {
                     if (t.size() != 7)
                         throw std::runtime_error("Invalid state!");

                     auto p = std::make_shared<BuildStyle>();

                     p->id = t[0].cast<int>();
                     p->laserPower = t[1].cast<float>();
                     p->laserSpeed = t[2].cast<float>();
                     p->laserFocus = t[3].cast<float>();
                     p->pointDistance = t[4].cast<int>();
                     p->pointExposureTime = t[5].cast<int>();
                     p->laserId = t[6].cast<int>();
                     p->laserMode = t[7].cast<slm::LaserMode>();
                     p->name = t[8].cast<std::u16string>();
                     p->description = t[9].cast<std::u16string>();
                     p->pointDelay = t[10].cast<int>();
                     p->jumpDelay = t[11].cast<int>();
                     p->jumpSpeed = t[12].cast<int>();
                     p->description = t[13].cast<std::u16string>();

                     auto py_state = t[14].cast<py::dict>();
                     return std::make_pair(p, py_state);

                }
            ));

    py::class_<slm::Model, std::shared_ptr<slm::Model>>(m, "Model", py::dynamic_attr())
        .def(py::init())
        .def(py::init<uint64_t, uint64_t>(), py::arg("mid"), py::arg("topSliceNum"))
        .def_property("mid", &Model::getId, &Model::setId)
        .def("__len__", [](const Model &s ) { return s.getBuildStyles().size(); })
        .def_property("buildStyles",py::cpp_function(&slm::Model::buildStylesRef,py::return_value_policy::reference, py::keep_alive<1,0>()),
                                    py::cpp_function(&slm::Model::setBuildStyles, py::keep_alive<1, 2>()))
        //.def_property("buildStyles", &Model::getBuildStyles, &Model::setBuildStyles)
        .def_property("topLayerId",  &Model::getTopSlice, &Model::setTopSlice)
        .def_property("name", &Model::getName, &Model::setName)
        .def_property("buildStyleName", &Model::getBuildStyleName, &Model::setBuildStlyeName)
        .def_property("buildStyleDescription", &Model::getBuildStyleDescription, &Model::setBuildStlyeDescription)
        .def(py::pickle(
                [](py::object self) { // __getstate__
                    /* Return a tuple that fully encodes the state of the object */
                    return py::make_tuple(self.attr("mid"),
                                          self.attr("name"),
                                          self.attr("buildStyleName"),
                                          self.attr("buildStyleDescription"),
                                          self.attr("topLayerId"),
                                          self.attr("buildStyles"),
                                          self.attr("__dict__"));
                },
                 [](const py::tuple &t) {
                     if (t.size() != 5)
                         throw std::runtime_error("Invalid state!");

                     auto p = std::make_shared<Model>(t[0].cast<int>(), /* Mid */
                                                      t[4].cast<int>() /* Top Layer Id */);

                     p->setName(t[1].cast<std::u16string>());
                     p->setBuildStlyeName(t[2].cast<std::u16string>());
                     p->setBuildStlyeDescription(t[3].cast<std::u16string>());
                     p->setBuildStyles(t[5].cast<std::vector<BuildStyle::Ptr>>());

                     auto py_state = t[6].cast<py::dict>();
                     return std::make_pair(p, py_state);

                }
            ));

    py::class_<slm::Layer, std::shared_ptr<slm::Layer>>(m, "Layer", py::dynamic_attr())
        .def(py::init())
        .def(py::init<uint64_t, uint64_t>(), py::arg("id"), py::arg("z"))
        .def("__len__", [](const Layer &s ) { return s.geometry().size(); })
        .def_property_readonly("layerFilePosition", &Layer::layerFilePosition)
        .def("isLoaded", &Layer::isLoaded)
        .def("getPointsGeometry", &Layer::getPntsGeometry)
        .def("getHatchGeometry", &Layer::getHatchGeometry)
        .def("getContourGeometry", &Layer::getContourGeometry)
        .def("appendGeometry", &Layer::appendGeometry,  py::keep_alive<1, 2>())
       // .def("geom", [](Layer &v) { return &(v.geometry()); }, py::keep_alive<1,0>())
        .def_property("geometry",py::cpp_function(&Layer::geometryRef,py::return_value_policy::reference, py::keep_alive<1,0>()),
                                 py::cpp_function(&Layer::setGeometry, py::keep_alive<1, 2>()))
        .def_property("z", &Layer::getZ, &Layer::setZ)
        .def_property("layerId", &Layer::getLayerId, &Layer::setLayerId)
        .def("getGeometry", &Layer::getGeometry, py::arg("scanMode") = slm::ScanMode::NONE)
        .def(py::pickle(
                [](py::object self) { // __getstate__
                    /* Return a tuple that fully encodes the state of the object */
                    return py::make_tuple(self.attr("layerId"), self.attr("z"), self.attr("geometry"), self.attr("__dict__"));
                },
                 [](const py::tuple &t) {
                     if (t.size() != 4)
                         throw std::runtime_error("Invalid state!");

                     auto p = std::make_shared<Layer>(t[0].cast<int>(), /* Layer Id */
                                                      t[1].cast<int>()  /* Layer Z */);


                     p->setGeometry(t[2].cast<std::vector<LayerGeometry::Ptr>>());
                     auto py_state = t[3].cast<py::dict>();
                     return std::make_pair(p, py_state);

                }
            ));

#ifdef PROJECT_VERSION
    m.attr("__version__") = "PROJECT_VERSION";
#else
    m.attr("__version__") = "dev";
#endif

}

