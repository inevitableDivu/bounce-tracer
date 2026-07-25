#include <jni.h>
#include <android/log.h>
#include "../../cpp/TrajectoryEngine.h"
#include "../../cpp/EmojiTrackerModule.h"

#define LOG_TAG "NativeBridge"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static TrajectoryEngine g_trajectoryEngine;

extern "C" JNIEXPORT void JNICALL
Java_com_inevitabledivu_bouncetracer_ScreenCaptureService_nativeProcessFrame(
    JNIEnv* env,
    jobject instance,
    jobject byteBuffer,
    jint width,
    jint height,
    jint pixelStride,
    jint rowStride) {

  // Get direct pointer to ByteBuffer memory (Zero-Copy)
  uint8_t* bufferPtr = static_cast<uint8_t*>(env->GetDirectBufferAddress(byteBuffer));
  if (!bufferPtr) {
    return;
  }

  double paddleWidth = facebook::react::EmojiTrackerModule::getPaddleWidth();
  TrackingResult result = g_trajectoryEngine.processFrame(bufferPtr, width, height, rowStride, paddleWidth);

  // Update shared C++ / JSI telemetry metrics
  facebook::react::EmojiTrackerModule::updateTelemetry(
      result.fps,
      result.processingTimeMs,
      result.x_land,
      result.velocity.x,
      result.velocity.y,
      result.isValid,
      result.anomalyDetected
  );

  // Update Native Floating Overlay HUD UI on Android UI Thread
  jclass trackerModuleClass = env->FindClass("com/inevitabledivu/bouncetracer/EmojiTrackerModule");
  if (trackerModuleClass) {
    jmethodID updateHudMethod = env->GetStaticMethodID(trackerModuleClass, "updateHUD", "(DDDZ)V");
    if (updateHudMethod) {
      env->CallStaticVoidMethod(trackerModuleClass, updateHudMethod,
                                (jdouble)result.fps,
                                (jdouble)result.processingTimeMs,
                                (jdouble)result.x_land,
                                (jboolean)result.isValid);
    }
  }

  // Trigger Native AccessibilityService gesture dispatcher if target landing is valid and moving downwards
  if (result.isValid && result.velocity.y > 0) {
    jclass accessibilityClass = env->FindClass("com/inevitabledivu/bouncetracer/InstagramEmojiAccessibilityService");
    if (accessibilityClass) {
      jmethodID dispatchMethod = env->GetStaticMethodID(accessibilityClass, "dispatchNativeTouch", "(FF)V");
      if (dispatchMethod) {
        env->CallStaticVoidMethod(accessibilityClass, dispatchMethod, (jfloat)result.x_land, 1650.0f);
      }
    }
  }
}
