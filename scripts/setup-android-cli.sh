#!/usr/bin/env bash
set -e

SDK_DIR="$HOME/.android-sdk"
CMDLINE_ZIP="commandlinetools-linux-15859902_latest.zip"
DOWNLOAD_URL="https://dl.google.com/android/repository/${CMDLINE_ZIP}"

echo "🚀 Setting up Android Command-Line Tools (No IDE)..."

mkdir -p "$SDK_DIR/cmdline-tools"

if [ ! -d "$SDK_DIR/cmdline-tools/latest" ]; then
  echo "📥 Downloading Android Command-Line Tools ($CMDLINE_ZIP)..."
  curl -o "$SDK_DIR/$CMDLINE_ZIP" "$DOWNLOAD_URL"
  
  echo "📦 Extracting CLI tools..."
  cd "$SDK_DIR/cmdline-tools"
  unzip -q "$SDK_DIR/$CMDLINE_ZIP"
  mv cmdline-tools latest
  rm "$SDK_DIR/$CMDLINE_ZIP"
fi

export ANDROID_HOME="$SDK_DIR"
export PATH="$PATH:$ANDROID_HOME/cmdline-tools/latest/bin:$ANDROID_HOME/platform-tools"

echo "✅ Accepting Android SDK licenses..."
yes | sdkmanager --licenses > /dev/null 2>&1 || true

echo "📥 Installing required SDK platforms, NDK, and CMake..."
sdkmanager "platform-tools" "platforms;android-34" "build-tools;34.0.0" "ndk;25.2.9519653" "cmake;3.22.1"

echo "🎉 Android CLI environment successfully configured!"
echo "--------------------------------------------------------"
echo "NOTE: OpenJDK 17 is required for modern Expo/Android builds."
echo "Install on Ubuntu/Debian via: sudo apt install openjdk-17-jdk"
echo ""
echo "Add the following lines to your ~/.bashrc or ~/.zshrc:"
echo "--------------------------------------------------------"
echo "export JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64"
echo "export ANDROID_HOME=$HOME/.android-sdk"
echo "export PATH=\$PATH:\$JAVA_HOME/bin:\$ANDROID_HOME/cmdline-tools/latest/bin:\$ANDROID_HOME/platform-tools"
echo "--------------------------------------------------------"
