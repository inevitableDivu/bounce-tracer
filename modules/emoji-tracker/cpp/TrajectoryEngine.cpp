#include "TrajectoryEngine.h"
#include <cmath>
#include <vector>
#include <algorithm>

TrajectoryEngine::TrajectoryEngine() {}

TrackingResult TrajectoryEngine::processFrame(uint8_t* frameBuffer, int width, int height, int rowStride, double paddleWidth) {
    auto startTime = std::chrono::high_resolution_clock::now();

    // Zero-copy wrapping of buffer into OpenCV Mat
    cv::Mat frame(height, width, CV_8UC4, frameBuffer, rowStride);
    cv::Mat hsv, mask;

    // Fast color segmentation / thresholding for emoji detection
    cv::cvtColor(frame, hsv, cv::COLOR_RGBA2RGB);
    cv::cvtColor(hsv, hsv, cv::COLOR_RGB2HSV);

    // Filter range targeting common emoji colors
    cv::inRange(hsv, cv::Scalar(0, 100, 100), cv::Scalar(30, 255, 255), mask);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    if (contours.empty()) {
        auto endTime = std::chrono::high_resolution_clock::now();
        double elapsedMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
        return {0.0, {0.0, 0.0}, false, false, 0.0, elapsedMs};
    }

    // Find largest contour (Emoji ball)
    auto largest = std::max_element(contours.begin(), contours.end(),
        [](const auto& a, const auto& b) { return cv::contourArea(a) < cv::contourArea(b); });

    cv::Moments m = cv::moments(*largest);
    if (m.m00 == 0) {
        auto endTime = std::chrono::high_resolution_clock::now();
        double elapsedMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
        return {0.0, {0.0, 0.0}, false, false, 0.0, elapsedMs};
    }

    Vector2D currentPos{m.m10 / m.m00, m.m01 / m.m00};
    auto currentTime = std::chrono::high_resolution_clock::now();

    if (m_lastPos.x < 0) {
        m_lastPos = currentPos;
        m_lastTime = currentTime;
        return {0.0, {0.0, 0.0}, false, false, 0.0, 0.0};
    }

    double dt = std::chrono::duration<double>(currentTime - m_lastTime).count();
    if (dt <= 0.001) dt = 0.001;

    double fps = 1.0 / dt;

    // Dynamic Velocity Vector Computation
    Vector2D currentVel{
        (currentPos.x - m_lastPos.x) / dt,
        (currentPos.y - m_lastPos.y) / dt
    };

    // Acceleration Anomaly Detection (Handles non-linear emojis like 👽)
    Vector2D accel{
        (currentVel.x - m_lastVel.x) / dt,
        (currentVel.y - m_lastVel.y) / dt
    };
    double totalAccel = std::sqrt(accel.x * accel.x + accel.y * accel.y);

    bool anomaly = totalAccel > ACCEL_THRESHOLD;

    // Update state
    m_lastPos = currentPos;
    m_lastVel = currentVel;
    m_lastTime = currentTime;

    auto endTime = std::chrono::high_resolution_clock::now();
    double elapsedMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();

    if (anomaly || currentVel.y <= 0) {
        // Ball is moving upwards or trajectory is anomalous; reset target
        return {0.0, currentVel, false, anomaly, fps, elapsedMs};
    }

    // --- Deterministic Elastic Bounce Prediction ---
    double timeToReachPaddle = (PADDLE_Y - currentPos.y) / currentVel.y;
    double unconstrainedXLand = currentPos.x + (currentVel.x * timeToReachPaddle);

    // Reflective boundary projection using period mapping: 2 * SCREEN_WIDTH
    double period = 2.0 * SCREEN_WIDTH;
    double modX = std::fmod(unconstrainedXLand, period);
    if (modX < 0) modX += period;

    double predictedX = (modX <= SCREEN_WIDTH) ? modX : (period - modX);

    // Align target position with center of paddle
    double targetCenterPosition = predictedX - (paddleWidth / 2.0);

    return {targetCenterPosition, currentVel, true, false, fps, elapsedMs};
}
