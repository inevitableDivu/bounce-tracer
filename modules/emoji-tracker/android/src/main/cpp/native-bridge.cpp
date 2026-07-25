#include <jni.h>
#include <android/log.h>
#include "../../cpp/TrajectoryEngine.h"
#include "../../cpp/EmojiTrackerModule.h"

#define LOG_TAG "NativeBridge"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static TrajectoryEngine g_trajectoryEngine;
static JavaVM* g_jvm = nullptr;
static jclass g_trackerModuleClass = nullptr;
static jmethodID g_updateHudMethod = nullptr;
static jclass g_accessibilityClass = nullptr;
static jmethodID g_dispatchMethod = nullptr;

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
  g_jvm = vm;
  JNIEnv* env = nullptr;
  if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
    return JNI_ERR;
  }

  jclass trackerCls = env->FindClass("com/inevitabledivu/bouncetracer/EmojiTrackerModule");
  if (trackerCls) {
    g_trackerModuleClass = reinterpret_cast<jclass>(env->NewGlobalRef(trackerCls));
    g_updateHudMethod = env->GetStaticMethodID(g_trackerModuleClass, "updateHUD", "(DDDZ)V");
  }

  jclass a11yCls = env->FindClass("com/inevitabledivu/bouncetracer/InstagramEmojiAccessibilityService");
  if (a11yCls) {
    g_accessibilityClass = reinterpret_cast<jclass>(env->NewGlobalRef(a11yCls));
    g_dispatchMethod = env->GetStaticMethodID(g_accessibilityClass, "dispatchNativeTouch", "(FF)V");
  }

  return JNI_VERSION_1_6;
}

extern "C" JNIEXPORT void JNICALL
Java_com_inevitabledivu_bouncetracer_ScreenCaptureService_nativeProcessFrame(
    JNIEnv* env,
    jobject instance,
    jobject byteBuffer,
    jint width,
    jint height,
    jint pixelStride,
    jint rowStride) {

  if (!byteBuffer) return;

  uint8_t* bufferPtr = static_cast<uint8_t*>(env->GetDirectBufferAddress(byteBuffer));
  if (!bufferPtr) return;

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

  // Update Native Floating Overlay HUD UI
  if (g_trackerModuleClass && g_updateHudMethod) {
    env->CallStaticVoidMethod(g_trackerModuleClass, g_updateHudMethod,
                              (jdouble)result.fps,
                              (jdouble)result.processingTimeMs,
                              (jdouble)result.x_land,
                              (jboolean)result.isValid);
  }

  // Trigger Native AccessibilityService gesture dispatcher if target landing is valid
  if (result.isValid && result.velocity.y > 0 && g_accessibilityClass && g_dispatchMethod) {
    env->CallStaticVoidMethod(g_accessibilityClass, g_dispatchMethod, (jfloat)result.x_land, 1650.0f);
  }
}
