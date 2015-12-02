# Set Include Directories
set(kcmgrub2helper_INCLUDE_DIRS ${KDE4_INCLUDES})
if(HAVE_HD)
    set(kcmgrub2helper_INCLUDE_DIRS ${kcmgrub2helper_INCLUDE_DIRS} ${HD_INCLUDE_DIR})
endif(HAVE_HD)

# Set Sources
set(kcmgrub2helper_SRCS helper.cpp)

# Set Link Libraries
set(kcmgrub2helper_LINK_LIBS ${KDE4_KDECORE_LIBS})
if(HAVE_HD)
    set(kcmgrub2helper_LINK_LIBS ${kcmgrub2helper_LINK_LIBS} ${HD_LIBRARY})
endif(HAVE_HD)

# Build & Link
include_directories(${kcmgrub2helper_INCLUDE_DIRS})
kde4_add_executable(kcmgrub2helper ${kcmgrub2helper_SRCS})
target_link_libraries(kcmgrub2helper ${kcmgrub2helper_LINK_LIBS})

# Install
install(TARGETS kcmgrub2helper DESTINATION ${LIBEXEC_INSTALL_DIR})
kde4_install_auth_helper_files(kcmgrub2helper org.kde.kcontrol.kcmgrub2 root)
kde4_install_auth_actions(org.kde.kcontrol.kcmgrub2 kcmgrub2.actions)
