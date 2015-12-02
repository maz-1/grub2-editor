project(kcm-grub2)
set(KCM_GRUB2_VERSION_MAJOR "0")
set(KCM_GRUB2_VERSION_MINOR "5")
set(KCM_GRUB2_VERSION_PATCH "8")
set(KCM_GRUB2_VERSION "${KCM_GRUB2_VERSION_MAJOR}.${KCM_GRUB2_VERSION_MINOR}.${KCM_GRUB2_VERSION_PATCH}")

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})

find_package(Qt4 4.6.0 REQUIRED)
find_package(KDE4 4.4.0 REQUIRED)
include(KDE4Defaults)
include(GRUBPaths)

add_definitions(-DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS -DMAGICKCORE_QUANTUM_DEPTH=16 -DMAGICKCORE_HDRI_ENABLE=0)

macro_optional_find_package(ImageMagick COMPONENTS Magick++ MagickCore)
macro_log_feature(ImageMagick_FOUND "ImageMagick" "Create splash images compatible with GRUB2" "http://www.imagemagick.org/" FALSE "" "")
macro_bool_to_01(ImageMagick_FOUND HAVE_IMAGEMAGICK)

macro_optional_find_package(HD)
macro_log_feature(HD_FOUND "hwinfo" "Retrieve list of resolutions valid in GRUB2" "http://www.opensuse.org/" FALSE "" "")
macro_bool_to_01(HD_FOUND HAVE_HD)

macro_optional_find_package(QApt)
macro_log_feature(QAPT_FOUND "LibQApt" "Remove unneeded old entries (qapt backend)" "https://projects.kde.org/projects/extragear/sysadmin/libqapt/" FALSE "" "")
macro_bool_to_01(QAPT_FOUND HAVE_QAPT)

macro_optional_find_package(QPackageKit)
macro_log_feature(QPACKAGEKIT_FOUND "QPackageKit" "Remove unneeded old entries (qpackagekit backend)" "http://www.packagekit.org/" FALSE "" "")
macro_bool_to_01(QPACKAGEKIT_FOUND HAVE_QPACKAGEKIT)

macro_display_feature_log()
configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/src/config.h)

add_subdirectory(other)
add_subdirectory(src)
find_package( Msgfmt REQUIRED )
find_package( Gettext REQUIRED )
add_subdirectory( po )
