set(LLVM_CS4277_P1 InstrumentFunctions)

set(InstrumentFunctions_SOURCES InstrumentFunctions.cpp)

llvm_map_components_to_libnames(LLVM_PASS_LIBS
  Core
  Support
  TransformUtils
)

foreach(plugin ${LLVM_CS4277_P1})
  add_library(${plugin} MODULE ${${plugin}_SOURCES})

  target_include_directories(${plugin}
    PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/../include"
      ${LLVM_INCLUDE_DIRS}
  )

  target_link_directories(${plugin} PRIVATE ${LLVM_LIBRARY_DIRS})

  target_link_libraries(${plugin}
    PRIVATE
      ${LLVM_PASS_LIBS}
  )

  if(MSVC)
    target_compile_options(${plugin} PRIVATE /EHsc /permissive- /Zc:__cplusplus)
    target_link_options(${plugin} PRIVATE /EXPORT:llvmGetPassPluginInfo)
    if(NOT LLVM_ENABLE_RTTI)
      target_compile_options(${plugin} PRIVATE /GR-)
    endif()
  else()
    target_compile_options(${plugin} PRIVATE -Wall -Wextra)
    if(NOT LLVM_ENABLE_RTTI)
      target_compile_options(${plugin} PRIVATE -fno-rtti)
    endif()
  endif()

  set_target_properties(${plugin}
    PROPERTIES
      PREFIX ""
      CXX_VISIBILITY_PRESET hidden
      VISIBILITY_INLINES_HIDDEN ON
  )
endforeach()
