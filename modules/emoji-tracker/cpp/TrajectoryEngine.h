#pragma once

#include <chrono>
#include <opencv2/opencv.hpp>
#include <vector>
#include <memory>

struct Vector2D {
  double x;
  double y;
};

// Extended telemetry for one tracked emoji
struct TrackingResult {
  double x_land;
  Vector2D velocity;
  Vector2D accel; // filtered acceleration estimate
  bool isValid;
  bool anomalyDetected;
  int trackId;    // -1 if none
  int trackCount; // number of active tracks this frame
  double fps;
  double processingTimeMs;
};

// Calibration: all screen-dependent constants in one place (Phase 4)
struct EngineCalibration
{
  double screenWidth = 1080.0;
  double screenHeight = 2400.0;
  double paddleY = 1650.0;
  double ceilingFraction = 0.15; // top fraction ignored (status bar / score)
  double cropBottomFraction = 0.92;
  double freezeY = 1250.0;       // prediction-freeze zone threshold
  double restitution = 1.0;      // floor/paddle bounce coefficient
  double framePeriodMs = 16.667; // display refresh period (Phase 3.6)
  double leadFrames = 5.0;       // dispatch lead time in display frames
};

// Constant-acceleration Kalman filter over [x, y, vx, vy, ax, ay]
class KalmanTracker
{
public:
  explicit KalmanTracker(int id, const Vector2D &pos, double timestampSec);

  int id() const { return m_id; }
  bool confirmed() const { return m_hits >= 2; }
  int missedFrames() const { return m_misses; }
  void incrementMisses() { ++m_misses; }
  double lastSeen() const { return m_lastTime; }

  // Predict state forward to `timestampSec` without measurement (also used
  // between frames to keep tracks alive during short occlusions).
  void predict(double timestampSec);

  // Correct with a new centroid measurement.
  void update(const Vector2D &pos, double timestampSec);

  Vector2D position() const;
  Vector2D velocity() const;
  Vector2D accel() const;

  // Predicted landing X at paddle Y given current filtered state.
  // Integrates with the 1/2*a*t^2 term and reflects off walls analytically.
  double predictLandingX(const EngineCalibration &cal) const;

  // Time until the emoji reaches paddleY (seconds), or -1 if not approaching.
  double timeToPaddle(const EngineCalibration &cal) const;

private:
  int m_id;
  double m_state[6]; // x, y, vx, vy, ax, ay
  double m_P[6][6];  // covariance
  double m_lastTime;
  int m_hits = 1;
  int m_misses = 0;
};

class TrajectoryEngine {
public:
  TrajectoryEngine();

  TrackingResult processFrame(uint8_t *frameBuffer, int width, int height,
                              int rowStride, double paddleWidth);

  void setCalibration(const EngineCalibration &cal) { m_cal = cal; }
  const EngineCalibration &calibration() const { return m_cal; }

private:
  // ---- detection pipeline ----
  cv::Mat m_lastGray;
  cv::Mat m_scratchGray; // preallocated buffers (Phase 5)
  cv::Mat m_scratchBlurred;
  cv::Mat m_scratchDiff;
  cv::Mat m_scratchMask;
  cv::Mat m_scratchEdges;
  cv::Mat m_scratchBlack;
  cv::Mat m_scratchFilteredBlack;
  cv::Mat m_scratchNonBlack;
  cv::Size m_bufferSize{0, 0};

  std::vector<std::pair<Vector2D, double>> detectCandidates(
      uint8_t *frameBuffer, int width, int height, int rowStride,
      std::vector<cv::Rect> &outBoxes);

  // ---- tracking ----
  std::vector<std::unique_ptr<KalmanTracker>> m_tracks;
  int m_nextTrackId = 1;
  double m_lastProcessTime = 0.0;
  bool m_hasPrevTime = false;

  // Frozen prediction (anti-jitter), keyed to the primary track id
  double m_predictedXLand = 540.0;
  bool m_hasPrediction = false;
  int m_predictionTrackId = -1;

  // ---- calibration & tunables ----
  EngineCalibration m_cal{};

  static constexpr double MIN_AREA = 500.0;
  static constexpr double MAX_AREA = 60000.0;        // widened for motion smear
  static constexpr double DT_MIN = 0.001;            // s
  static constexpr double DT_MAX = 0.100;            // s
  static constexpr double MAX_MISSES = 18;           // ~300ms at 60Hz
  static constexpr double GATE_DISTANCE = 180.0;     // px association gate
  static constexpr double ANOMALY_INNOVATION = 90.0; // px residual gate
};
