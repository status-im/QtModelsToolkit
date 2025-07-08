find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Qml)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Qml)

include(FetchContent)

FetchContent_Declare(
  SortFilterProxyModel
  GIT_REPOSITORY https://github.com/status-im/SortFilterProxyModel.git
  GIT_TAG 6477b1e465b02f18007900125864cdfa812ba8ad
)

FetchContent_MakeAvailable(SortFilterProxyModel)
target_compile_options(SortFilterProxyModel PRIVATE -w)

enable_testing()

add_executable(ModelQueryTest tst_ModelQuery.cpp)
target_link_libraries(ModelQueryTest PRIVATE ${PROJECT_NAME} ${PROJECT_NAME}TestLib)
add_test(NAME ModelQueryTest COMMAND ModelQueryTest)

add_executable(RolesRenamingModelTest tst_RolesRenamingModel.cpp)
target_link_libraries(RolesRenamingModelTest ${PROJECT_NAME} ${PROJECT_NAME}TestLib)
add_test(NAME RolesRenamingModelTest COMMAND RolesRenamingModelTest)

add_executable(LeftJoinModelTest tst_LeftJoinModel.cpp)
target_link_libraries(LeftJoinModelTest ${PROJECT_NAME} ${PROJECT_NAME}TestLib)
add_test(NAME LeftJoinModelTest COMMAND LeftJoinModelTest)

add_executable(ObjectProxyModelTest tst_ObjectProxyModel.cpp)
target_link_libraries(ObjectProxyModelTest ${PROJECT_NAME} ${PROJECT_NAME}TestLib)
add_test(NAME ObjectProxyModelTest COMMAND ObjectProxyModelTest)

add_executable(AggregatorTest tst_Aggregator.cpp)
target_link_libraries(AggregatorTest ${PROJECT_NAME} ${PROJECT_NAME}TestLib)
add_test(NAME AggregatorTest COMMAND AggregatorTest)

add_executable(SingleRoleAggregatorTest tst_SingleRoleAggregator.cpp)
target_link_libraries(SingleRoleAggregatorTest ${PROJECT_NAME} ${PROJECT_NAME}TestLib)
add_test(NAME SingleRoleAggregatorTest COMMAND SingleRoleAggregatorTest)

add_executable(SumAggregatorTest tst_SumAggregator.cpp)
target_link_libraries(SumAggregatorTest ${PROJECT_NAME} ${PROJECT_NAME}TestLib)
add_test(NAME SumAggregatorTest COMMAND SumAggregatorTest)

add_executable(FunctionAggregatorTest tst_FunctionAggregator.cpp)
target_link_libraries(FunctionAggregatorTest ${PROJECT_NAME} ${PROJECT_NAME}TestLib)
add_test(NAME FunctionAggregatorTest COMMAND FunctionAggregatorTest)

add_executable(ConcatModelTest tst_ConcatModel.cpp)
target_link_libraries(ConcatModelTest ${PROJECT_NAME} ${PROJECT_NAME}TestLib SortFilterProxyModel)
add_test(NAME ConcatModelTest COMMAND ConcatModelTest)

add_executable(WritableProxyModelTest tst_WritableProxyModel.cpp)
target_link_libraries(WritableProxyModelTest ${PROJECT_NAME} ${PROJECT_NAME}TestLib)
add_test(NAME WritableProxyModelTest COMMAND WritableProxyModelTest)

add_executable(MovableModelTest tst_MovableModel.cpp)
target_link_libraries(MovableModelTest ${PROJECT_NAME} ${PROJECT_NAME}TestLib)
add_test(NAME MovableModelTest COMMAND MovableModelTest)

add_executable(ModelSyncedContainerTest tst_ModelSyncedContainer.cpp)
target_link_libraries(ModelSyncedContainerTest ${PROJECT_NAME} ${PROJECT_NAME}TestLib)
add_test(NAME ModelSyncedContainerTest COMMAND ModelSyncedContainerTest)

add_executable(ModelEntryTest tst_ModelEntry.cpp)
target_link_libraries(ModelEntryTest ${PROJECT_NAME} ${PROJECT_NAME}TestLib)
add_test(NAME ModelEntryTest COMMAND ModelEntryTest)

add_executable(SnapshotObjectTest tst_SnapshotObject.cpp)
target_link_libraries(SnapshotObjectTest ${PROJECT_NAME} ${PROJECT_NAME}TestLib)
add_test(NAME SnapshotObjectTest COMMAND SnapshotObjectTest)

add_executable(GroupingModelTest tst_GroupingModel.cpp)
target_link_libraries(GroupingModelTest ${PROJECT_NAME} ${PROJECT_NAME}TestLib)
add_test(NAME GroupingModelTest COMMAND GroupingModelTest)

add_executable(ModelCountTest tst_ModelCount.cpp)
target_link_libraries(ModelCountTest ${PROJECT_NAME} ${PROJECT_NAME}TestLib)
add_test(NAME ModelCountTest COMMAND ModelCountTest)
