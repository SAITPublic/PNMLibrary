set(exported_targets pnm sls_secure)

foreach(target ${exported_targets})
  target_include_directories(${target} INTERFACE 
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
endforeach()

include(GNUInstallDirs)

install(TARGETS ${exported_targets} DESTINATION ${CMAKE_INSTALL_LIBDIR} EXPORT PNMLibTargets)

export(EXPORT PNMLibTargets
  FILE "${CMAKE_BINARY_DIR}/cmake/PNMLibTargets.cmake"
  NAMESPACE PNM::
)

configure_file(${CMAKE_CURRENT_LIST_DIR}/PNMLibConfig.cmake
  "${CMAKE_BINARY_DIR}/cmake/PNMLibConfig.cmake"
  COPYONLY
)

install(EXPORT PNMLibTargets
  FILE PNMLibTargets.cmake
  NAMESPACE PNM::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/PNMLib
)

install(FILES ${CMAKE_CURRENT_LIST_DIR}/PNMLibConfig.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/PNMLib)

install(DIRECTORY include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
