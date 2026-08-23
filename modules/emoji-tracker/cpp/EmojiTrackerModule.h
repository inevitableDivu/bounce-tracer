#pragma once

#include <atomic>
#include <mutex>

namespace facebook::react
{

  // Screen/refresh calibration shared between JS/Kotlin and the native engine.
  struct ModuleCalibration
  {
    double screenWidth = 1080.0;
    double screenHeight = 2400.0;
    double paddleY = 1650.0;
    double frameRateHz = 60.0;
    double leadFrames = 5.0;
    double restitution = 1.0;
  };

  class EmojiTrackerModule
  {
  public:
    static void setPaddleWidth(double width);
    static double getPaddleWidth();

    static void setCalibration(const ModuleCalibration &cal);
    static ModuleCalibration getCalibration();

    static void updateTelemetry(double fps, double processingTimeMs, double xLand,
                                double vx, double vy, double ax, double ay,
                                bool isTracking, bool anomaly,
                                int trackId, int trackCount);

  private:
    static inline std::atomic<double> s_fps{0.0};
    static inline std::atomic<double> s_processingTimeMs{0.0};
    static inline std::atomic<double> s_predictedXLand{0.0};
    static inline std::atomic<double> s_velocityX{0.0};
    static inline std::atomic<double> s_velocityY{0.0};
    static inline std::atomic<double> s_accelX{0.0};
    static inline std::atomic<double> s_accelY{0.0};
    static inline std::atomic<bool> s_isTracking{false};
    static inline std::atomic<bool> s_anomalyDetected{false};
    static inline std::atomic<int> s_trackId{-1};
    static inline std::atomic<int> s_trackCount{0};
    static inline std::atomic<double> s_paddleWidth{120.0};

    static inline std::mutex s_calMutex;
    static inline ModuleCalibration s_calibration{};
  };

} // namespace facebook::react
