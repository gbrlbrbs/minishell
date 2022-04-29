add_library(parser parser.c)
add_library(shell shell.c)
add_library(process process.c)

target_link_libraries(shell parser)
target_link_libraries(parser process)
target_link_libraries(shell process)