add_library(execution
    aggregate.cpp
    expression.cpp
    operator.cpp
)

find_package(PkgConfig REQUIRED)
find_package(absl REQUIRED)
pkg_check_modules(RE2 REQUIRED re2)

target_include_directories(execution PUBLIC ${RE2_INCLUDE_DIRS})

target_include_directories(execution PUBLIC
    ${PROJECT_SOURCE_DIR}
)

target_link_libraries(execution PUBLIC 
    io
    storage
    ${RE2_LIBRARIES}
    absl::flat_hash_map
    absl::flat_hash_set
)

target_compile_features(execution PRIVATE
    cxx_std_23
)
