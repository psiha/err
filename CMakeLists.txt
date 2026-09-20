cmake_minimum_required( VERSION 3.28 )

project( psi_err LANGUAGES CXX )

###################
## Deps
###################

# Download CPM.cmake

file(
  DOWNLOAD
  https://github.com/cpm-cmake/CPM.cmake/releases/latest/download/get_cpm.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/cmake/get_cpm.cmake
)
set( CPM_SOURCE_CACHE "${CMAKE_CURRENT_BINARY_DIR}/deps" )

if ( APPLE ) # :wat: GH CI
    set( GIT_EXECUTABLE /usr/bin/git CACHE FILEPATH "Path to git executable" FORCE )
endif()

include( ${CMAKE_CURRENT_BINARY_DIR}/cmake/get_cpm.cmake )


# Boost (header-only: assert, config, throw_exception and - for last_win32_error - winapi) plus
# the unofficial psiha config_ex (BOOST_ATTRIBUTES / BOOST_ASSUME / BOOST_COLD / ...), the same
# CPM-sourced set psiha/vm and psiha/sweater already use so a fresh clone builds standalone,
# without depending on any locally installed Boost.

set( boost_ver boost-1.90.0 )
CPMAddPackage( "gh:boostorg/static_assert#${boost_ver}"   ) # Boost::core dependency
CPMAddPackage( "gh:boostorg/throw_exception#${boost_ver}" )
CPMAddPackage( "gh:boostorg/config#${boost_ver}"          )
CPMAddPackage( "gh:boostorg/assert#${boost_ver}"          )
CPMAddPackage( "gh:boostorg/core#${boost_ver}"            )
CPMAddPackage( "gh:boostorg/preprocessor#${boost_ver}"    )
CPMAddPackage( "gh:boostorg/predef#${boost_ver}"          ) # Boost::winapi dependency
CPMAddPackage( "gh:boostorg/winapi#${boost_ver}"          )

CPMAddPackage( "gh:psiha/config_ex#master" )

# An aggregate Boost::boost INTERFACE target so this file's own linkage below (and module.cmake's)
# resolves the same way it does inside psiha/vm and psiha/sweater.
if ( NOT TARGET Boost::boost )
    add_library( Boost::boost INTERFACE IMPORTED )
    target_link_libraries( Boost::boost INTERFACE
        Boost::config Boost::assert Boost::throw_exception Boost::core Boost::preprocessor Boost::winapi
    )
    target_include_directories( Boost::boost INTERFACE "${config_ex_SOURCE_DIR}/include" )
endif()


###################
## Target
###################

add_library( psi_err INTERFACE )
add_library( Psi::Err ALIAS psi_err )
target_include_directories( psi_err INTERFACE "${CMAKE_CURRENT_LIST_DIR}/include" )
target_compile_features( psi_err INTERFACE cxx_std_23 )
target_link_libraries( psi_err INTERFACE Boost::boost )
if ( WIN32 )
    target_compile_definitions( psi_err INTERFACE WIN32_LEAN_AND_MEAN NOMINMAX )
endif()

include( module.cmake )

if ( PROJECT_IS_TOP_LEVEL )
    enable_testing()
    add_subdirectory( test )
endif()
