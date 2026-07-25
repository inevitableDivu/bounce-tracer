cmake_minimum_required(VERSION 3.18.1)
project(emojitracker)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set OpenCV_DIR to the downloaded SDK in project root
set(OpenCV_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../opencv-sdk/sdk/native/jni)

# Find OpenCV NDK package
find_package(OpenCV REQUIRED)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/cpp
    ${OpenCV_INCLUDE_DIRS}
)

add_library(
    emojitracker
    SHARED
    cpp/EmojiTrackerModule.cpp
    cpp/TrajectoryEngine.cpp
    android/src/main/cpp/native-bridge.cpp
)

target_link_libraries(
    emojitracker
    ${OpenCV_LIBS}
    log
    android
)
