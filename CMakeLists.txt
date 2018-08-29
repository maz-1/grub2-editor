cmake_minimum_required( VERSION 2.8.12 )
cmake_policy(SET CMP0002 OLD)
cmake_policy(SET CMP0026 OLD)
cmake_policy(SET CMP0054 OLD)
cmake_policy(SET CMP0063 OLD)
cmake_policy(SET CMP0064 OLD)

project(kcm-grub2)
set(KCM_GRUB2_VERSION_MAJOR "0")
set(KCM_GRUB2_VERSION_MINOR "7")
set(KCM_GRUB2_VERSION_PATCH "0")
set(KCM_GRUB2_VERSION "${KCM_GRUB2_VERSION_MAJOR}.${KCM_GRUB2_VERSION_MINOR}.${KCM_GRUB2_VERSION_PATCH}")
set(KDE_ENABLE_EXCEPTIONS "-fexceptions -UQT_NO_EXCEPTIONS")

find_package(ECM 0.0.11 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)
include(FeatureSummary)

find_package(Qt5 REQUIRED COMPONENTS Widgets DBus)
find_package(KF5 REQUIRED COMPONENTS CoreAddons I18n Auth ConfigWidgets KIO Solid)
#find_package(Qca-qt5 REQUIRED)

include(GRUBPaths)

include(MacroLogFeature)
include(MacroBoolTo01)

add_definitions(-DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS -DMAGICKCORE_QUANTUM_DEPTH=16 -DMAGICKCORE_HDRI_ENABLE=0)

find_package(ImageMagick COMPONENTS Magick++ MagickCore)
macro_log_feature(ImageMagick_FOUND "ImageMagick" "Create splash images compatible with GRUB2" "http://www.imagemagick.org/" FALSE "" "")
macro_bool_to_01(ImageMagick_FOUND HAVE_IMAGEMAGICK)

find_package(HD)
macro_log_feature(HD_FOUND "hwinfo" "Retrieve list of resolutions valid in GRUB2" "http://www.opensuse.org/" FALSE "" "")
macro_bool_to_01(HD_FOUND HAVE_HD)

find_package(QApt 3.0.0)
macro_log_feature(QAPT_FOUND "LibQApt" "Remove unneeded old entries (qapt backend)" "https://projects.kde.org/projects/extragear/sysadmin/libqapt/" FALSE "" "")
macro_bool_to_01(QApt_FOUND HAVE_QAPT)

find_package(PackageKitQt5)
macro_log_feature(QPACKAGEKIT_FOUND "QPackageKit" "Remove unneeded old entries (qpackagekit backend)" "http://www.packagekit.org/" FALSE "" "")
macro_bool_to_01(QPACKAGEKIT_FOUND HAVE_QPACKAGEKIT)

macro_display_feature_log()
configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/src/config.h)

add_subdirectory(other)
add_subdirectory(src)
find_package( Msgfmt REQUIRED )
find_package( Gettext REQUIRED )
add_subdirectory( po )
