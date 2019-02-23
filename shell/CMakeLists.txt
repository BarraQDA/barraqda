
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${CMAKE_CURRENT_BINARY_DIR}/../
)

# barraqda

set(barraqda_SRCS
   main.cpp
   okular_main.cpp
   shell.cpp
   shellutils.cpp
)

message("CMAKE_CURRENT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}")
file(GLOB ICONS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/../ui/data/icons/*-apps-barraqda.png")
ecm_add_app_icon(barraqda_SRCS ICONS ${ICONS_SRCS})

add_executable(barraqda ${barraqda_SRCS})

if(NOT BUILD_SHARED_LIBS)
    target_link_libraries(barraqda KF5::Parts KF5::WindowSystem KF5::Crash KF5::Archive okularcore okularpart)
else()
    target_link_libraries(barraqda KF5::Parts KF5::WindowSystem KF5::Crash)
endif()
if(TARGET KF5::Activities)
    target_compile_definitions(barraqda PUBLIC -DWITH_KACTIVITIES=1)

	target_link_libraries(barraqda KF5::Activities)
endif()

install(TARGETS barraqda ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})


# barraqda shell data files

install( PROGRAMS org.kde.barraqda.desktop  DESTINATION  ${KDE_INSTALL_APPDIR} )
install( FILES shell.rc DESTINATION ${KDE_INSTALL_KXMLGUI5DIR}/barraqda )
install( FILES org.kde.barraqda.appdata.xml DESTINATION  ${KDE_INSTALL_METAINFODIR} )
