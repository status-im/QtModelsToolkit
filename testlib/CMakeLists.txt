find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Qml Quick Test)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Qml Quick Test)

add_library(${PROJECT_NAME}TestLib
    src/listmodelwrapper.cpp
    src/modelsignalsspy.cpp
    src/modeltestutils.cpp
    src/persistentindexestester.cpp
    src/testmodel.cpp

    include/qtmodelstoolkit/testing/listmodelwrapper.h
    include/qtmodelstoolkit/testing/listmodelwrapper.h
    include/qtmodelstoolkit/testing/modelsignalsspy.h
    include/qtmodelstoolkit/testing/modeltestutils.h
    include/qtmodelstoolkit/testing/persistentindexestester.h
    include/qtmodelstoolkit/testing/testmodel.h
)

target_link_libraries(${PROJECT_NAME}TestLib PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Qml
    Qt${QT_VERSION_MAJOR}::Quick
    Qt${QT_VERSION_MAJOR}::Test
)

target_include_directories(${PROJECT_NAME}TestLib PUBLIC include)
