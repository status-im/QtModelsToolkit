set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Quick)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Quick)

add_executable(demo-app main.cpp qml.qrc)

include(FetchContent)

FetchContent_Declare(
  SortFilterProxyModel
  GIT_REPOSITORY https://github.com/status-im/SortFilterProxyModel.git
  GIT_TAG master
)

FetchContent_MakeAvailable(SortFilterProxyModel)
target_compile_options(SortFilterProxyModel PRIVATE -w)

target_link_libraries(demo-app
  PRIVATE ${PROJECT_NAME} Qt${QT_VERSION_MAJOR}::Quick SortFilterProxyModel)
