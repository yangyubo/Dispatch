aux_source_directory(./ DISPATCH_TESTS)
add_executable(DispatchTests ${DISPATCH_TESTS})

#find_library(CORE_FOUNDATION CoreFoundation)
#if (NOT CORE_FOUNDATION)
#    message(FATAL_ERROR "CoreFoundation not found")
#endif()

target_link_libraries(DispatchTests Dispatch "-framework CoreFoundation")

