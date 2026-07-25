#pragma once

#include <opencv2/opencv.hpp>
#include <chrono>

struct Vector2D {
    double x;
    double y;
};

struct TrackingResult {
    double x_land;
    Vector2D velocity;
    bool isValid;
    bool anomalyDetected;
    double fps;
    double processingTimeMs;
};

class TrajectoryEngine {
private:
    Vector2D m_lastPos{-1.0, -1.0};
    Vector2D m_lastVel{0.0, 0.0};
    std::chrono::high_resolution_clock::time_point m_lastTime;
    cv::Mat m_lastGray;
    double m_predictedXLand = 540.0;
    bool m_hasPrediction = false;
    
    const double ACCEL_THRESHOLD = 3500.0; // px/s^2 anomaly check limit
    const double PADDLE_Y = 1650.0;        // Standard screen Y coordinate of paddle
    const double SCREEN_WIDTH = 1080.0;

public:
    TrajectoryEngine();
    TrackingResult processFrame(uint8_t* frameBuffer, int width, int height, int rowStride, double paddleWidth);
};
