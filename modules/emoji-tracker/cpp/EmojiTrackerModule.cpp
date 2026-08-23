#include "EmojiTrackerModule.h"

namespace facebook::react
{

  void EmojiTrackerModule::setPaddleWidth(double width)
  {
    s_paddleWidth.store(width);
  }

  double EmojiTrackerModule::getPaddleWidth()
  {
    return s_paddleWidth.load();
  }

  void EmojiTrackerModule::setCalibration(const ModuleCalibration &cal)
  {
    std::lock_guard<std::mutex> lock(s_calMutex);
    s_calibration = cal;
  }

  ModuleCalibration EmojiTrackerModule::getCalibration()
  {
    std::lock_guard<std::mutex> lock(s_calMutex);
    return s_calibration;
  }

  void EmojiTrackerModule::updateTelemetry(double fps, double processingTimeMs,
                                           double xLand, double vx, double vy,
                                           double ax, double ay, bool isTracking,
                                           bool anomaly, int trackId, int trackCount)
  {
    s_fps.store(fps);
    s_processingTimeMs.store(processingTimeMs);
    s_predictedXLand.store(xLand);
    s_velocityX.store(vx);
    s_velocityY.store(vy);
    s_accelX.store(ax);
    s_accelY.store(ay);
    s_isTracking.store(isTracking);
    s_anomalyDetected.store(anomaly);
    s_trackId.store(trackId);
    s_trackCount.store(trackCount);
  }

} // namespace facebook::react
