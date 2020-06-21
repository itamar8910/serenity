set(SOURCES
    BackgroundAction.cpp
    Thread.cpp
)

serenity_lib(LibThread thread)
target_link_libraries(LibThread LibC LibCore LibPthread)

serenity_shared_lib(LibThreadShared thread)
target_link_libraries(LibThreadShared LibCShared LibCoreShared LibPthread)
