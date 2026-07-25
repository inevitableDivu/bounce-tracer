#include "TrajectoryEngine.h"
#include <cmath>
#include <vector>
#include <algorithm>

TrajectoryEngine::TrajectoryEngine() {}

TrackingResult TrajectoryEngine::processFrame(uint8_t* frameBuffer, int width, int height, int rowStride, double paddleWidth) {
    auto startTime = std::chrono::high_resolution_clock::now();

    // Zero-copy wrapping of RGBA buffer into OpenCV Mat
    cv::Mat frame(height, width, CV_8UC4, frameBuffer, rowStride);
    cv::Mat gray, hsv, maskColor, maskGray, combinedMask;

    // Convert to HSV and Grayscale for dual-mode emoji detection
    cv::cvtColor(frame, hsv, cv::COLOR_RGBA2RGB);
    cv::cvtColor(hsv, hsv, cv::COLOR_RGB2HSV);
    cv::cvtColor(frame, gray, cv::COLOR_RGBA2GRAY);

    // 1. Color Saturation Mask (Catches all colored emojis: 🏀, 🏓, 🥑, 👾, 🟡, etc.)
    cv::inRange(hsv, cv::Scalar(0, 30, 40), cv::Scalar(180, 255, 255), maskColor);

    // 2. Adaptive Contrast Mask (Catches monochrome / B&W emojis like ⚽)
    cv::threshold(gray, maskGray, 200, 255, cv::THRESH_BINARY);

    // Combine masks
    cv::bitwise_or(maskColor, maskGray, combinedMask);

    // Crop ROI to game field (ignore status bar at top 10% and paddle/keyboard at bottom 15%)
    int cropTop = static_cast<int>(height * 0.12);
    int cropBottom = static_cast<int>(height * 0.85);
    cv::Mat roi = combinedMask(cv::Range(cropTop, cropBottom), cv::Range(0, width));

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(roi, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    // Filter valid emoji ball contours by size/area (approx 40px to 180px diameter)
    double minArea = 500.0;
    double maxArea = 35000.0;

    std::vector<cv::Point> bestContour;
    double maxAreaFound = 0;

    for (const auto& contour : contours) {
        double area = cv::contourArea(contour);
        if (area >= minArea && area <= maxArea && area > maxAreaFound) {
            // Check aspect ratio to ensure shape is roughly square/circular (emoji ball)
            cv::Rect bbox = cv::boundingRect(contour);
            double aspectRatio = static_cast<double>(bbox.width) / bbox.height;
            if (aspectRatio >= 0.6 && aspectRatio <= 1.4) {
                maxAreaFound = area;
                bestContour = contour;
            }
        }
    }

    if (bestContour.empty()) {
        auto endTime = std::chrono::high_resolution_clock::now();
        double elapsedMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
        return {0.0, {0.0, 0.0}, false, false, 60.0, elapsedMs};
    }

    cv::Moments m = cv::moments(bestContour);
    if (m.m00 == 0) {
        auto endTime = std::chrono::high_resolution_clock::now();
        double elapsedMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
        return {0.0, {0.0, 0.0}, false, false, 60.0, elapsedMs};
    }

    // Offset Y coordinate back by cropTop offset
    Vector2D currentPos{m.m10 / m.m00, (m.m01 / m.m00) + cropTop};
    auto currentTime = std::chrono::high_resolution_clock::now();

    if (m_lastPos.x < 0) {
        m_lastPos = currentPos;
        m_lastTime = currentTime;
        return {0.0, {0.0, 0.0}, false, false, 60.0, 0.0};
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
        // Ball is moving upwards or trajectory is anomalous; show tracking active but no downward paddle move needed yet
        return {currentPos.x, currentVel, true, anomaly, fps, elapsedMs};
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
