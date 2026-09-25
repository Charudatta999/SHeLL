# Install rules + CPack (deb / rpm / tar.gz) for the shell-only build.
# The GUI target is not packaged: its deps are vendored and not distro-provided.

include(GNUInstallDirs)

install(TARGETS shellrepl RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES LICENSE README.md DESTINATION ${CMAKE_INSTALL_DOCDIR})

set(CPACK_PACKAGE_NAME "shell-repl")
set(CPACK_PACKAGE_VENDOR "Charudatta Jadhav")
set(CPACK_PACKAGE_CONTACT "Charudatta Jadhav <cjawsome999@gmail.com>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/Charudatta999/SHeLL")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
# CI overrides this (SHELL_PACKAGE_VERSION) so non-tag artifacts are traceable.
if(SHELL_PACKAGE_VERSION)
    set(CPACK_PACKAGE_VERSION "${SHELL_PACKAGE_VERSION}")
else()
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
endif()

set(CPACK_GENERATOR "TGZ;DEB;RPM")
set(CPACK_ARCHIVE_FILE_NAME "shell-repl-${CPACK_PACKAGE_VERSION}-linux-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "shell-repl-${PROJECT_VERSION}-src")
set(CPACK_SOURCE_IGNORE_FILES "/\\\\.git/;/build[^/]*/;/third_party/Library-sources/")

set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_SECTION "shells")

set(CPACK_RPM_PACKAGE_LICENSE "MIT")
set(CPACK_RPM_PACKAGE_GROUP "System Environment/Shells")
set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
set(CPACK_RPM_PACKAGE_AUTOREQ ON)

include(CPack)
