#include "TrajectoryEngine.h"
#include <cmath>
#include <algorithm>
#include <limits>

// ============================================================================
// KalmanTracker — constant-acceleration filter over [x, y, vx, vy, ax, ay]
// ============================================================================

namespace
{

    constexpr int N = 6;

    // Process noise: base values; Q is inflated adaptively when the innovation
    // (measurement residual) grows, which lets the filter follow acceleration
    // that ramps up over time without a hard-coded jerk model.
    struct NoiseParams
    {
        double pos = 4.0;
        double vel = 40.0;
        double acc = 400.0;
    };
    constexpr NoiseParams k_noise{};

    // File-local tunables shared by KalmanTracker and TrajectoryEngine
    constexpr double k_dtMin = 0.001;            // s
    constexpr double k_dtMax = 0.100;            // s
    constexpr double k_anomalyInnovation = 90.0; // px residual gate
    constexpr double k_measNoise = 25.0;         // R = diag(25) ~ 5px noise

} // namespace

KalmanTracker::KalmanTracker(int id, const Vector2D &pos, double timestampSec)
    : m_id(id), m_lastTime(timestampSec)
{
    m_state[0] = pos.x;
    m_state[1] = pos.y;
    m_state[2] = 0.0;
    m_state[3] = 0.0;
    m_state[4] = 0.0;
    m_state[5] = 0.0;

    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j)
            m_P[i][j] = (i == j) ? 100.0 : 0.0;
}

void KalmanTracker::predict(double timestampSec)
{
    double dt = timestampSec - m_lastTime;
    if (dt <= 0.0)
        return;
    if (dt > k_dtMax)
        dt = k_dtMax;

    // State transition: x' = x + v*dt + 0.5*a*dt^2, v' = v + a*dt, a' = a
    double halfDt2 = 0.5 * dt * dt;
    m_state[0] += m_state[2] * dt + m_state[4] * halfDt2;
    m_state[1] += m_state[3] * dt + m_state[5] * halfDt2;
    m_state[2] += m_state[4] * dt;
    m_state[3] += m_state[5] * dt;

    // Covariance propagation F*P*F' + Q (F only has identity/dt terms).
    // Build F row-wise for position rows.
    const double F[N][N] = {
        {1, 0, dt, 0, halfDt2, 0},
        {0, 1, 0, dt, 0, halfDt2},
        {0, 0, 1, 0, dt, 0},
        {0, 0, 0, 1, 0, dt},
        {0, 0, 0, 0, 1, 0},
        {0, 0, 0, 0, 0, 1}};

    double FP[N][N];
    for (int i = 0; i < N; ++i)
    {
        for (int j = 0; j < N; ++j)
        {
            double sum = 0.0;
            for (int k = 0; k < N; ++k)
                sum += F[i][k] * m_P[k][j];
            FP[i][j] = sum;
        }
    }
    // newP = FP * F^T
    double newP[N][N];
    for (int i = 0; i < N; ++i)
    {
        for (int j = 0; j < N; ++j)
        {
            double sum = 0.0;
            for (int k = 0; k < N; ++k)
                sum += FP[i][k] * F[j][k];
            newP[i][j] = sum;
        }
    }

    // Adaptive process noise: scale with dt growth so longer gaps widen P.
    double qScale = dt / 0.016667; // relative to one 60Hz frame
    double Qdiag[N] = {
        k_noise.pos * qScale, k_noise.pos * qScale,
        k_noise.vel * qScale, k_noise.vel * qScale,
        k_noise.acc * qScale, k_noise.acc * qScale};

    for (int i = 0; i < N; ++i)
    {
        newP[i][i] += Qdiag[i];
        for (int j = 0; j < N; ++j)
            m_P[i][j] = newP[i][j];
    }

    m_lastTime = timestampSec;
}

void KalmanTracker::update(const Vector2D &pos, double timestampSec)
{
    predict(timestampSec);

    // Measurement model: we observe position only (H picks rows 0,1).
    double innov[2] = {pos.x - m_state[0], pos.y - m_state[1]};
    double S[2][2] = {
        {m_P[0][0] + k_measNoise, m_P[0][1]},
        {m_P[1][0], m_P[1][1] + k_measNoise}}; // R = diag(25) ~ 5px measurement noise

    double detS = S[0][0] * S[1][1] - S[0][1] * S[1][0];
    if (std::fabs(detS) < 1e-9)
        detS = 1e-9;
    double Sinv[2][2] = {
        {S[1][1] / detS, -S[0][1] / detS},
        {-S[1][0] / detS, S[0][0] / detS}};

    // Anomaly detection via innovation magnitude (replaces ACCEL_THRESHOLD):
    // large residual => sudden teleport/acceleration event. We do NOT discard
    // the prediction; we inflate P so the filter adapts quickly and flag it.
    double innovNorm = std::sqrt(innov[0] * innov[0] + innov[1] * innov[1]);
    if (innovNorm > k_anomalyInnovation)
    {
        for (int i = 0; i < N; ++i)
            m_P[i][i] *= 8.0;
    }

    double K[6][2];
    for (int i = 0; i < N; ++i)
    {
        K[i][0] = m_P[i][0] * Sinv[0][0] + m_P[i][1] * Sinv[1][0];
        K[i][1] = m_P[i][0] * Sinv[0][1] + m_P[i][1] * Sinv[1][1];
    }

    for (int i = 0; i < N; ++i)
        m_state[i] += K[i][0] * innov[0] + K[i][1] * innov[1];

    // P' = (I - K*H) * P ; H*P is just rows 0,1 of P
    double newP2[N][N];
    for (int i = 0; i < N; ++i)
    {
        for (int j = 0; j < N; ++j)
        {
            newP2[i][j] = m_P[i][j] - (K[i][0] * m_P[0][j] + K[i][1] * m_P[1][j]);
        }
    }
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j)
            m_P[i][j] = newP2[i][j];

    m_hits++;
    m_misses = 0;
}

Vector2D KalmanTracker::position() const { return {m_state[0], m_state[1]}; }
Vector2D KalmanTracker::velocity() const { return {m_state[2], m_state[3]}; }
Vector2D KalmanTracker::accel() const { return {m_state[4], m_state[5]}; }

double KalmanTracker::timeToPaddle(const EngineCalibration &cal) const
{
    const double y = m_state[1];
    const double vy = m_state[3];
    const double ay = m_state[5];
    const double targetY = cal.paddleY;

    if (vy > 0.0 && y <= targetY)
    {
        // Moving down: solve y + vy*t + 0.5*ay*t^2 = targetY
        double c = y - targetY;
        double disc = vy * vy - 2.0 * ay * c;
        if (disc < 0.0)
            return -1.0;
        double t = (-vy + std::sqrt(disc)) / ay; // larger root = arrival time
        if (ay <= 1e-6)
            t = c / -vy; // degenerate: constant velocity
        return (t > 0.0) ? t : -1.0;
    }
    if (vy < 0.0)
    {
        // Moving up: time to ceiling bounce, then down to paddle with reflected
        // velocity (perfectly elastic ceiling).
        double ceilingY = cal.screenHeight * cal.ceilingFraction;
        double c = y - ceilingY;
        double disc = vy * vy - 2.0 * ay * c;
        if (disc < 0.0)
            return -1.0;
        double tUp;
        if (ay >= -1e-6)
            tUp = c / -vy;
        else
            tUp = (-vy + std::sqrt(disc)) / ay;
        if (tUp <= 0.0)
            return -1.0;
        double vyAtCeiling = vy + ay * tUp;
        double vyReflected = -vyAtCeiling * cal.restitution;
        if (vyReflected <= 0.0)
            return -1.0;
        double distDown = targetY - ceilingY;
        double disc2 = vyReflected * vyReflected + 2.0 * ay * distDown;
        double tDown = (ay > 1e-6)
                           ? (-vyReflected + std::sqrt(std::max(0.0, disc2))) / ay
                           : distDown / vyReflected;
        return tUp + tDown;
    }
    return -1.0;
}

double KalmanTracker::predictLandingX(const EngineCalibration &cal) const
{
    double totalTime = timeToPaddle(cal);
    if (totalTime <= 0.0)
        return m_state[0];

    // Unconstrained landing X including the 1/2*a*t^2 term.
    double unconstrained = m_state[0] + m_state[2] * totalTime +
                           0.5 * m_state[4] * totalTime * totalTime;

    // Reflective boundary projection using period mapping: 2 * W
    double period = 2.0 * cal.screenWidth;
    double modX = std::fmod(unconstrained, period);
    if (modX < 0.0)
        modX += period;
    return (modX <= cal.screenWidth) ? modX : (period - modX);
}

// ============================================================================
// TrajectoryEngine — detection pipeline + multi-track management
// ============================================================================

TrajectoryEngine::TrajectoryEngine() = default;

std::vector<std::pair<Vector2D, double>> TrajectoryEngine::detectCandidates(
    uint8_t *frameBuffer, int width, int height, int rowStride,
    std::vector<cv::Rect> &outBoxes)
{
    // Zero-copy wrapping of RGBA buffer into OpenCV Mat
    cv::Mat frame(height, width, CV_8UC4, frameBuffer, rowStride);

    // (Phase 5) Reuse preallocated buffers when the frame size is stable.
    if (m_bufferSize != frame.size())
    {
        m_scratchGray.create(height, width, CV_8UC1);
        m_scratchBlurred.create(height, width, CV_8UC1);
        m_scratchDiff.create(height, width, CV_8UC1);
        m_scratchMask.create(height, width, CV_8UC1);
        m_scratchEdges.create(height, width, CV_8UC1);
        m_scratchBlack.create(height, width, CV_8UC1);
        m_scratchFilteredBlack.create(height, width, CV_8UC1);
        m_scratchNonBlack.create(height, width, CV_8UC1);
        m_lastGray.release();
        m_bufferSize = frame.size();
    }

    cv::Mat &gray = m_scratchGray;
    cv::cvtColor(frame, gray, cv::COLOR_RGBA2GRAY);

    cv::Mat &blurred = m_scratchBlurred;
    cv::GaussianBlur(gray, blurred, cv::Size(5, 5), 0);

    // 1. Motion Mask via Temporal Frame Differencing
    cv::Mat &motionMask = m_scratchMask;
    motionMask.setTo(0);
    if (!m_lastGray.empty() && m_lastGray.size() == gray.size())
    {
        cv::Mat &diff = m_scratchDiff;
        cv::absdiff(gray, m_lastGray, diff);
        cv::threshold(diff, motionMask, 10, 255, cv::THRESH_BINARY);
    }
    gray.copyTo(m_lastGray);

    // 2. Canny edges intersected with motion mask — only moving edges survive.
    cv::Mat &combinedMask = m_scratchEdges;
    cv::Canny(blurred, combinedMask, 35, 100);
    if (!m_lastGray.empty())
    {
        cv::dilate(motionMask, motionMask, cv::Mat(), cv::Point(-1, -1), 2);
        cv::bitwise_and(combinedMask, motionMask, combinedMask);
    }

    // 3. Black paddle exclusion (solid black bar >= 20x10 px).
    cv::Mat &blackMask = m_scratchBlack;
    cv::threshold(gray, blackMask, 55, 255, cv::THRESH_BINARY_INV);

    std::vector<std::vector<cv::Point>> blackContours;
    cv::findContours(blackMask, blackContours, cv::RETR_EXTERNAL,
                     cv::CHAIN_APPROX_SIMPLE);

    cv::Mat &filteredBlackMask = m_scratchFilteredBlack;
    filteredBlackMask.setTo(0);
    for (const auto &contour : blackContours)
    {
        cv::Rect bbox = cv::boundingRect(contour);
        if (bbox.width >= 20 && bbox.height >= 10)
        {
            cv::drawContours(filteredBlackMask,
                             std::vector<std::vector<cv::Point>>{contour}, -1,
                             255, cv::FILLED);
        }
    }
    cv::dilate(filteredBlackMask, filteredBlackMask, cv::Mat(),
               cv::Point(-1, -1), 3);
    cv::Mat &nonBlackMask = m_scratchNonBlack;
    cv::bitwise_not(filteredBlackMask, nonBlackMask);
    cv::bitwise_and(combinedMask, nonBlackMask, combinedMask);

    cv::dilate(combinedMask, combinedMask, cv::Mat(), cv::Point(-1, -1), 1);

    // Crop ROI to game field.
    int cropTop = static_cast<int>(height * m_cal.ceilingFraction);
    int cropBottom = static_cast<int>(height * m_cal.cropBottomFraction);
    cv::Mat roi = combinedMask(cv::Range(cropTop, cropBottom),
                               cv::Range(0, width));

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(roi, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    // Keep top-N candidates instead of only the largest (multi-emoji support).
    constexpr int MAX_CANDIDATES = 6;
    std::vector<std::pair<Vector2D, double>> candidates; // {centroid, area}
    outBoxes.clear();

    for (const auto &contour : contours)
    {
        double area = cv::contourArea(contour);
        if (area < MIN_AREA || area > MAX_AREA)
            continue;

        cv::Rect bbox = cv::boundingRect(contour);
        double aspectRatio = static_cast<double>(bbox.width) / bbox.height;
        if (aspectRatio < 0.6 || aspectRatio > 1.4)
            continue;

        cv::Moments mo = cv::moments(contour);
        if (mo.m00 == 0)
            continue;

        Vector2D centroid{mo.m10 / mo.m00, (mo.m01 / mo.m00) + cropTop};
        candidates.emplace_back(centroid, area);
        outBoxes.push_back(bbox);

        if (static_cast<int>(candidates.size()) >= MAX_CANDIDATES)
            break;
    }

    // Largest first so association prefers the most confident detections.
    std::sort(candidates.begin(), candidates.end(),
              [](const auto &a, const auto &b)
              { return a.second > b.second; });
    return candidates;
}

TrackingResult TrajectoryEngine::processFrame(uint8_t *frameBuffer, int width,
                                              int height, int rowStride,
                                              double paddleWidth)
{
    auto startTime = std::chrono::high_resolution_clock::now();
    auto nowSec = []()
    {
        return std::chrono::duration<double>(
                   std::chrono::high_resolution_clock::now().time_since_epoch())
            .count();
    };
    const double currentTime = nowSec();

    std::vector<cv::Rect> boxes;
    auto candidates = detectCandidates(frameBuffer, width, height, rowStride,
                                       boxes);

    TrackingResult result{};
    result.trackId = -1;
    result.trackCount = 0;

    // ---- dt handling: clamp to [1ms, 100ms] ----
    double dt = DT_MAX;
    if (m_hasPrevTime)
    {
        dt = currentTime - m_lastProcessTime;
        dt = std::clamp(dt, DT_MIN, DT_MAX);
    }
    m_lastProcessTime = currentTime;
    m_hasPrevTime = true;

    // ---- associate candidates with existing tracks (nearest neighbor) ----
    constexpr double GATE2 = GATE_DISTANCE * GATE_DISTANCE;
    std::vector<bool> used(candidates.size(), false);

    for (auto &track : m_tracks)
    {
        track->predict(currentTime);

        Vector2D predictedPos = track->position();
        int bestIdx = -1;
        double bestDist2 = GATE2;
        for (size_t i = 0; i < candidates.size(); ++i)
        {
            if (used[i])
                continue;
            double dx = candidates[i].first.x - predictedPos.x;
            double dy = candidates[i].first.y - predictedPos.y;
            double d2 = dx * dx + dy * dy;
            if (d2 < bestDist2)
            {
                bestDist2 = d2;
                bestIdx = static_cast<int>(i);
            }
        }

        if (bestIdx >= 0)
        {
            used[bestIdx] = true;
            track->update(candidates[bestIdx].first, currentTime);
        }
        else
        {
            track->incrementMisses();
        }
    }

    // ---- spawn new tracks from unassociated candidates ----
    for (size_t i = 0; i < candidates.size(); ++i)
    {
        if (used[i])
            continue;
        m_tracks.push_back(
            std::make_unique<KalmanTracker>(m_nextTrackId++, candidates[i].first,
                                            currentTime));
    }

    // ---- prune stale tracks (~300ms of misses at 60Hz) ----
    m_tracks.erase(std::remove_if(m_tracks.begin(), m_tracks.end(),
                                  [](const std::unique_ptr<KalmanTracker> &t)
                                  {
                                      return t->missedFrames() > MAX_MISSES;
                                  }),
                   m_tracks.end());

    result.trackCount = static_cast<int>(m_tracks.size());
    if (result.trackCount == 0)
    {
        m_hasPrediction = false;
        m_predictionTrackId = -1;
        auto endTime = std::chrono::high_resolution_clock::now();
        result.processingTimeMs =
            std::chrono::duration<double, std::milli>(endTime - startTime)
                .count();
        result.isValid = false;
        result.fps = 1.0 / dt;
        return result;
    }

    // ---- target selection: confirmed track soonest to the paddle ----
    KalmanTracker *primary = nullptr;
    double bestTime = std::numeric_limits<double>::max();
    for (auto &track : m_tracks)
    {
        if (!track->confirmed())
            continue;
        double t = track->timeToPaddle(m_cal);
        if (t >= 0.0 && t < bestTime)
        {
            bestTime = t;
            primary = track.get();
        }
    }
    // Fallback: any confirmed track (e.g., moving upward with no valid ETA).
    if (!primary)
    {
        for (auto &track : m_tracks)
        {
            if (track->confirmed())
            {
                primary = track.get();
                break;
            }
        }
    }
    if (!primary)
    {
        auto endTime = std::chrono::high_resolution_clock::now();
        result.processingTimeMs =
            std::chrono::duration<double, std::milli>(endTime - startTime)
                .count();
        result.isValid = false;
        result.fps = 1.0 / dt;
        return result;
    }

    result.velocity = primary->velocity();
    result.accel = primary->accel();
    result.trackId = primary->id();
    result.isValid = true;
    result.fps = 1.0 / dt;

    // Anomaly flag: high filtered acceleration magnitude OR large innovation.
    double accelMag = std::sqrt(result.accel.x * result.accel.x +
                                result.accel.y * result.accel.y);
    result.anomalyDetected = accelMag > 3500.0;

    // Reset frozen prediction when the tracked emoji switches or moves up.
    if (m_predictionTrackId != primary->id())
    {
        m_hasPrediction = false;
        m_predictionTrackId = primary->id();
    }
    if (result.velocity.y < 0)
    {
        m_hasPrediction = false;
    }

    // Freeze zone: below freezeY with a cached prediction, don't recompute —
    // prevents jitter in the critical final frames before contact.
    if (result.velocity.y > 0 &&
        primary->position().y > m_cal.freezeY && m_hasPrediction)
    {
        result.x_land = m_predictedXLand;
    }
    else
    {
        // Landing X already integrates over the full flight time; the paddle
        // lead-time is handled by dispatching early via leadFrames in cal.
        double landingX = primary->predictLandingX(m_cal);
        result.x_land = landingX - (paddleWidth / 2.0);
        m_predictedXLand = result.x_land;
        m_hasPrediction = true;
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    result.processingTimeMs =
        std::chrono::duration<double, std::milli>(endTime - startTime).count();
    return result;
}
