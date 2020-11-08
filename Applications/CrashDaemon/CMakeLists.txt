set(SOURCES
    main.cpp
    CoreDumpReader.cpp
)

serenity_bin(CrashDaemon)
target_link_libraries(CrashDaemon LibC LibCore)
