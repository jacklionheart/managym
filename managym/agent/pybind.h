#pragma once

#include <pybind11/pybind11.h>

/**
 * Register the C++-side classes/enums with Pybind11. 
 * 
 * Typically you have:
 *   void registerEnums(py::module& m);
 *   void registerDataClasses(py::module& m);
 *   void registerGameState(py::module& m);
 * etc.
 */
namespace py = pybind11;