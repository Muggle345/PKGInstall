set(BUILD_SHARED_LIBS OFF)
set(BUILD_TESTING OFF)
set_directory_properties(PROPERTIES
    EXCLUDE_FROM_ALL ON
    SYSTEM ON
)

# CryptoPP
if (NOT TARGET cryptopp::cryptopp)
    set(CRYPTOPP_INSTALL OFF)
    set(CRYPTOPP_BUILD_TESTING OFF)
    set(CRYPTOPP_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/cryptopp)
    # cryptopp instruction set checks do not account for added compile options,
    # so disable extensions in the library config to match our chosen target CPU.
    set(CRYPTOPP_DISABLE_AESNI ON)
    set(CRYPTOPP_DISABLE_AVX2 ON)
    add_subdirectory(cryptopp-cmake)
    file(COPY cryptopp DESTINATION cryptopp FILES_MATCHING PATTERN "*.h")
    # remove externals/cryptopp from include directories because it contains a conflicting zlib.h file
    set_target_properties(cryptopp PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_BINARY_DIR}/cryptopp")
endif()

# zlib
if (NOT TARGET ZLIB::ZLIB)
    set(ZLIB_ENABLE_TESTS OFF)
    set(WITH_GTEST OFF)
    set(WITH_NEW_STRATEGIES ON)
    set(WITH_NATIVE_INSTRUCTIONS ON)
    set(ZLIB_COMPAT ON CACHE BOOL "" FORCE)
    include(FetchContent)
    FetchContent_Declare(
        ZLIB
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/zlib-ng"
        OVERRIDE_FIND_PACKAGE
    )
    FetchContent_MakeAvailable(ZLIB)
    add_library(ZLIB::ZLIB ALIAS zlib)
    # libpng expects this variable to exist after its find_package(ZLIB)
    set(ZLIB_INCLUDE_DIRS "${FETCHCONTENT_BASE_DIR}/zlib-build")
endif()

# fmtlib
if (NOT TARGET fmt::fmt)
    add_subdirectory(fmt)
endif()

# Toml11
if (NOT TARGET toml11::toml11)
    add_subdirectory(toml11)

    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
      if (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        get_target_property(_toml11_compile_options toml11 INTERFACE_COMPILE_OPTIONS)
        list(REMOVE_ITEM _toml11_compile_options "/Zc:preprocessor")
        set_target_properties(toml11 PROPERTIES INTERFACE_COMPILE_OPTIONS ${_toml11_compile_options})
      endif()
    endif()
endif()
