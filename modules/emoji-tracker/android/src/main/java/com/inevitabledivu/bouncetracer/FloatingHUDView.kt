package com.inevitabledivu.bouncetracer

import android.content.Context
import android.graphics.Color
import android.graphics.PixelFormat
import android.os.Build
import android.view.Gravity
import android.view.MotionEvent
import android.view.View
import android.view.WindowManager
import android.widget.LinearLayout
import android.widget.TextView

class FloatingHUDView(private val context: Context) {

    private val windowManager: WindowManager =
        context.getSystemService(Context.WINDOW_SERVICE) as WindowManager

    private var hudView: View? = null
    private var params: WindowManager.LayoutParams? = null

    private var fpsTextView: TextView? = null
    private var latencyTextView: TextView? = null
    private var targetTextView: TextView? = null
    private var statusTextView: TextView? = null

    fun show() {
        if (hudView != null) return

        val layoutParamsType = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY
        } else {
            @Suppress("DEPRECATION")
            WindowManager.LayoutParams.TYPE_PHONE
        }

        params = WindowManager.LayoutParams(
            WindowManager.LayoutParams.WRAP_CONTENT,
            WindowManager.LayoutParams.WRAP_CONTENT,
            layoutParamsType,
            WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE or WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN,
            PixelFormat.TRANSLUCENT
        ).apply {
            gravity = Gravity.TOP or Gravity.END
            x = 30
            y = 200
        }

        val container = LinearLayout(context).apply {
            orientation = LinearLayout.VERTICAL
            setBackgroundColor(Color.parseColor("#E60F172A"))
            setPadding(32, 24, 32, 24)
        }

        val title = TextView(context).apply {
            text = "🤖 BOUNCE TRACER HUD"
            setTextColor(Color.parseColor("#38BDF8"))
            textSize = 13f
            setPadding(0, 0, 0, 12)
        }
        container.addView(title)

        fpsTextView = createValueTextView("FPS: 0.0")
        latencyTextView = createValueTextView("Latency: 0.00 ms")
        targetTextView = createValueTextView("Target X: 0.0 px")
        statusTextView = createValueTextView("Status: READY")

        container.addView(fpsTextView)
        container.addView(latencyTextView)
        container.addView(targetTextView)
        container.addView(statusTextView)

        // Make HUD Draggable
        container.setOnTouchListener(object : View.OnTouchListener {
            private var initialX = 0
            private var initialY = 0
            private var initialTouchX = 0f
            private var initialTouchY = 0f

            override fun onTouch(v: View?, event: MotionEvent): Boolean {
                when (event.action) {
                    MotionEvent.ACTION_DOWN -> {
                        initialX = params?.x ?: 0
                        initialY = params?.y ?: 0
                        initialTouchX = event.rawX
                        initialTouchY = event.rawY
                        return true
                    }
                    MotionEvent.ACTION_MOVE -> {
                        params?.x = initialX - (event.rawX - initialTouchX).toInt()
                        params?.y = initialY + (event.rawY - initialTouchY).toInt()
                        windowManager.updateViewLayout(container, params)
                        return true
                    }
                }
                return false
            }
        })

        hudView = container
        windowManager.addView(hudView, params)
    }

    private fun createValueTextView(textVal: String): TextView {
        return TextView(context).apply {
            text = textVal
            setTextColor(Color.WHITE)
            textSize = 11f
            setPadding(0, 2, 0, 2)
        }
    }

    fun updateTelemetry(fps: Double, latencyMs: Double, xLand: Double, isTracking: Boolean) {
        hudView?.post {
            fpsTextView?.text = String.format("FPS: %.1f", fps)
            latencyTextView?.text = String.format("Latency: %.2f ms", latencyMs)
            targetTextView?.text = String.format("Target X: %.1f px", xLand)
            statusTextView?.text = if (isTracking) "Status: TRACKING 🎯" else "Status: SEARCHING 🔍"
            statusTextView?.setTextColor(if (isTracking) Color.GREEN else Color.YELLOW)
        }
    }

    fun hide() {
        if (hudView != null) {
            windowManager.removeView(hudView)
            hudView = null
        }
    }
}
