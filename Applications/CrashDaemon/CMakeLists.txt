set(SOURCES
    main.cpp
    Backtrace.cpp
    CoreDumpReader.cpp
)

serenity_bin(CrashDaemon)
target_link_libraries(CrashDaemon LibC LibCore)
