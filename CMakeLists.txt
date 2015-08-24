cmake_minimum_required(VERSION 2.8)

project(TickleMonster C)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -g -std=c99")
if(APPLE)
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fnested-functions")
  if(CMAKE_INSTALL_PREFIX MATCHES "/usr/local")
    set(CMAKE_INSTALL_PREFIX "/Applications")
    set(BIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}")
  endif(CMAKE_INSTALL_PREFIX MATCHES "/usr/local")
endif(APPLE)
SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include("cmake/SetPaths.cmake")

if(MINGW)
    # Guess MINGDIR from the value of CMAKE_C_COMPILER if it's not set.
    if("$ENV{MINGDIR}" STREQUAL "")
        string(REGEX REPLACE "/bin/[^/]*$" "" MINGDIR "${CMAKE_C_COMPILER}")
        message(STATUS "Guessed MinGW directory: ${MINGDIR}")
    else("$ENV{MINGDIR}" STREQUAL "")
        file(TO_CMAKE_PATH "$ENV{MINGDIR}" MINGDIR)
        message(STATUS "Using MINGDIR: ${MINGDIR}")
    endif("$ENV{MINGDIR}" STREQUAL "")

    # Search in MINGDIR for headers and libraries.
    set(CMAKE_PREFIX_PATH "${MINGDIR}")

    set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -mwindows")

endif(MINGW)

add_subdirectory(src)
add_subdirectory(data)
add_subdirectory(lib)

# uninstall target
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake" IMMEDIATE @ONLY)
add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

INCLUDE(InstallRequiredSystemLibraries)

SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Who's got tickles?")
SET(CPACK_PACKAGE_VENDOR "Sebastian Krzyszkowiak")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
SET(CPACK_PACKAGE_VERSION_MAJOR "0")
SET(CPACK_PACKAGE_VERSION_MINOR "1")
SET(CPACK_PACKAGE_VERSION_PATCH "0")
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "Tickle Monster")
IF(WIN32 AND NOT UNIX)
  # There is a bug in NSI that does not handle full unix paths properly. Make
  # sure there is at least one set of four (4) backlasshes.
  SET(CPACK_GENERATOR "NSIS")
  SET(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\ticklemonster.exe")
  SET(CPACK_NSIS_DISPLAY_NAME "Tickle Monster")
  SET(CPACK_NSIS_HELP_LINK "http:\\\\\\\\dosowisko.net\\\\\\\\ticklemonster")
  SET(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\dosowisko.net")
  SET(CPACK_NSIS_CONTACT "dos@dosowisko.net")
  SET(CPACK_NSIS_CREATE_ICONS "CreateShortCut '\$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\${CPACK_NSIS_DISPLAY_NAME}.lnk' '\$INSTDIR\\\\bin\\\\ticklemonster.exe'")
  SET(CPACK_NSIS_MUI_FINISHPAGE_RUN "ticklemonster.exe")
ELSE(WIN32 AND NOT UNIX)
  SET(CPACK_GENERATOR "DEB" "RPM")
  SET(CPACK_STRIP_FILES "bin/ticklemonster")
  SET(CPACK_SOURCE_STRIP_FILES "")
  IF(APPLE)
      SET(CPACK_GENERATOR "DragNDrop")
  ENDIF(APPLE)
ENDIF(WIN32 AND NOT UNIX)

SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Sebastian Krzyszkowiak <dos@dosowisko.net>")
SET(CPACK_DEBIAN_PACKAGE_DEPENDS "liballegro5.0,liballegro-ttf5.0,liballegro-image5.0,liballegro-audio5.0,liballegro-acodec5.0 ")

include(CPack)
