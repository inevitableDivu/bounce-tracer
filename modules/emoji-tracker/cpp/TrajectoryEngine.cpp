#include "TrajectoryEngine.h"
#include <cmath>
#include <vector>
#include <algorithm>

TrajectoryEngine::TrajectoryEngine() {}

TrackingResult TrajectoryEngine::processFrame(uint8_t* frameBuffer, int width, int height, int rowStride, double paddleWidth) {
    auto startTime = std::chrono::high_resolution_clock::now();

    // Zero-copy wrapping of RGBA buffer into OpenCV Mat
    cv::Mat frame(height, width, CV_8UC4, frameBuffer, rowStride);
    cv::Mat gray;
    cv::cvtColor(frame, gray, cv::COLOR_RGBA2GRAY);

    // Smooth the grayscale image to reduce compression noise/artifacts from screen capture
    cv::Mat blurred;
    cv::GaussianBlur(gray, blurred, cv::Size(5, 5), 0);

    // 1. Motion Mask via Temporal Frame Differencing
    // This completely wipes out static elements like the score text, chat bubbles, background UI.
    cv::Mat motionMask = cv::Mat::zeros(gray.size(), CV_8UC1);
    if (!m_lastGray.empty() && m_lastGray.size() == gray.size()) {
        cv::Mat diff;
        cv::absdiff(gray, m_lastGray, diff);
        // Any pixel shift > 10 grayscale units is classified as motion
        cv::threshold(diff, motionMask, 10, 255, cv::THRESH_BINARY);
    }
    gray.copyTo(m_lastGray);

    // 2. Canny Edge Detection (highlights boundaries of shapes)
    cv::Mat combinedMask;
    cv::Canny(blurred, combinedMask, 35, 100);

    // 3. Intersect Canny edges with Motion Mask
    // Only moving edges (the emoji) will survive!
    if (!m_lastGray.empty()) {
        // Dilate the motion mask slightly to ensure it overlaps fully with the current frame edges
        cv::dilate(motionMask, motionMask, cv::Mat(), cv::Point(-1, -1), 2);
        cv::bitwise_and(combinedMask, motionMask, combinedMask);
    }

    // 4. Black Color Exclusion Mask (to ignore the black paddle and its borders)
    // The paddle is a solid black bar. By masking out pixels under a threshold (gray < 55),
    // dilating it to cover the transition edges, and subtracting it, we completely erase the paddle
    // from our detection space without needing to crop the bottom of the screen.
    // To protect tiny black details on emojis (like soccer balls), we only filter out
    // black objects that are larger than 20px in width and 10px in height.
    cv::Mat blackMask;
    cv::threshold(gray, blackMask, 55, 255, cv::THRESH_BINARY_INV);

    std::vector<std::vector<cv::Point>> blackContours;
    cv::findContours(blackMask, blackContours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    cv::Mat filteredBlackMask = cv::Mat::zeros(blackMask.size(), CV_8UC1);
    for (const auto& contour : blackContours) {
        cv::Rect bbox = cv::boundingRect(contour);
        if (bbox.width >= 20 && bbox.height >= 10) {
            cv::drawContours(filteredBlackMask, std::vector<std::vector<cv::Point>>{contour}, -1, 255, cv::FILLED);
        }
    }

    cv::dilate(filteredBlackMask, filteredBlackMask, cv::Mat(), cv::Point(-1, -1), 3);
    cv::Mat nonBlackMask;
    cv::bitwise_not(filteredBlackMask, nonBlackMask);
    cv::bitwise_and(combinedMask, nonBlackMask, combinedMask);

    // Dilate the edges slightly to close any small gaps in the emoji boundary
    cv::dilate(combinedMask, combinedMask, cv::Mat(), cv::Point(-1, -1), 1);

    // Crop ROI to game field
    // Ignore top 15% (status bar and score overlay)
    // Crop bottom at 92% of screen height to ignore system navigation buttons,
    // keeping the entire paddle area fully open for emoji tracking!
    int cropTop = static_cast<int>(height * 0.15);
    int cropBottom = static_cast<int>(height * 0.92);
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

    // Reset prediction freeze when the ball starts moving upwards
    if (currentVel.y < 0) {
        m_hasPrediction = false;
    }

    if (anomaly) {
        return {currentPos.x, currentVel, true, anomaly, fps, elapsedMs};
    }

    // Freeze target Y threshold: 1250 px (approx. bottom 30% of the game area)
    // If the ball goes below this threshold and we already have a valid cached prediction,
    // we bypass calculations and immediately return the frozen target center to prevent jitters/lag.
    if (currentVel.y > 0 && currentPos.y > 1250.0 && m_hasPrediction) {
        return {m_predictedXLand, currentVel, true, false, fps, elapsedMs};
    }

    // Dynamic ceiling-reflective trajectory prediction
    double totalTime = 0.0;
    if (currentVel.y > 0) {
        // Ball is moving downwards: straight path to paddle
        totalTime = (PADDLE_Y - currentPos.y) / currentVel.y;
    } else if (currentVel.y < 0) {
        // Ball is moving upwards: predict bounce off ceiling, then travel to paddle
        double ceilingY = height * 0.15;
        totalTime = (2.0 * ceilingY - currentPos.y - PADDLE_Y) / currentVel.y;
    }

    double targetCenterPosition = currentPos.x;
    if (totalTime > 0.0) {
        double unconstrainedXLand = currentPos.x + (currentVel.x * totalTime);

        // Reflective boundary projection using period mapping: 2 * SCREEN_WIDTH
        double period = 2.0 * SCREEN_WIDTH;
        double modX = std::fmod(unconstrainedXLand, period);
        if (modX < 0) modX += period;

        double predictedX = (modX <= SCREEN_WIDTH) ? modX : (period - modX);

        // Align target position with center of paddle
        targetCenterPosition = predictedX - (paddleWidth / 2.0);
        
        // Cache the valid prediction
        m_predictedXLand = targetCenterPosition;
        m_hasPrediction = true;
    }

    return {targetCenterPosition, currentVel, true, false, fps, elapsedMs};
}
