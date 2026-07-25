package com.inevitabledivu.bouncetracer

import android.accessibilityservice.AccessibilityService
import android.accessibilityservice.GestureDescription
import android.graphics.Path
import android.view.accessibility.AccessibilityEvent

class InstagramEmojiAccessibilityService : AccessibilityService() {

    override fun onAccessibilityEvent(event: AccessibilityEvent?) {}

    override fun onInterrupt() {}

    override fun onServiceConnected() {
        super.onServiceConnected()
        instance = this
    }

    private var lastGestureTime = 0L
    private var lastTargetX = 0f
    private val MIN_MOVE_THRESHOLD = 12f // pixels
    private val GESTURE_MIN_INTERVAL_MS = 60L // strict 60ms throttle to prevent Android queue bloat

    /**
     * Injects horizontal paddle positioning stroke with sub-10ms latency
     */
    fun injectPaddleMove(targetX: Float, paddleY: Float) {
        val now = System.currentTimeMillis()
        val timeDiff = now - lastGestureTime
        val posDiff = Math.abs(targetX - lastTargetX)

        // Throttle gesture dispatches so we don't clog the Android Accessibility queue
        if (timeDiff < GESTURE_MIN_INTERVAL_MS) {
            return
        }
        if (posDiff < MIN_MOVE_THRESHOLD && timeDiff < 200L) {
            return // Skip small adjustments unless it has been a while
        }

        lastGestureTime = now
        lastTargetX = targetX

        val path = Path().apply {
            moveTo(targetX, paddleY)
            lineTo(targetX + 1f, paddleY)
        }

        val stroke = GestureDescription.StrokeDescription(path, 0L, 5L)
        val gesture = GestureDescription.Builder()
            .addStroke(stroke)
            .build()

        dispatchGesture(gesture, null, null)
    }

    companion object {
        var instance: InstagramEmojiAccessibilityService? = null
            private set

        @JvmStatic
        fun dispatchNativeTouch(x: Float, y: Float) {
            instance?.injectPaddleMove(x, y)
        }
    }
}
