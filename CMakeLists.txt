project(cecforwarder)
cmake_minimum_required(VERSION 2.8.9)

set(cecforwarder_NAME cecforwarder)
set(cecforwarder_DESCRIPTION "libCEC test client")
set(cecforwarder_VERSION_MAJOR ${LIBCEC_VERSION_MAJOR})
set(cecforwarder_VERSION_MINOR ${LIBCEC_VERSION_MINOR})
set(cecforwarder_VERSION_PATCH ${LIBCEC_VERSION_PATCH})

enable_language(CXX)
include(CheckCXXSourceCompiles)
include(CheckLibraryExists)
include(CheckIncludeFiles)
include(CheckCXXCompilerFlag)

check_cxx_compiler_flag("-std=c++11" SUPPORTS_CXX11)
if (SUPPORTS_CXX11)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

find_package(p8-platform REQUIRED)
find_package(Threads REQUIRED)

set(cecforwarder_SOURCES main.cpp cecforwarder.cpp irsend.cpp config.cpp)

add_executable(cec-forwarder ${cecforwarder_SOURCES})
set_target_properties(cec-forwarder PROPERTIES VERSION ${LIBCEC_VERSION_MAJOR}.${LIBCEC_VERSION_MINOR}.${LIBCEC_VERSION_PATCH})
target_link_libraries(cec-forwarder ${p8-platform_LIBRARIES})
target_link_libraries(cec-forwarder ${CMAKE_THREAD_LIBS_INIT})

if (NOT WIN32)
  # check for dlopen
  check_library_exists(dl dlopen "" HAVE_DLOPEN)
  if (HAVE_DLOPEN)
    target_link_libraries(cec-forwarder dl)
  endif()

  # rt
  check_library_exists(rt clock_gettime "" HAVE_RT)
  if (HAVE_RT)
    target_link_libraries(cec-forwarder rt)
  endif()

  # CoreVideo
  if (APPLE)
    target_link_libraries(cec-forwarder "-framework CoreVideo")
  endif()
else()
  add_definitions(-DTARGET_WINDOWS -DNOMINMAX -D_CRT_SECURE_NO_WARNINGS -D_WINSOCKAPI_)
  if (NOT ${WIN64})
    add_definitions(-D_USE_32BIT_TIME_T)
  endif()
endif()

include_directories(${p8-platform_INCLUDE_DIRS}
                    ${PROJECT_SOURCE_DIR})

# write env.h
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/env.h.in ${CMAKE_CURRENT_SOURCE_DIR}/env.h)

if (WIN32)
  install(TARGETS     cec-forwarder
          DESTINATION .)
else()
  install(TARGETS     cec-forwarder
          DESTINATION bin/.)
endif()
