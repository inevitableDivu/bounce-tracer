#include <jni.h>
#include <android/log.h>
#include "../../cpp/TrajectoryEngine.h"
#include "../../cpp/EmojiTrackerModule.h"

#define LOG_TAG "NativeBridge"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static TrajectoryEngine g_trajectoryEngine;

extern "C" JNIEXPORT void JNICALL
Java_com_emojibot_ScreenCaptureService_nativeProcessFrame(
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

  // Update shared telemetry metrics
  facebook::react::EmojiTrackerModule::updateTelemetry(
      result.fps,
      result.processingTimeMs,
      result.x_land,
      result.velocity.x,
      result.velocity.y,
      result.isValid,
      result.anomalyDetected
  );

  // Trigger Native AccessibilityService gesture dispatcher if target landing is valid
  if (result.isValid) {
    jclass accessibilityClass = env->FindClass("com/emojibot/InstagramEmojiAccessibilityService");
    if (accessibilityClass) {
      jmethodID dispatchMethod = env->GetStaticMethodID(accessibilityClass, "dispatchNativeTouch", "(FF)V");
      if (dispatchMethod) {
        env->CallStaticVoidMethod(accessibilityClass, dispatchMethod, (jfloat)result.x_land, 1650.0f);
      }
    }
  }
}
