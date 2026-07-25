#pragma once

#include <jsi/jsi.h>
#include <ReactCommon/TurboModule.h>
#include <atomic>
#include <memory>

namespace facebook::react {

class JSI_EXPORT EmojiTrackerModule : public TurboModule {
public:
  EmojiTrackerModule(std::shared_ptr<CallInvoker> jsInvoker);
  ~EmojiTrackerModule();

  // TurboModule JSI Bindings
  jsi::Value getTelemetrySync(jsi::Runtime& rt);
  void setPaddleWidth(double width);

  static void updateTelemetry(double fps, double processingTimeMs, double xLand, double vx, double vy, bool isTracking, bool anomaly);
  static double getPaddleWidth();

private:
  static inline std::atomic<double> s_fps{0.0};
  static inline std::atomic<double> s_processingTimeMs{0.0};
  static inline std::atomic<double> s_predictedXLand{0.0};
  static inline std::atomic<double> s_velocityX{0.0};
  static inline std::atomic<double> s_velocityY{0.0};
  static inline std::atomic<bool> s_isTracking{false};
  static inline std::atomic<bool> s_anomalyDetected{false};
  static inline std::atomic<double> s_paddleWidth{120.0};
};

} // namespace facebook::react
