
set(SOURCES
    example.cpp
)

function(serenity_shared_lib target_name fs_name)
    add_library(${target_name} SHARED ${SOURCES})
    install(TARGETS ${target_name} LIBRARY DESTINATION usr/lib)
    set_target_properties(${target_name} PROPERTIES OUTPUT_NAME ${fs_name})
endfunction()

serenity_shared_lib(ExampleSO exampleso)
target_link_libraries(ExampleSO)
