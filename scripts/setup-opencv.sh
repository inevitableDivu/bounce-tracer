#!/usr/bin/env bash
set -e

OPENCV_VERSION="4.10.0"
OPENCV_ZIP="opencv-${OPENCV_VERSION}-android-sdk.zip"
OPENCV_URL="https://github.com/opencv/opencv/releases/download/${OPENCV_VERSION}/${OPENCV_ZIP}"
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TARGET_DIR="${PROJECT_ROOT}/opencv-sdk"

echo "=== Bounce Tracer OpenCV Android SDK Setup ==="

if [ -d "${TARGET_DIR}/sdk/native/jni" ]; then
    echo "✅ OpenCV Android SDK is already installed at ${TARGET_DIR}."
    exit 0
fi

echo "📥 Downloading OpenCV ${OPENCV_VERSION} Android SDK..."
cd "${PROJECT_ROOT}"

if [ ! -f "${OPENCV_ZIP}" ]; then
    curl -L -o "${OPENCV_ZIP}" "${OPENCV_URL}"
fi

echo "📦 Extracting OpenCV Android SDK..."
TEMP_EXTRACT_DIR="${PROJECT_ROOT}/temp_opencv_extract"
mkdir -p "${TEMP_EXTRACT_DIR}"
unzip -q "${OPENCV_ZIP}" -d "${TEMP_EXTRACT_DIR}"

# Locate the extracted OpenCV-android-sdk folder
EXTRACTED_SDK="$(find "${TEMP_EXTRACT_DIR}" -maxdepth 2 -type d -name "*android-sdk*" | head -n 1)"

if [ -z "${EXTRACTED_SDK}" ]; then
    echo "❌ Error: Failed to locate extracted OpenCV Android SDK folder."
    rm -rf "${TEMP_EXTRACT_DIR}" "${OPENCV_ZIP}"
    exit 1
fi

echo "🚚 Moving SDK to ${TARGET_DIR}..."
mv "${EXTRACTED_SDK}" "${TARGET_DIR}"

echo "🧹 Cleaning up temporary files..."
rm -rf "${TEMP_EXTRACT_DIR}" "${OPENCV_ZIP}"

if [ -f "${TARGET_DIR}/sdk/native/jni/OpenCVConfig.cmake" ]; then
    echo "🎉 OpenCV ${OPENCV_VERSION} Android SDK successfully installed at ${TARGET_DIR}!"
else
    echo "❌ Error: OpenCVConfig.cmake not found after installation."
    exit 1
fi
