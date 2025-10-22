cmake_minimum_required(VERSION 3.24)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

project(PKGInstall LANGUAGES CXX)

set(BASE_ARCHITECTURE "${CMAKE_SYSTEM_PROCESSOR}")

# Next, match common architecture strings down to a known common value.
if (BASE_ARCHITECTURE MATCHES "(x86)|(X86)|(amd64)|(AMD64)")
    set(ARCHITECTURE "x86_64")
elseif (BASE_ARCHITECTURE MATCHES "(aarch64)|(AARCH64)|(arm64)|(ARM64)")
    set(ARCHITECTURE "arm64")
else()
    message(FATAL_ERROR "Unsupported CPU architecture: ${BASE_ARCHITECTURE}")
endif()

# Setup a custom clang-format target (if clang-format can be found) that will run
# against all the src files. This should be used before making a pull request.

if (CLANG_FORMAT)
    set(SRCS ${PROJECT_SOURCE_DIR)
    set(CCOMMENT "Running clang format against all the .h and .cpp files in src/")
    if (WIN32)
        add_custom_target(clang-format
            COMMAND powershell.exe -Command "Get-ChildItem '${SRCS}/*' -Include *.cpp,*.h,*.mm -Recurse | Foreach {&'${CLANG_FORMAT}' -i $_.fullname}"
            COMMENT ${CCOMMENT})
    else()
        add_custom_target(clang-format
            COMMAND find ${SRCS} -iname *.h -o -iname *.cpp -o -iname *.mm | xargs ${CLANG_FORMAT} -i
            COMMENT ${CCOMMENT})
    endif()
    unset(SRCS)
    unset(CCOMMENT)
endif()

if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR NOT MSVC)
    find_package(cryptopp 8.9.0 MODULE)
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent)
find_package(ZLIB 1.3 MODULE)
find_package(fmt 10.2.0 CONFIG)

add_subdirectory(externals)

set(PROJECT_SOURCES
        src/alignment.h
        src/concepts.h
        src/crypto.cpp
        src/crypto.h
        src/endian.h
        src/enum.h
        src/io_file.cpp
        src/io_file.h
        src/keys.h
        src/loader.cpp
        src/loader.h
	src/nt_api.cpp
        src/nt_api.h
        src/pfs.h
        src/pkg.cpp
        src/pkg.h
        src/pkg_type.cpp
        src/pkg_type.h
        src/psf.cpp
        src/psf.h
        src/types.h
        main.cpp
        mainWindow.cpp
        mainWindow.h
        mainWindow.ui
)

qt_add_executable(PKGInstall
    ${PROJECT_SOURCES}
)

if (WIN32)
    target_sources(PKGInstall PRIVATE dist/PKGInstall.rc)
endif()

target_link_libraries(PKGInstall PRIVATE Qt6::Widgets Qt6::Concurrent)

target_link_libraries(PKGInstall PRIVATE cryptopp::cryptopp ZLIB::ZLIB fmt::fmt)

set_target_properties(PKGInstall PROPERTIES
    WIN32_EXECUTABLE TRUE
)

target_include_directories(PKGInstall PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/dist)
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    install(FILES "PKGInstall.desktop" DESTINATION "share/applications")
    install(FILES "PKGIcon.png" DESTINATION "share/icons/hicolor/512x512/apps")
endif()

