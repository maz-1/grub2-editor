#ifndef CONFIG_H
#define CONFIG_H

#define KCM_GRUB2_VERSION "@KCM_GRUB2_VERSION@"

#define HAVE_IMAGEMAGICK @HAVE_IMAGEMAGICK@
#define HAVE_HD @HAVE_HD@
#define HAVE_QAPT @HAVE_QAPT@
#define HAVE_QPACKAGEKIT @HAVE_QPACKAGEKIT@

#define GRUB_INSTALL_EXE "@GRUB_INSTALL_EXE@"
#define GRUB_MKCONFIG_EXE "@GRUB_MKCONFIG_EXE@"
#define GRUB_PROBE_EXE "@GRUB_PROBE_EXE@"
#define GRUB_SET_DEFAULT_EXE "@GRUB_SET_DEFAULT_EXE@"
#define GRUB_MAKE_PASSWD_EXE "@GRUB_MAKE_PASSWD_EXE@"
#define GRUB_MENU "@GRUB_MENU@"
#define GRUB_CONFIG "@GRUB_CONFIG@"
#define GRUB_ENV "@GRUB_ENV@"
#define GRUB_MEMTEST "@GRUB_MEMTEST@"
#define GRUB_CONFIGDIR "@GRUB_CONFIGDIR@/"
#define GRUB_SECURITY "@GRUB_SECURITY@"
#define GRUB_RMECHO "@GRUB_RMECHO@"

enum actionType {
    actionLoad,
    actionProbe,
    actionProbevbe
};

enum GrubFile {
    GrubMenuFile,
    GrubConfigurationFile,
    GrubEnvironmentFile,
    GrubMemtestFile,
    GrubGroupFile
};

#endif
