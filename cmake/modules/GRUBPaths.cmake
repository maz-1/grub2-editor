if(GRUB_INSTALL_EXE AND GRUB_MKCONFIG_EXE AND GRUB_PROBE_EXE AND GRUB_SET_DEFAULT_EXE AND GRUB_MAKE_PASSWD_EXE AND GRUB_MENU AND GRUB_CONFIG AND GRUB_ENV AND GRUB_MEMTEST AND GRUB_CONFIGDIR AND GRUB_SECURITY AND GRUB_RMECHO)
    message(STATUS "All GRUB paths were defined in the CMake cache. By-passing automatic resolution.")
elseif(NOT (GRUB_INSTALL_EXE OR GRUB_MKCONFIG_EXE OR GRUB_PROBE_EXE OR GRUB_SET_DEFAULT_EXE))
    find_program(GRUB_INSTALL_EXE NAMES grub-install DOC "GRUB install executable file path.")
    find_program(GRUB_MKCONFIG_EXE NAMES grub-mkconfig DOC "GRUB mkconfig executable file path.")
    find_program(GRUB_PROBE_EXE NAMES grub-probe DOC "GRUB probe executable file path.")
    find_program(GRUB_SET_DEFAULT_EXE NAMES grub-set-default DOC "GRUB set-default executable file path.")
    find_program(GRUB_MAKE_PASSWD_EXE NAMES grub-mkpasswd-pbkdf2 DOC "GRUB mkpasswd-pbkdf2 executable file path.")
    if(GRUB_INSTALL_EXE AND GRUB_MKCONFIG_EXE AND GRUB_PROBE_EXE AND GRUB_SET_DEFAULT_EXE AND GRUB_MAKE_PASSWD_EXE)
        set(GRUB_MENU "/boot/grub/grub.cfg" CACHE FILEPATH "GRUB menu file path.")
        set(GRUB_CONFIG "/etc/default/grub" CACHE FILEPATH "GRUB configuration file path.")
        set(GRUB_ENV "/boot/grub/grubenv" CACHE FILEPATH "GRUB environment file path.")
        set(GRUB_MEMTEST "/etc/grub.d/60_memtest86+" CACHE FILEPATH "GRUB memtest file path.")
        set(GRUB_CONFIGDIR "/etc/grub.d/" CACHE FILEPATH "GRUB configuration path.")
        set(GRUB_SECURITY "01_header_passwd" CACHE FILEPATH "GRUB security configuration file.")
        set(GRUB_RMECHO "99_rmecho" CACHE FILEPATH "GRUB script to remove echo commands.")
    else(GRUB_INSTALL_EXE AND GRUB_MKCONFIG_EXE AND GRUB_PROBE_EXE AND GRUB_SET_DEFAULT_EXE AND GRUB_MAKE_PASSWD_EXE)
        unset(GRUB_INSTALL_EXE CACHE)
        unset(GRUB_MKCONFIG_EXE CACHE)
        unset(GRUB_PROBE_EXE CACHE)
        unset(GRUB_SET_DEFAULT_EXE CACHE)
        unset(GRUB_MAKE_PASSWD_EXE CACHE)
        find_program(GRUB_INSTALL_EXE NAMES grub2-install DOC "GRUB install executable file path.")
        find_program(GRUB_MKCONFIG_EXE NAMES grub2-mkconfig DOC "GRUB mkconfig executable file path.")
        find_program(GRUB_PROBE_EXE NAMES grub2-probe DOC "GRUB probe executable file path.")
        find_program(GRUB_SET_DEFAULT_EXE NAMES grub2-set-default DOC "GRUB set-default executable file path.")
        find_program(GRUB_MAKE_PASSWD_EXE NAMES grub-mkpasswd-pbkdf2 DOC "GRUB mkpasswd-pbkdf2 executable file path.")
        if(GRUB_INSTALL_EXE AND GRUB_MKCONFIG_EXE AND GRUB_PROBE_EXE AND GRUB_SET_DEFAULT_EXE AND GRUB_MAKE_PASSWD_EXE)
            set(GRUB_MENU "/boot/grub2/grub.cfg" CACHE FILEPATH "GRUB menu file path.")
            set(GRUB_CONFIG "/etc/default/grub" CACHE FILEPATH "GRUB configuration file path.")
            set(GRUB_ENV "/boot/grub2/grubenv" CACHE FILEPATH "GRUB environment file path.")
            set(GRUB_MEMTEST "/etc/grub.d/60_memtest86+" CACHE FILEPATH "GRUB memtest file path.")
            set(GRUB_CONFIGDIR "/etc/grub.d/" CACHE FILEPATH "GRUB configuration path.")
            set(GRUB_SECURITY "01_header_passwd" CACHE FILEPATH "GRUB security configuration file.")
            set(GRUB_RMECHO "99_rmecho" CACHE FILEPATH "GRUB script to remove echo commands.")
        else(GRUB_INSTALL_EXE AND GRUB_MKCONFIG_EXE AND GRUB_PROBE_EXE AND GRUB_SET_DEFAULT_EXE AND GRUB_MAKE_PASSWD_EXE)
            unset(GRUB_INSTALL_EXE CACHE)
            unset(GRUB_MKCONFIG_EXE CACHE)
            unset(GRUB_PROBE_EXE CACHE)
            unset(GRUB_SET_DEFAULT_EXE CACHE)
            unset(GRUB_MAKE_PASSWD_EXE CACHE)
            find_program(GRUB_INSTALL_EXE NAMES burg-install DOC "GRUB install executable file path.")
            find_program(GRUB_MKCONFIG_EXE NAMES burg-mkconfig DOC "GRUB mkconfig executable file path.")
            find_program(GRUB_PROBE_EXE NAMES burg-probe DOC "GRUB probe executable file path.")
            find_program(GRUB_SET_DEFAULT_EXE NAMES burg-set-default DOC "GRUB set-default executable file path.")
            find_program(GRUB_MAKE_PASSWD_EXE NAMES grub-mkpasswd-pbkdf2 DOC "GRUB mkpasswd-pbkdf2 executable file path.")
            if(GRUB_INSTALL_EXE AND GRUB_MKCONFIG_EXE AND GRUB_PROBE_EXE AND GRUB_SET_DEFAULT_EXE AND GRUB_MAKE_PASSWD_EXE)
                set(GRUB_MENU "/boot/burg/burg.cfg" CACHE FILEPATH "GRUB menu file path.")
                set(GRUB_CONFIG "/etc/default/burg" CACHE FILEPATH "GRUB configuration file path.")
                set(GRUB_ENV "/boot/burg/burgenv" CACHE FILEPATH "GRUB environment file path.")
                set(GRUB_MEMTEST "/etc/burg.d/20_memtest86+" CACHE FILEPATH "GRUB memtest file path.")
                set(GRUB_CONFIGDIR "/etc/grub.d/" CACHE FILEPATH "GRUB configuration path.")
                set(GRUB_SECURITY "01_header_passwd" CACHE FILEPATH "GRUB security configuration file.")
                set(GRUB_RMECHO "99_rmecho" CACHE FILEPATH "GRUB script to remove echo commands.")
            else(GRUB_INSTALL_EXE AND GRUB_MKCONFIG_EXE AND GRUB_PROBE_EXE AND GRUB_SET_DEFAULT_EXE AND GRUB_MAKE_PASSWD_EXE)
                message(FATAL_ERROR "Could not automatically resolve GRUB paths. Please specify all of them manually.")
            endif(GRUB_INSTALL_EXE AND GRUB_MKCONFIG_EXE AND GRUB_PROBE_EXE AND GRUB_SET_DEFAULT_EXE AND GRUB_MAKE_PASSWD_EXE)
        endif(GRUB_INSTALL_EXE AND GRUB_MKCONFIG_EXE AND GRUB_PROBE_EXE AND GRUB_SET_DEFAULT_EXE AND GRUB_MAKE_PASSWD_EXE)
    endif(GRUB_INSTALL_EXE AND GRUB_MKCONFIG_EXE AND GRUB_PROBE_EXE AND GRUB_SET_DEFAULT_EXE AND GRUB_MAKE_PASSWD_EXE)
else(GRUB_INSTALL_EXE AND GRUB_MKCONFIG_EXE AND GRUB_PROBE_EXE AND GRUB_SET_DEFAULT_EXE AND GRUB_MAKE_PASSWD_EXE AND GRUB_MENU AND GRUB_CONFIG AND GRUB_ENV AND GRUB_MEMTEST AND GRUB_CONFIGDIR AND GRUB_SECURITY AND GRUB_RMECHO)
    message(FATAL_ERROR "Some, but not all, GRUB paths were defined in the CMake cache. Please define them all or let CMake do it. In the latter case define none.")
endif(GRUB_INSTALL_EXE AND GRUB_MKCONFIG_EXE AND GRUB_PROBE_EXE AND GRUB_SET_DEFAULT_EXE AND GRUB_MAKE_PASSWD_EXE AND GRUB_MENU AND GRUB_CONFIG AND GRUB_ENV AND GRUB_MEMTEST AND GRUB_CONFIGDIR AND GRUB_SECURITY AND GRUB_RMECHO)

message(STATUS "--------------------------------------------------------------------------")
message(STATUS "GRUB_INSTALL_EXE: ${GRUB_INSTALL_EXE}")
message(STATUS "GRUB_MKCONFIG_EXE: ${GRUB_MKCONFIG_EXE}")
message(STATUS "GRUB_PROBE_EXE: ${GRUB_PROBE_EXE}")
message(STATUS "GRUB_SET_DEFAULT_EXE: ${GRUB_SET_DEFAULT_EXE}")
message(STATUS "GRUB_MAKE_PASSWD_EXE: ${GRUB_MAKE_PASSWD_EXE}")
message(STATUS "GRUB_MENU: ${GRUB_MENU}")
message(STATUS "GRUB_CONFIG: ${GRUB_CONFIG}")
message(STATUS "GRUB_ENV: ${GRUB_ENV}")
message(STATUS "GRUB_MEMTEST: ${GRUB_MEMTEST}")
message(STATUS "GRUB_CONFIGDIR: ${GRUB_CONFIGDIR}")
message(STATUS "GRUB_SECURITY: ${GRUB_SECURITY}")
message(STATUS "GRUB_RMECHO: ${GRUB_RMECHO}")
message(STATUS "--------------------------------------------------------------------------")
