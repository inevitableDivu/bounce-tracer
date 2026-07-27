# 🤖 Bounce Tracer

> **Real-Time Automated Instagram Emoji Game Tracker & Sub-30ms Touch Dispatcher for Android**

Bounce Tracer is a high-performance, real-time automation bot designed for the Instagram Emoji Bounce Game on Android. Built with **Expo SDK 57**, **React Native (New Architecture)**, **C++20 with OpenCV**, **Android MediaProjection**, and **AccessibilityService**, Bounce Tracer captures high-speed screen frames, predicts projectile trajectory landing points with elastic wall bounce physics, and dispatches native touch gestures in under 30 milliseconds.

---

## ✨ Features

- **⚡ Sub-30ms End-to-End Latency**: Zero-copy frame buffer passing (`GetDirectBufferAddress`) directly from Android `MediaProjection` `ImageReader` to native C++ NDK logic.
- **📐 OpenCV Trajectory Engine**: C++ physics engine tracking emoji centroids and computing exact paddle landing coordinates ($x_{\text{land}}$) using modular wall reflection arithmetic ($x_{\text{bounded}} = \text{mod}(x_{\text{raw}}, 2W)$).
- **⚠️ Acceleration Anomaly Detection**: Detects non-linear trajectory changes ($\Delta v / \Delta t$) for unpredictable emojis (such as 👽) and adapts target positioning dynamically.
- **👆 Low-Latency Gesture Injection**: Automated paddle movement via Android `AccessibilityService.dispatchGesture()`.
- **📱 Dual Telemetry HUD Overlays**:
  - **Native WindowManager Floating Overlay**: System-wide HUD rendering FPS, latency, target $X$, velocity vectors, and tracking status directly over Instagram or any active application.
  - **React Native Control Panel UI**: In-app interface for configuring settings, monitoring JSI telemetry loops, and toggling automation.
- **🔌 Automatic Expo Prebuild Integration**: Custom Expo Config Plugin (`plugins/withEmojiTracker.js`) auto-injects permissions, configures Accessibility XML, and links native Kotlin services during prebuild.

---

## 🏗 Architecture

```
┌────────────────────────────────┐
│  Android Screen Capture        │
│  (MediaProjection Foreground) │
└───────────────┬────────────────┘
                │ Direct Memory Pointer (Zero-Copy)
                ▼
┌────────────────────────────────┐
│  C++ OpenCV Trajectory Engine  │
│  - HSV Centroid Segmentation   │
│  - Elastic Reflection Physics  │
│  - Acceleration Anomaly Filter │
└───────┬────────────────┬───────┘
        │                │
        ▼ Target x_land  ▼ Telemetry Data
┌───────────────┐ ┌────────────────────────────────┐
│ Accessibility │ │ Telemetry System               │
│ Touch Service │ │ - Native WindowManager HUD     │
│ (<30ms)       │ │ - React Native Control Panel   │
└───────────────┘ └────────────────────────────────┘
```

---

## 🛠 Tech Stack

- **Framework**: React Native 0.86 (New Architecture), Expo SDK ~57.0.8, TypeScript 6.0
- **Native / C++ Engine**: C++20, OpenCV NDK SDK, CMake
- **Android APIs**: MediaProjection API, AccessibilityService API, WindowManager Overlay System
- **Package Manager**: `pnpm`

---

## 📂 Project Structure

```
bounce-tracer/
├── App.tsx                                   # React Native main entry & control screen
├── app.json                                  # Expo app configuration & config plugin registration
├── plugins/
│   └── withEmojiTracker.js                  # Expo Config Plugin for manifest permissions & native linking
├── modules/
│   └── emoji-tracker/                        # Local Expo Module (TurboModule & Native Services)
│       ├── CMakeLists.txt                    # Native C++ build configuration
│       ├── cpp/                              # Native C++ trajectory calculation core
│       │   ├── TrajectoryEngine.h / .cpp     # OpenCV centroid tracking & bounce physics
│       │   └── EmojiTrackerModule.h / .cpp   # JSI C++ TurboModule bindings
│       ├── android/src/main/java/.../
│       │   ├── EmojiTrackerModule.kt         # Native Kotlin Expo module & intent launcher
│       │   ├── ScreenCaptureService.kt       # MediaProjection Foreground Service
│       │   ├── InstagramEmojiAccessibilityService.kt # Gesture injection dispatcher
│       │   └── FloatingHUDView.kt            # Native WindowManager system overlay view
│       └── src/
│           └── NativeEmojiTrackerSpec.ts     # TypeScript Codegen TurboModule Spec
├── src/
│   ├── components/
│   │   └── HUDOverlay.tsx                    # React Native HUD control overlay component
│   └── types/
│       └── global.d.ts                       # Global type definitions for native module
└── scripts/
    └── setup-android-cli.sh                  # Linux Android Command Line Tools installer
```

---

## 🚀 Getting Started

### Prerequisites

- **Node.js**: v18+ and `pnpm`
- **JDK**: Java 17 OpenJDK (required for Expo SDK 57 Android builds)
- **Android SDK & NDK**: Android NDK 26+ and API Level 26+ (Android 8.0+)
- **Android Device**: Physical Android device recommended for MediaProjection & Accessibility gesture execution.

### Installation

1. **Clone the repository**:
   ```bash
   git clone https://github.com/inevitabledivu/bounce-tracer.git
   cd bounce-tracer
   ```

2. **Install dependencies**:
   ```bash
   pnpm install
   ```

3. **Install OpenCV Android NDK SDK**:
   ```bash
   pnpm setup:opencv
   ```

4. **Configure Android CLI Tools (Linux optional)**:
   ```bash
   bash scripts/setup-android-cli.sh
   ```

---

## 📱 Running the App

1. **Generate Native Android Project (Prebuild)**:
   ```bash
   npx expo prebuild --platform android
   ```

2. **Build and Run on Connected Android Device**:
   ```bash
   pnpm android
   ```
   *or*
   ```bash
   npx expo run:android
   ```

---

## 🔐 Required System Permissions

To function correctly, **Bounce Tracer** requires three Android system permissions:

1. **Display Over Apps (`SYSTEM_ALERT_WINDOW`)**: Required for rendering the native floating telemetry HUD over Instagram. Click "Enable Display Over Apps" in the control panel.
2. **Accessibility Service (`BIND_ACCESSIBILITY_SERVICE`)**: Required for injecting paddle touch gestures. Enable "Automated Instagram Emoji Game Touch Automation Service" in Android Accessibility Settings.
3. **Screen Recording (`MEDIA_PROJECTION`)**: Prompted automatically when starting automation to capture screen frames.

---

## 🤝 Contributing & License

Contributions are welcome! Please ensure you follow Expo SDK 57 native development guidelines.

This project is licensed under the MIT License. See [LICENSE](file:///home/inevitable/Projects/bounce-tracer/LICENSE) for details.
