# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FindShaderC
----------

Try to find SPIR-V Shader Compiler

IMPORTED Targets
^^^^^^^^^^^^^^^^

This module defines :prop_tgt:`IMPORTED` target ``ShaderC::ShaderC``, if
Shader Compiler has been found.

Result Variables
^^^^^^^^^^^^^^^^

This module defines the following variables::

  ShaderC_FOUND          - True if Shader Compiler was found
  ShaderC_INCLUDE_DIRS   - include directories for Shader Compiler
  ShaderC_LIBRARIES      - link against this library to use Shader Compiler

The module will also define two cache variables::

  ShaderC_INCLUDE_DIR    - the Shader Compiler include directory
  ShaderC_LIBRARY        - the path to the Shader Compiler library

#]=======================================================================]

if(WIN32)
  find_path(ShaderC_INCLUDE_DIR
    NAMES shaderc/shaderc.h
    PATHS
      "$ENV{VULKAN_SDK}/Include"
    )

  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    find_library(ShaderC_LIBRARY
      NAMES shaderc_combined
      PATHS
        "$ENV{VULKAN_SDK}/Lib"
        "$ENV{VULKAN_SDK}/Bin"
        )
  elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    find_library(ShaderC_LIBRARY
      NAMES shaderc_combined
      PATHS
        "$ENV{VULKAN_SDK}/Lib32"
        "$ENV{VULKAN_SDK}/Bin32"
        NO_SYSTEM_ENVIRONMENT_PATH
        )
  endif()
else()
    find_path(ShaderC_INCLUDE_DIR
      NAMES shaderc/shaderc.h
      PATHS
        "$ENV{VULKAN_SDK}/include")
    find_library(ShaderC_LIBRARY
      NAMES shaderc
      PATHS
        "$ENV{VULKAN_SDK}/lib")
endif()

set(ShaderC_LIBRARIES ${ShaderC_LIBRARY})
set(ShaderC_INCLUDE_DIRS ${ShaderC_INCLUDE_DIR})

find_package_handle_standard_args(ShaderC
  DEFAULT_MSG
  ShaderC_LIBRARY ShaderC_INCLUDE_DIR)

mark_as_advanced(ShaderC_INCLUDE_DIR ShaderC_LIBRARY)

if(ShaderC_FOUND AND NOT TARGET ShaderC::ShaderC)
  add_library(ShaderC::ShaderC UNKNOWN IMPORTED)
  set_target_properties(ShaderC::ShaderC PROPERTIES
    IMPORTED_LOCATION "${ShaderC_LIBRARIES}"
    INTERFACE_INCLUDE_DIRECTORIES "${ShaderC_INCLUDE_DIRS}")
endif()
