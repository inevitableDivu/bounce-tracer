#include "EmojiTrackerModule.h"

namespace facebook::react {

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

} // namespace facebook::react
