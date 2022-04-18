add_executable(minishell main.c)
include_directories(${minishell_SOURCE_DIR}/src/lib)
add_subdirectory(lib)
target_link_libraries(minishell process)