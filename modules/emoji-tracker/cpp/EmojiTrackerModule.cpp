#include "EmojiTrackerModule.h"

namespace facebook::react {

EmojiTrackerModule::EmojiTrackerModule(std::shared_ptr<CallInvoker> jsInvoker)
    : TurboModule("EmojiTrackerModule", jsInvoker) {}

EmojiTrackerModule::~EmojiTrackerModule() {}

void EmojiTrackerModule::setPaddleWidth(double width) {
  s_paddleWidth.store(width);
}

double EmojiTrackerModule::getPaddleWidth() {
  return s_paddleWidth.load();
}

void EmojiTrackerModule::updateTelemetry(double fps, double processingTimeMs, double xLand, double vx, double vy, bool isTracking, bool anomaly) {
  s_fps.store(fps);
  s_processingTimeMs.store(processingTimeMs);
  s_predictedXLand.store(xLand);
  s_velocityX.store(vx);
  s_velocityY.store(vy);
  s_isTracking.store(isTracking);
  s_anomalyDetected.store(anomaly);
}

jsi::Value EmojiTrackerModule::getTelemetrySync(jsi::Runtime& rt) {
  jsi::Object obj(rt);
  obj.setProperty(rt, "fps", jsi::Value(s_fps.load()));
  obj.setProperty(rt, "processingTimeMs", jsi::Value(s_processingTimeMs.load()));
  obj.setProperty(rt, "predictedXLand", jsi::Value(s_predictedXLand.load()));
  obj.setProperty(rt, "velocityX", jsi::Value(s_velocityX.load()));
  obj.setProperty(rt, "velocityY", jsi::Value(s_velocityY.load()));
  obj.setProperty(rt, "isTracking", jsi::Value(s_isTracking.load()));
  obj.setProperty(rt, "anomalyDetected", jsi::Value(s_anomalyDetected.load()));
  return obj;
}

} // namespace facebook::react
