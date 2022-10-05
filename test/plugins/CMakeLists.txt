add_library(module_t MODULE module_t.cpp)
target_link_libraries(module_t PRIVATE meld::core)

add_library(source_t MODULE source_t.cpp)
target_link_libraries(source_t PRIVATE meld::core)


add_test(NAME job:add_t
         COMMAND meld -c ${CMAKE_CURRENT_SOURCE_DIR}/add.json)
