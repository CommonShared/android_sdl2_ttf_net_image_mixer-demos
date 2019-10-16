# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/SDL2-2.0.10/include)
include_directories(${CMAKE_SOURCE_DIR}/SDL2-2.0.10/src)
include_directories(${CMAKE_SOURCE_DIR}/SDL2_net-2.0.1)
include_directories(${CMAKE_SOURCE_DIR}/SDL2_ttf-2.0.15)
include_directories(${CMAKE_SOURCE_DIR}/SDL2_mixer-2.0.4)
include_directories(${CMAKE_SOURCE_DIR}/SDL2_image-2.0.5)
include_directories(${CMAKE_SOURCE_DIR}/ffmpeg-dev/include)

add_library(hidapi
        SHARED
        IMPORTED)
set_target_properties(hidapi
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/jniLibs/${ANDROID_ABI}/libhidapi.so)
add_library(SDL2
        SHARED
        IMPORTED)
set_target_properties(SDL2
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/jniLibs/${ANDROID_ABI}/libSDL2.so)
add_library(SDL2_net
        SHARED
        IMPORTED)
set_target_properties(SDL2_net
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/jniLibs/${ANDROID_ABI}/libSDL2_net.so)
add_library(SDL2_ttf
        SHARED
        IMPORTED)
set_target_properties(SDL2_ttf
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/jniLibs/${ANDROID_ABI}/libSDL2_ttf.so)
add_library(mpg123
        SHARED
        IMPORTED)
set_target_properties(mpg123
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/jniLibs/${ANDROID_ABI}/libmpg123.so)
add_library(SDL2_mixer
        SHARED
        IMPORTED)
set_target_properties(SDL2_mixer
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/jniLibs/${ANDROID_ABI}/libSDL2_mixer.so)
add_library(SDL2_image
        SHARED
        IMPORTED)
set_target_properties(SDL2_image
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/jniLibs/${ANDROID_ABI}/libSDL2_image.so)
##############################################################################

add_library(avcodec
        SHARED
        IMPORTED)
set_target_properties(avcodec
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/jniLibs/${ANDROID_ABI}/libavcodec.so)

add_library(avdevice
        SHARED
        IMPORTED)
set_target_properties(avdevice
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/jniLibs/${ANDROID_ABI}/libavdevice.so)

add_library(avfilter
        SHARED
        IMPORTED)
set_target_properties(avfilter
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/jniLibs/${ANDROID_ABI}/libavfilter.so)

add_library(avformat
        SHARED
        IMPORTED)
set_target_properties(avformat
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/jniLibs/${ANDROID_ABI}/libavformat.so)

add_library(avutil
        SHARED
        IMPORTED)
set_target_properties(avutil
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/jniLibs/${ANDROID_ABI}/libavutil.so)

add_library(swresample
        SHARED
        IMPORTED)
set_target_properties(swresample
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/jniLibs/${ANDROID_ABI}/libswresample.so)

add_library(swscale
        SHARED
        IMPORTED)
set_target_properties(swscale
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/jniLibs/${ANDROID_ABI}/libswscale.so)

##############################################################################
add_library( # Sets the name of the library.
        native-lib

        # Sets the library as a shared library.
        SHARED

        # Provides a relative path to your source file(s).
        ${CMAKE_SOURCE_DIR}/native-lib.cpp
        ${CMAKE_SOURCE_DIR}/SDL_android_main.cpp
        #${CMAKE_SOURCE_DIR}/main.cpp
        #${CMAKE_SOURCE_DIR}/fivedata.cpp
        #${CMAKE_SOURCE_DIR}/five.cpp

        #${CMAKE_SOURCE_DIR}/main-play-video.cpp

        ${CMAKE_SOURCE_DIR}/cmdutils.c
        ${CMAKE_SOURCE_DIR}/main-ffplay.c
        )

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.


find_library( # Sets the name of the path variable.
        log-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log)

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
        native-lib

        # Links the target library to the log library
        # included in the NDK.
        ${log-lib}
        SDL2
        SDL2_net
        SDL2_image
        SDL2_mixer
        SDL2_ttf
        mpg123
        hidapi
        #############################
        avcodec
        avdevice
        avfilter
        avformat
        avutil
        swresample
        swscale
        #############################
        #${SDL2-lib}
        )