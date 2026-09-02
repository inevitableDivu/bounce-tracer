# 🤖 Bounce Tracer

> **Real-Time Automated Instagram Emoji Game Tracker & Sub-10ms Touch Dispatcher for Android**

Bounce Tracer is a high-performance, real-time automation bot designed for the Instagram Emoji Bounce Game on Android. Built with **Expo SDK 57**, **React Native (New Architecture)**, **C++20 with OpenCV**, **Android MediaProjection**, and **AccessibilityService**, Bounce Tracer captures high-speed screen frames, predicts projectile trajectory landing points with elastic wall bounce physics, and dispatches native touch gestures in under 10 milliseconds.

---

## ✨ Features

- **⚡ Sub-10ms End-to-End Latency**: Zero-copy frame buffer passing (`GetDirectBufferAddress`) directly from Android `MediaProjection` `ImageReader` to native C++ NDK logic, with preallocated OpenCV scratch buffers for a zero-allocation processing loop (~2-4ms C++ processing time).
- **🧠 6D Kalman Filter Trajectory Engine**: Constant-acceleration Kalman filter over the state vector $[x, y, v_x, v_y, a_x, a_y]$ per tracked emoji. Landing prediction solves $y_0 + v_y t + \frac{1}{2}a_y t^2 = y_{\text{paddle}}$ analytically and reflects off walls via modular arithmetic ($x_{\text{bounded}} = \text{mod}(x_{\text{raw}}, 2W)$).
- **🎯 Multi-Emoji Identity Tracking**: Top-N contour candidates are associated to per-track Kalman filters using nearest-neighbor gating (180px spatial gate), with automatic track spawning/pruning (~300ms miss timeout). The soonest-to-paddle confirmed track is selected as the automation target.
- **⚠️ Adaptive Anomaly Handling**: Non-linear emojis (such as 👽) no longer break tracking — large filter innovations inflate process uncertainty so the estimator re-converges quickly instead of discarding predictions.
- **🧊 Anti-Jitter Prediction Freeze**: Once the emoji descends past the freeze zone ($y > 1250$px) with a cached landing prediction, recomputation is bypassed to prevent erratic paddle micro-adjustments before contact.
- **🖥 Refresh-Rate Adaptive Timing**: Display refresh rate is detected at runtime (60/90/120/144Hz); frame-period quantization, gesture throttle cadence, and dispatch lead-time all scale with the detected rate.
- **👆 Low-Latency Gesture Injection**: Automated paddle movement via Android `AccessibilityService.dispatchGesture()`.
- **📱 Dual Telemetry HUD Overlays**:
  - **Native WindowManager Floating Overlay**: System-wide HUD rendering FPS, latency, target $X$, velocity vectors, and tracking status directly over Instagram or any active application.
  - **React Native Control Panel UI**: Modern glass-style dashboard showing live speed, acceleration magnitude, active track ID/count, anomaly state, and latency health (amber warning when a frame exceeds one 60Hz slot).
- **🔌 Automatic Expo Prebuild Integration**: Custom Expo Config Plugin (`plugins/withEmojiTracker.js`) auto-injects permissions, configures Accessibility XML, and links native Kotlin services during prebuild.

---

## 🏗 Architecture

```
┌────────────────────────────────┐
│  Android Screen Capture        │
│  (MediaProjection Foreground) │
│  + Runtime refresh-rate detect │
└───────────────┬────────────────┘
                │ Direct Memory Pointer (Zero-Copy)
                ▼
┌────────────────────────────────┐
│  C++ OpenCV Trajectory Engine  │
│  - Motion-diff + Canny detect  │
│  - 6D Kalman Filter per track  │
│  - Multi-track association     │
│  - Quadratic landing solver    │
│  - Elastic Reflection Physics  │
│  - Anomaly-robust estimation   │
└───────┬────────────────┬───────┘
        │                │
        ▼ Target x_land  ▼ Telemetry Data
┌───────────────┐ ┌────────────────────────────────┐
│ Accessibility │ │ Telemetry System               │
│ Touch Service │ │ - Native WindowManager HUD     │
│ (<10ms)       │ │ - React Native Control Panel   │
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
│       │   ├── TrajectoryEngine.h / .cpp     # 6D Kalman filter, multi-track association & bounce physics
│       │   └── EmojiTrackerModule.h / .cpp   # JSI C++ TurboModule bindings, telemetry store & calibration
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

## ⚙️ Tuning & Calibration

All screen-dependent constants live in a single `EngineCalibration` struct (C++) mirrored by `ModuleCalibration` in the Kotlin/JS bridge — no more hardcoded values scattered across layers:

| Constant | Default | Purpose |
| :-- | :-- | :-- |
| `screenWidth/Height` | 1080 × 2400 | Playfield bounds for wall reflection |
| `paddleY` | 1650 px | Landing target row |
| `restitution` | 1.0 | Floor/paddle bounce elasticity |
| `frameRateHz` | auto-detected | Display refresh rate (60–240Hz) |
| `leadFrames` | 5 | Dispatch lead time, in display frames |
| `freezeY` | 1250 px | Anti-jitter prediction freeze threshold |

The anomaly gate no longer discards predictions: large Kalman innovations flag the emoji as anomalous and widen filter uncertainty so tracking continues through 👽-style non-linear motion.

---

## 🤝 Contributing & License

Contributions are welcome! Please ensure you follow Expo SDK 57 native development guidelines.

This project is licensed under a Custom Non-Commercial License. See [LICENSE](LICENSE) for details.
