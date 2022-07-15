add_library(parser parser.c)
add_library(shell shell.c)

target_link_libraries(shell parser)
#target_link_libraries(parser process)