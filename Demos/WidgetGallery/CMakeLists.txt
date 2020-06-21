set(SOURCES
    main.cpp
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pie")

serenity_bin(WidgetGallery)
target_link_libraries(WidgetGallery LibGUIShared)
#target_link_libraries(WidgetGallery LibGUI)
