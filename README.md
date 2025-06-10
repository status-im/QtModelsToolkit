# QtModelsToolkit

**QtModelsToolkit** is a set of proxy models and utilities for working with Qt's
`QAbstractItemModel` and its derivatives, designed to make model manipulation
and integration easier, especially in QML applications.

It allows to manipulate data in a simple, declarative way without the need to
create custom models in C++.

The library was originally created as part of the
[Status Desktop](https://github.com/status-im/status-desktop) project, then
split off into a separate repository.

Online documentation can be found here: https://status-im.github.io/QtModelsToolkit/

## Features

### Proxy models

- **ConcatModel**
  A proxy model that allows concatenating multiple source models by rows.
  The model performs all the necessary roles mapping internally.

- **LeftJoinModel**
  Supports joining two models, combining their data based on value of the role
  specified as a joining role. Works like a dynamic LEFT JOIN from SQL.
    
- **MovableModel**
  A proxy model that allows rows to be reordered without modifying the source
  model.
  
- **GroupingModel**
  A proxy model that groups items from a source model based on value of role
  specified as grouping role.
  
- **ObjectProxyModel**
  Swiss Army knife of proxy models. It allows to create new roles and modify
  existing ones using a delegate defined in QML.

- **WritableProxyModel**
  A proxy model that allows editing and tracking of changes independently from
  the source model in a very efficient way. Also allows to delete rows.
  
- **RolesRenamingModel**
  A proxy model that allows easily rename model roles.


### Utilities

- **ModelEntry**
  A component representing a single model's entry in a fully dynamic way.
  Additionally it can be configured to cache the entry when removed from the
  source model.
  
- **ModelQuery**
  Utility functions to inspect and query models, e.g. retrieving row data or
  role names.

- **SnapshotModel**
  A model that takes a snapshot of another model's state for later inspection
  or display.
  
- **Aggregators**
  A set of components that allows to calculate the value from the entire model
  in a dynamic way, reacting to changes in the model.
  

## Getting Started

### Requirements

- Qt 5.15 or Qt 6 (Core and QML modules)
- C++17 or newer
- CMake 3.24+

### Building

To build stand-alone (to run the demo app or for development), clone the
repository and use CMake to build:

```bash
git clone https://github.com/status-im/QtModelsToolkit.git
cd QtModelsToolkit
mkdir build && cd build
cmake .. -DQTMODELSTOOLKIT_BUILD_DEMO_APP=ON -DQTMODELSTOOLKIT_BUILD_TESTS=ON
cmake --build . -j
```

Build flags:
- `QTMODELSTOOLKIT_BUILD_TOOLKIT`: Build the toolkit (default ON)
- `QTMODELSTOOLKIT_BUILD_DEMO_APP`: Build demo application (default OFF)
- `QTMODELSTOOLKIT_BUILD_TESTLIB`: Build testing utilities library (default OFF)
- `QTMODELSTOOLKIT_BUILD_TESTS`: Build tests (default OFF)

#### Running demo app
```bash
./demoapp/demo-app
```

#### Running tests
```bash
ctest --output-on-failure --test-dir tests
```

#### Generating docs
```bash
mkdir build && cd build
cmake ..
cmake --build . --target qdoc
```


### Integration

The simplest integration method is to use `FetchContent`:

```cmake
include(FetchContent)
FetchContent_Declare(
  QtModelsToolkit
  GIT_REPOSITORY https://github.com/status-im/QtModelsToolkit.git
  GIT_TAG master
)

FetchContent_MakeAvailable(QtModelsToolkit)

target_link_libraries(<YourTarget> QtModelsToolkit)
```

Alternatively the library can be integrated using git submodules:
```cmake
# QtModelsToolkit fetched as a submodule to vendor directory
add_subdirectory(vendor/QtModelsToolkit)
target_link_libraries(<YourTarget> QtModelsToolkit)
```

Sample project is available here: https://github.com/status-im/QtModelsToolkit-Example

## License

Mozilla Public License Version 2.0. See [LICENSE](LICENSE) for details.
