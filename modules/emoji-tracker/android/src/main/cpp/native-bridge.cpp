#include <jni.h>
#include <android/log.h>
#include "TrajectoryEngine.h"
#include "EmojiTrackerModule.h"

#define LOG_TAG "NativeBridge"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static TrajectoryEngine g_trajectoryEngine;
static JavaVM *g_jvm = nullptr;
static jclass g_trackerModuleClass = nullptr;
static jmethodID g_updateHudMethod = nullptr;
static jmethodID g_updateExtendedTelemetryMethod = nullptr;
static jclass g_accessibilityClass = nullptr;
static jmethodID g_dispatchMethod = nullptr;
static double g_paddleY = 1650.0; // updated from Kotlin via setCalibration

// Convert ModuleCalibration to the engine's EngineCalibration
static void applyCalibrationToEngine()
{
  auto cal = facebook::react::EmojiTrackerModule::getCalibration();
  EngineCalibration ec;
  ec.screenWidth = cal.screenWidth;
  ec.screenHeight = cal.screenHeight;
  ec.paddleY = cal.paddleY;
  ec.restitution = cal.restitution;
  ec.framePeriodMs = (cal.frameRateHz > 1.0) ? 1000.0 / cal.frameRateHz : 16.667;
  ec.leadFrames = cal.leadFrames;
  g_trajectoryEngine.setCalibration(ec);
  g_paddleY = cal.paddleY;
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
  __android_log_print(ANDROID_LOG_INFO, "EmojiTracker_Native", "JNI_OnLoad: Initializing...");
  g_jvm = vm;
  JNIEnv *env = nullptr;
  if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK)
  {
    __android_log_print(ANDROID_LOG_ERROR, "EmojiTracker_Native", "JNI_OnLoad: GetEnv failed");
    return JNI_ERR;
  }

  jclass trackerCls = env->FindClass("com/inevitabledivu/bouncetracer/EmojiTrackerModule");
  if (trackerCls)
  {
    __android_log_print(ANDROID_LOG_INFO, "EmojiTracker_Native", "JNI_OnLoad: Found EmojiTrackerModule class");
    g_trackerModuleClass = reinterpret_cast<jclass>(env->NewGlobalRef(trackerCls));
    g_updateHudMethod = env->GetStaticMethodID(g_trackerModuleClass, "updateHUD", "(DDDZ)V");
    if (!g_updateHudMethod)
    {
      __android_log_print(ANDROID_LOG_ERROR, "EmojiTracker_Native", "JNI_OnLoad: updateHUD method NOT found");
    }
    else
    {
      __android_log_print(ANDROID_LOG_INFO, "EmojiTracker_Native", "JNI_OnLoad: updateHUD method registered");
    }
    g_updateExtendedTelemetryMethod = env->GetStaticMethodID(
        g_trackerModuleClass, "updateExtendedTelemetry", "(DDDDIIZ)V");
    if (!g_updateExtendedTelemetryMethod)
    {
      __android_log_print(ANDROID_LOG_ERROR, "EmojiTracker_Native", "JNI_OnLoad: updateExtendedTelemetry method NOT found");
    }
  }
  else
  {
    __android_log_print(ANDROID_LOG_ERROR, "EmojiTracker_Native", "JNI_OnLoad: EmojiTrackerModule class NOT found");
  }
  if (env->ExceptionCheck())
  {
    env->ExceptionClear();
  }

  jclass a11yCls = env->FindClass("com/inevitabledivu/bouncetracer/InstagramEmojiAccessibilityService");
  if (a11yCls)
  {
    __android_log_print(ANDROID_LOG_INFO, "EmojiTracker_Native", "Found InstagramEmojiAccessibilityService class");
    g_accessibilityClass = reinterpret_cast<jclass>(env->NewGlobalRef(a11yCls));
    g_dispatchMethod = env->GetStaticMethodID(g_accessibilityClass, "dispatchNativeTouch", "(FF)V");
    if (!g_dispatchMethod)
    {
      __android_log_print(ANDROID_LOG_ERROR, "EmojiTracker_Native", "JNI_OnLoad: dispatchNativeTouch method NOT found");
    }
  }
  else
  {
    __android_log_print(ANDROID_LOG_ERROR, "EmojiTracker_Native", "JNI_OnLoad: InstagramEmojiAccessibilityService class NOT found");
  }
  if (env->ExceptionCheck())
  {
    env->ExceptionClear();
  }

  applyCalibrationToEngine();

  return JNI_VERSION_1_6;
}

extern "C" JNIEXPORT void JNICALL
Java_com_inevitabledivu_bouncetracer_ScreenCaptureService_nativeSetCalibration(
    JNIEnv *env,
    jobject instance,
    jint screenWidth,
    jint screenHeight,
    jdouble paddleY,
    jdouble frameRateHz,
    jdouble leadFrames,
    jdouble restitution)
{
  facebook::react::ModuleCalibration cal;
  cal.screenWidth = static_cast<double>(screenWidth);
  cal.screenHeight = static_cast<double>(screenHeight);
  cal.paddleY = paddleY;
  cal.frameRateHz = frameRateHz;
  cal.leadFrames = leadFrames;
  cal.restitution = restitution;
  facebook::react::EmojiTrackerModule::setCalibration(cal);
  __android_log_print(ANDROID_LOG_INFO, "EmojiTracker_Native",
                      "nativeSetCalibration: %dx%d paddleY=%.0f rate=%.1fHz lead=%.1f frames e=%.2f",
                      screenWidth, screenHeight, paddleY, frameRateHz, leadFrames, restitution);
}

extern "C" JNIEXPORT void JNICALL
Java_com_inevitabledivu_bouncetracer_ScreenCaptureService_nativeProcessFrame(
    JNIEnv *env,
    jobject instance,
    jobject byteBuffer,
    jint width,
    jint height,
    jint pixelStride,
    jint rowStride)
{

  if (!byteBuffer)
    return;

  uint8_t *bufferPtr = static_cast<uint8_t *>(env->GetDirectBufferAddress(byteBuffer));
  if (!bufferPtr)
    return;

  try
  {
    applyCalibrationToEngine();
    double paddleWidth = facebook::react::EmojiTrackerModule::getPaddleWidth();
    TrackingResult result = g_trajectoryEngine.processFrame(bufferPtr, width, height, rowStride, paddleWidth);

    static int frameCount = 0;
    frameCount++;
    if (frameCount % 60 == 0 || result.isValid)
    {
      __android_log_print(ANDROID_LOG_INFO, "EmojiTracker_Native",
                          "Frame processed: frame=%d, valid=%d, fps=%.2f, latency=%.2f, xLand=%.2f, vel=(%.2f, %.2f), accel=(%.1f, %.1f), tracks=%d",
                          frameCount, result.isValid, result.fps, result.processingTimeMs, result.x_land, result.velocity.x, result.velocity.y, result.accel.x, result.accel.y, result.trackCount);
    }

    // Update shared telemetry metrics
    facebook::react::EmojiTrackerModule::updateTelemetry(
        result.fps,
        result.processingTimeMs,
        result.x_land,
        result.velocity.x,
        result.velocity.y,
        result.accel.x,
        result.accel.y,
        result.isValid,
        result.anomalyDetected,
        result.trackId,
        result.trackCount);

    // Push extended telemetry (accel, tracks) to Kotlin for JS polling
    if (g_trackerModuleClass && g_updateExtendedTelemetryMethod)
    {
      env->CallStaticVoidMethod(g_trackerModuleClass, g_updateExtendedTelemetryMethod,
                                (jdouble)result.velocity.x,
                                (jdouble)result.velocity.y,
                                (jdouble)result.accel.x,
                                (jdouble)result.accel.y,
                                (jint)result.trackId,
                                (jint)result.trackCount,
                                (jboolean)result.anomalyDetected);
      if (env->ExceptionCheck())
      {
        env->ExceptionClear();
      }
    }

    // Update Native Floating Overlay HUD UI
    if (g_trackerModuleClass && g_updateHudMethod)
    {
      env->CallStaticVoidMethod(g_trackerModuleClass, g_updateHudMethod,
                                (jdouble)result.fps,
                                (jdouble)result.processingTimeMs,
                                (jdouble)result.x_land,
                                (jboolean)result.isValid);
      if (env->ExceptionCheck())
      {
        env->ExceptionClear();
      }
    }

    // Trigger Native AccessibilityService gesture dispatcher if target landing is valid
    if (result.isValid && g_accessibilityClass && g_dispatchMethod)
    {
      env->CallStaticVoidMethod(g_accessibilityClass, g_dispatchMethod, (jfloat)result.x_land, (jfloat)g_paddleY);
      if (env->ExceptionCheck())
      {
        env->ExceptionClear();
      }
    }
  }
  catch (...)
  {
    // Catch all C++ exceptions to prevent JNI crash (SIGABRT)
    __android_log_print(ANDROID_LOG_ERROR, "EmojiTracker_Native", "Native crash prevented in nativeProcessFrame");
  }
}
