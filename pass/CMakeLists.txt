cmake_minimum_required(VERSION 3.20)
project(cs4277-p1-pass LANGUAGES CXX)

set(LLVM_INSTALL_DIR "D:/clang+llvm-22.1.4-x86_64-pc-windows-msvc" CACHE PATH
    "LLVM 22.1.4 installation directory")

if(NOT LLVM_DIR)
  set(LLVM_DIR "${LLVM_INSTALL_DIR}/lib/cmake/llvm" CACHE PATH
      "Directory containing LLVMConfig.cmake")
endif()

if(NOT EXISTS "${LLVM_DIR}/LLVMConfig.cmake")
  message(FATAL_ERROR
    "Could not find LLVMConfig.cmake at:\n"
    "  ${LLVM_DIR}/LLVMConfig.cmake\n"
    "LLVM pass plugins need a full LLVM development install with C++ headers, "
    "CMake package files, and LLVM libraries. The LLVM runtime-only Windows "
    "package is not enough.")
endif()

find_package(LLVM 22.1.4 REQUIRED CONFIG)

if(NOT LLVM_VERSION_MAJOR EQUAL 22)
  message(FATAL_ERROR "Found LLVM ${LLVM_PACKAGE_VERSION}, but this pass is configured for LLVM 22.1.4")
endif()

set(HAVE_LLVM_CXX_HEADERS FALSE)
foreach(include_dir IN LISTS LLVM_INCLUDE_DIRS)
  if(EXISTS "${include_dir}/llvm/IR/IRBuilder.h")
    set(HAVE_LLVM_CXX_HEADERS TRUE)
  endif()
endforeach()

if(NOT HAVE_LLVM_CXX_HEADERS)
  message(FATAL_ERROR
    "LLVM C++ headers were not found in LLVM_INCLUDE_DIRS:\n"
    "  ${LLVM_INCLUDE_DIRS}\n"
    "Install or point LLVM_DIR at a full LLVM 22.1.4 development build.")
endif()

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake from: ${LLVM_DIR}")
message(STATUS "LLVM include dirs: ${LLVM_INCLUDE_DIRS}")
message(STATUS "LLVM library dirs: ${LLVM_LIBRARY_DIRS}")

if(WIN32 AND TARGET LLVMDebugInfoPDB)
  find_library(DIAGUIDS_LIBRARY
    NAMES diaguids
    PATHS
      "$ENV{VSINSTALLDIR}/DIA SDK/lib/amd64"
      "C:/Program Files/Microsoft Visual Studio/18/Community/DIA SDK/lib/amd64"
      "C:/Program Files/Microsoft Visual Studio/2022/Community/DIA SDK/lib/amd64"
      "C:/Program Files/Microsoft Visual Studio/2022/Enterprise/DIA SDK/lib/amd64"
    NO_DEFAULT_PATH
  )

  if(NOT DIAGUIDS_LIBRARY)
    find_library(DIAGUIDS_LIBRARY NAMES diaguids)
  endif()

  if(DIAGUIDS_LIBRARY)
    get_target_property(LLVM_DEBUGINFO_PDB_LINK_LIBS LLVMDebugInfoPDB
      INTERFACE_LINK_LIBRARIES)
    set(LLVM_DEBUGINFO_PDB_FIXED_LINK_LIBS)

    foreach(link_lib IN LISTS LLVM_DEBUGINFO_PDB_LINK_LIBS)
      if(link_lib MATCHES "diaguids\\.lib$" AND NOT EXISTS "${link_lib}")
        list(APPEND LLVM_DEBUGINFO_PDB_FIXED_LINK_LIBS "${DIAGUIDS_LIBRARY}")
      else()
        list(APPEND LLVM_DEBUGINFO_PDB_FIXED_LINK_LIBS "${link_lib}")
      endif()
    endforeach()

    set_target_properties(LLVMDebugInfoPDB PROPERTIES
      INTERFACE_LINK_LIBRARIES "${LLVM_DEBUGINFO_PDB_FIXED_LINK_LIBS}")
    message(STATUS "Using DIA SDK library: ${DIAGUIDS_LIBRARY}")
  else()
    message(WARNING
      "Could not find diaguids.lib. If linking fails, install the Visual Studio "
      "DIA SDK or configure with -DDIAGUIDS_LIBRARY=<path-to-diaguids.lib>.")
  endif()
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if(CMAKE_SIZEOF_VOID_P AND NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(FATAL_ERROR "This pass must be built as x64 to match ${LLVM_INSTALL_DIR}")
endif()

add_definitions(${LLVM_DEFINITIONS})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")

foreach(config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${config} "${PROJECT_BINARY_DIR}/bin/${config}")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${config} "${PROJECT_BINARY_DIR}/bin/${config}")
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${config} "${PROJECT_BINARY_DIR}/lib/${config}")
endforeach()

add_subdirectory(lib)
