add_library(process process.c)
add_library(job job.c)
add_library(shell shell.c)

target_link_libraries(job process)
target_link_libraries(shell process)
target_link_libraries(shell job)