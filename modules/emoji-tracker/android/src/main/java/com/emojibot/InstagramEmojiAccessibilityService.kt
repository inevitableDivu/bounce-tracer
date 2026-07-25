package com.emojibot

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

    /**
     * Injects horizontal paddle positioning stroke with sub-10ms latency
     */
    fun injectPaddleMove(targetX: Float, paddleY: Float) {
        val path = Path().apply {
            moveTo(targetX, paddleY)
            lineTo(targetX + 1f, paddleY)
        }

        val stroke = GestureDescription.StrokeDescription(path, 0L, 5L)
        val gesture = GestureDescription.Builder()
            .addStroke(stroke)
            .build()

        dispatchGesture(gesture, object : GestureResultCallback() {
            override fun onCompleted(gestureDescription: GestureDescription?) {
                super.onCompleted(gestureDescription)
            }
            override fun onCancelled(gestureDescription: GestureDescription?) {
                super.onCancelled(gestureDescription)
            }
        }, null)
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
