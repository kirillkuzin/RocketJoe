#pragma once

#include <pybind11/pybind11.h>

namespace components { namespace python_sandbox { namespace detail {

    namespace py = pybind11;

    class context_manager;

    auto add_jupyter(py::module&, context_manager*) -> void;

}}} // namespace components::python_sandbox::detail