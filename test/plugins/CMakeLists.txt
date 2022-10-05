add_library(module_t SHARED module_t.cpp)
target_link_libraries(module_t PRIVATE meld::core)

add_library(source_t SHARED source_t.cpp)
target_link_libraries(source_t PRIVATE meld::core)
