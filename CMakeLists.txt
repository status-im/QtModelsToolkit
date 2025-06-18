cmake_minimum_required(VERSION 3.19)

project(QtModelsToolkit VERSION 1.0 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

option(QTMODELSTOOLKIT_BUILD_TOOLKIT "Build toolkit library" ON)
option(QTMODELSTOOLKIT_BUILD_DEMO_APP "Build demo application" OFF)
option(QTMODELSTOOLKIT_BUILD_TESTLIB "Build testing library" OFF)
option(QTMODELSTOOLKIT_BUILD_TESTS "Build tests" OFF)

if(QTMODELSTOOLKIT_BUILD_TOOLKIT
    OR QTMODELSTOOLKIT_BUILD_DEMO_APP
    OR QTMODELSTOOLKIT_BUILD_TESTS)
  add_subdirectory(toolkit)
endif()

if(QTMODELSTOOLKIT_BUILD_DEMO_APP)
  add_subdirectory(demoapp)
endif()

if(QTMODELSTOOLKIT_BUILD_TESTLIB
    OR QTMODELSTOOLKIT_BUILD_TESTS)
  add_subdirectory(testlib)
endif()

if(QTMODELSTOOLKIT_BUILD_TESTS)
  add_subdirectory(tests)
endif()

find_program(QDOC_EXECUTABLE NAMES qdoc)

if(QDOC_EXECUTABLE)
  set(QDOC_CONF "${CMAKE_CURRENT_SOURCE_DIR}/qtmodelstoolkit.qdocconf")

  add_custom_target(qdoc
      COMMAND ${QDOC_EXECUTABLE} ${QDOC_CONF}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      COMMENT "Generating documentation with QDoc"
      VERBATIM
  )
else()
  message(STATUS "QDoc not found!")
endif()

add_custom_target(Other ALL SOURCES
  README.md
  toolkit/src/index.qdoc docs-styling.css qtmodelstoolkit.qdocconf
  .github/workflows/build-and-test-linux.yml
  .github/workflows/build-and-test-windows.yml
)
