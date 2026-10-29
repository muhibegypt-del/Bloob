cmake_minimum_required(VERSION 3.16)
project(bloob_native VERSION 3.2.1 LANGUAGES C CXX)

# Cross-compile for the TV with the webOS NDK (openlgtv buildroot-nc4):
#   cmake -B build -DCMAKE_TOOLCHAIN_FILE=/opt/arm-webos-linux-gnueabi_sdk-buildroot/share/buildroot/toolchainfile.cmake
# Host builds (no toolchain) only build the unit tests.

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif ()

add_compile_options(-Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers)
add_compile_definitions(BLOOB_VERSION="${PROJECT_VERSION}")
if (CMAKE_CROSSCOMPILING)
  # GCC notes that ARM parameter passing changed in GCC 7; irrelevant with a static C++ runtime.
  add_compile_options(-Wno-psabi)
endif ()

# Platform-free core, shared by the app and the tests.
add_library(buna_core STATIC src/session.cpp src/json.cpp src/log.cpp src/model.cpp src/play.cpp src/resources.cpp src/tasks.cpp src/stats.cpp src/apps.cpp)
find_package(Threads REQUIRED)
target_link_libraries(buna_core PUBLIC Threads::Threads)
target_include_directories(buna_core PUBLIC src)

# Calibration (CALIBRATION-PLAN.md): colour science, LG's tables, the panel
# model, the procedure, and the virtual G2 it is tested on. No dependencies.
add_library(bloob_cal STATIC cal/colour.cpp cal/lut.cpp cal/measure.cpp cal/model.cpp cal/grey.cpp cal/sdr.cpp cal/hdr.cpp cal/sim.cpp)
target_include_directories(bloob_cal PUBLIC cal)
# Optimised even in Debug: the tests run whole calibrations on the virtual G2.
target_compile_options(bloob_cal PRIVATE -O2)
# bloob-cal: the calibration runner on the TV (root, over SSH): luna-send and the
# second-screen socket to the TV, Argyll's spotread for the meter (tools/bloob_cal.cpp).
add_executable(bloob-cal tools/bloob_cal.cpp cal/tv_device.cpp cal/ssap.cpp cal/meter.cpp)
target_link_libraries(bloob-cal PRIVATE bloob_cal buna_core)
if (CMAKE_CROSSCOMPILING)
  target_link_options(bloob-cal PRIVATE -static-libstdc++ -static-libgcc -Wl,--as-needed)
endif ()

# The playback engine's demuxer and audio converter (3.0) and Headphones mode
# (3.2: binaural.cpp, headphones.cpp), on Bloob's own
# static FFmpeg (scripts/build-ffmpeg-libs.sh arm|host, into build-ffmpeg/).
if (CMAKE_CROSSCOMPILING)
  set(FFMPEG_PREFIX ${CMAKE_SOURCE_DIR}/build-ffmpeg/arm)
else ()
  set(FFMPEG_PREFIX ${CMAKE_SOURCE_DIR}/build-ffmpeg/host)
endif ()
if (NOT EXISTS ${FFMPEG_PREFIX}/lib/libavformat.a)
  message(FATAL_ERROR "FFmpeg libraries missing: run scripts/build-ffmpeg-libs.sh ${CMAKE_CROSSCOMPILING}")
endif ()
add_library(buna_demux STATIC src/demux.cpp src/binaural.cpp src/headphones.cpp)
target_include_directories(buna_demux PUBLIC src ${FFMPEG_PREFIX}/include)
target_compile_definitions(buna_demux PUBLIC __STDC_CONSTANT_MACROS)
target_link_libraries(buna_demux PUBLIC buna_core
  ${FFMPEG_PREFIX}/lib/libavfilter.a ${FFMPEG_PREFIX}/lib/libavformat.a ${FFMPEG_PREFIX}/lib/libavcodec.a
  ${FFMPEG_PREFIX}/lib/libswresample.a ${FFMPEG_PREFIX}/lib/libavutil.a m)

# The app: shell, renderer, screens and media. The TV build talks to the real
# system bus; the desktop build swaps in a stand-in (luna_host.cpp).
set(APP_SOURCES
  src/main.cpp src/gfx.cpp src/media.cpp src/platform.cpp src/http.cpp src/api.cpp
  src/images.cpp src/icons.cpp src/ui.cpp src/common.cpp src/pairing.cpp src/speech.cpp
  src/browse.cpp src/details.cpp src/sources.cpp src/player.cpp src/search.cpp src/settings.cpp src/inspect.cpp src/alloc.cpp
  src/es_player.cpp src/starfish.cpp)

if (CMAKE_CROSSCOMPILING)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(SDL2 REQUIRED sdl2)
  pkg_check_modules(GLIB REQUIRED glib-2.0)

  add_executable(bloob ${APP_SOURCES} src/luna.cpp)
  target_include_directories(bloob PRIVATE ${SDL2_INCLUDE_DIRS} ${GLIB_INCLUDE_DIRS})
  # BLOOB_STARFISH: the engine's TV output (starfish.cpp; libplayerAPIs is opened at run time).
  target_compile_definitions(bloob PRIVATE BLOOB_STARFISH)
  target_link_libraries(bloob PRIVATE buna_core buna_demux ${SDL2_LIBRARIES} ${GLIB_LIBRARIES} helpers pthread dl)
  # The TV's libstdc++ is older than the NDK's; carry our own C++ runtime so the
  # binary needs no newer GLIBCXX symbols. Only C interfaces cross into TV libs.
  target_link_options(bloob PRIVATE -static-libstdc++ -static-libgcc -Wl,--as-needed)

  set(PKG ${CMAKE_BINARY_DIR}/pkg)
  # Restage from scratch so files dropped from the app never linger in the package.
  add_custom_command(TARGET bloob POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${PKG}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PKG}/bin ${PKG}/assets/fonts ${PKG}/assets/media
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:bloob> ${PKG}/bin/bloob
    COMMAND ${CMAKE_STRIP} ${PKG}/bin/bloob
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/package/appinfo.json ${CMAKE_SOURCE_DIR}/package/icon.png ${PKG}/
    COMMAND ${CMAKE_COMMAND} -E copy
      ${CMAKE_SOURCE_DIR}/third_party/fonts/Inter-Regular.ttf
      ${CMAKE_SOURCE_DIR}/third_party/fonts/Inter-Medium.ttf
      ${CMAKE_SOURCE_DIR}/third_party/fonts/Inter-SemiBold.ttf
      ${CMAKE_SOURCE_DIR}/third_party/fonts/Inter-Bold.ttf
      ${CMAKE_SOURCE_DIR}/third_party/fonts/Inter-LICENSE.txt
      ${PKG}/assets/fonts/
    COMMAND ${CMAKE_COMMAND} -E copy
      ${CMAKE_SOURCE_DIR}/../tv-app/service/www/media/big-buck-bunny-480p-30sec.mp4
      ${CMAKE_SOURCE_DIR}/../tv-app/service/www/media/CREDITS.txt
      ${PKG}/assets/media/
    COMMENT "Staging package in ${PKG}")
else ()
  # Desktop build of the full UI with a stand-in bus, for scripted screenshots.
  find_package(PkgConfig)
  if (PkgConfig_FOUND)
    pkg_check_modules(SDL2 sdl2)
  endif ()
  if (SDL2_FOUND)
    add_executable(bloob_host ${APP_SOURCES} src/luna_host.cpp)
    target_include_directories(bloob_host PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(bloob_host PRIVATE buna_core buna_demux ${SDL2_LIBRARIES} pthread dl)
  endif ()
  # Headphones mode previews: WAVs to hear the rooms on a phone (tools/hp_preview.cpp).
  add_executable(hp_preview tools/hp_preview.cpp)
  target_link_libraries(hp_preview PRIVATE buna_demux)
  enable_testing()
  add_executable(native_tests tests/session_test.cpp tests/play_test.cpp tests/runtime_test.cpp tests/demux_test.cpp tests/headphones_test.cpp)
  target_link_libraries(native_tests PRIVATE buna_core buna_demux)
  add_test(NAME native_tests COMMAND native_tests)
  add_executable(cal_tests tests/cal_test.cpp)
  target_link_libraries(cal_tests PRIVATE bloob_cal)
  target_compile_options(cal_tests PRIVATE -O2)
  add_test(NAME cal_tests COMMAND cal_tests)
endif ()
