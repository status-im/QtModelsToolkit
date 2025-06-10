find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Qml Test)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Qml Test)

set(SOURCES
  src/aggregator.cpp
  src/concatmodel.cpp
  src/functionaggregator.cpp
  src/groupingmodel.cpp
  src/leftjoinmodel.cpp
  src/modelcount.cpp
  src/modelentry.cpp
  src/modelquery.cpp
  src/movablemodel.cpp
  src/objectproxymodel.cpp
  src/registerqmltypes.cpp
  src/rolesrenamingmodel.cpp
  src/singleroleaggregator.cpp
  src/snapshotmodel.cpp
  src/snapshotobject.cpp
  src/sumaggregator.cpp
  src/writableproxymodel.cpp
)

set(HEADERS
  include/qtmodelstoolkit/aggregator.h
  include/qtmodelstoolkit/concatmodel.h
  include/qtmodelstoolkit/functionaggregator.h
  include/qtmodelstoolkit/groupingmodel.h
  include/qtmodelstoolkit/leftjoinmodel.h
  include/qtmodelstoolkit/modelcount.h
  include/qtmodelstoolkit/modelentry.h
  include/qtmodelstoolkit/modelsyncedcontainer.h
  include/qtmodelstoolkit/modelquery.h
  include/qtmodelstoolkit/movablemodel.h
  include/qtmodelstoolkit/objectproxymodel.h
  include/qtmodelstoolkit/registerqmltypes.h
  include/qtmodelstoolkit/rolesrenamingmodel.h
  include/qtmodelstoolkit/singleroleaggregator.h
  include/qtmodelstoolkit/snapshotmodel.h
  include/qtmodelstoolkit/snapshotobject.h
  include/qtmodelstoolkit/sumaggregator.h
  include/qtmodelstoolkit/writableproxymodel.h
)

add_library(${PROJECT_NAME} OBJECT ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  Qt${QT_VERSION_MAJOR}::Core
  Qt${QT_VERSION_MAJOR}::Qml
  $<$<CONFIG:Debug>:Qt${QT_VERSION_MAJOR}::Test>
)
