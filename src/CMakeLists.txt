add_subdirectory(helper)

# Set Include Directories
# set(kcm_grub2_INCLUDE_DIRS ${KDE4_INCLUDES})
if(HAVE_IMAGEMAGICK)
    set(kcm_grub2_INCLUDE_DIRS ${kcm_grub2_INCLUDE_DIRS} ${ImageMagick_INCLUDE_DIRS})
endif(HAVE_IMAGEMAGICK)
if(HAVE_QAPT)
    set(kcm_grub2_INCLUDE_DIRS ${kcm_grub2_INCLUDE_DIRS} ${QAPT_INCLUDE_DIR})
elseif(HAVE_QPACKAGEKIT)
    set(kcm_grub2_INCLUDE_DIRS ${kcm_grub2_INCLUDE_DIRS} ${QPACKAGEKIT_INCLUDE_DIR})
endif(HAVE_QAPT)

# Set Sources
set(kcm_grub2_SRCS common.cpp entry.cpp installDlg.cpp kcm_grub2.cpp widgets/regexpinputdialog.cpp)
ki18n_wrap_ui(kcm_grub2_SRCS ../ui/installDlg.ui ../ui/groupDlg.ui ../ui/userDlg.ui ../ui/kcm_grub2.ui)
if(HAVE_IMAGEMAGICK)
    set(kcm_grub2_SRCS ${kcm_grub2_SRCS} convertDlg.cpp)
    ki18n_wrap_ui(kcm_grub2_SRCS ../ui/convertDlg.ui)
endif(HAVE_IMAGEMAGICK)
if(HAVE_QAPT OR HAVE_QPACKAGEKIT)
    set(kcm_grub2_SRCS ${kcm_grub2_SRCS} removeDlg.cpp)
    ki18n_wrap_ui(kcm_grub2_SRCS ../ui/removeDlg.ui)
endif(HAVE_QAPT OR HAVE_QPACKAGEKIT)
if(HAVE_QAPT)
    if(QAPT_VERSION_MAJOR EQUAL 1)
        set(kcm_grub2_SRCS ${kcm_grub2_SRCS} qaptBackend.cpp)
    elseif(QAPT_VERSION_MAJOR EQUAL 2)
        set(kcm_grub2_SRCS ${kcm_grub2_SRCS} qapt2Backend.cpp)
    endif(QAPT_VERSION_MAJOR EQUAL 1)
elseif(HAVE_QPACKAGEKIT)
    set(kcm_grub2_SRCS ${kcm_grub2_SRCS} qPkBackend.cpp)
endif(HAVE_QAPT)

# Set Link Libraries
set(kcm_grub2_LINK_LIBS
     Qt5::Widgets
     KF5::ConfigWidgets
     KF5::KIOWidgets
     KF5::I18n
     KF5::Auth
     KF5::Solid
     #qca-qt5
     )
if(HAVE_IMAGEMAGICK)
    set(kcm_grub2_LINK_LIBS ${kcm_grub2_LINK_LIBS} ${ImageMagick_LIBRARIES})
endif(HAVE_IMAGEMAGICK)
if(HAVE_QAPT)
    set(kcm_grub2_LINK_LIBS ${kcm_grub2_LINK_LIBS} ${QAPT_LIBRARY})
elseif(HAVE_QPACKAGEKIT)
    set(kcm_grub2_LINK_LIBS ${kcm_grub2_LINK_LIBS} ${QPACKAGEKIT_LIB})
endif(HAVE_QAPT)

# Definitions
if(HAVE_IMAGEMAGICK)
    add_definitions(${KDE_ENABLE_EXCEPTIONS})
endif(HAVE_IMAGEMAGICK)

# Build & Link
include_directories(${kcm_grub2_INCLUDE_DIRS})
add_library(kcm_grub2 MODULE ${kcm_grub2_SRCS})
target_link_libraries(kcm_grub2 ${kcm_grub2_LINK_LIBS})

# Install
install(TARGETS kcm_grub2 DESTINATION ${PLUGIN_INSTALL_DIR})
