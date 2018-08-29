# Install
install(FILES kcm_grub2.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES grub2-editor.svg DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/apps)
install(FILES kcm-grub2-languages DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${CMAKE_PROJECT_NAME}/config RENAME languages)
