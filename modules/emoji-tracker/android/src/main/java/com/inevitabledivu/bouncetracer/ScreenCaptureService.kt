package com.inevitabledivu.bouncetracer

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.Service
import android.content.Context
import android.content.Intent
import android.content.pm.ServiceInfo
import android.graphics.PixelFormat
import android.hardware.display.DisplayManager
import android.hardware.display.VirtualDisplay
import android.media.ImageReader
import android.media.projection.MediaProjection
import android.media.projection.MediaProjectionManager
import android.os.Build
import android.os.Handler
import android.os.IBinder
import android.os.Looper
import android.util.Log
import android.view.Display
import android.view.WindowManager
import androidx.core.app.NotificationCompat
import java.nio.ByteBuffer

class ScreenCaptureService : Service() {

    private var mediaProjection: MediaProjection? = null
    private var virtualDisplay: VirtualDisplay? = null
    private var imageReader: ImageReader? = null
    private val TAG = "EmojiTracker_Service"

    private external fun nativeProcessFrame(
        buffer: ByteBuffer,
        width: Int,
        height: Int,
        pixelStride: Int,
        rowStride: Int
    )

    private external fun nativeSetCalibration(
        screenWidth: Int,
        screenHeight: Int,
        paddleY: Double,
        frameRateHz: Double,
        leadFrames: Double,
        restitution: Double
    )

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        Log.d(TAG, "onStartCommand: Starting ScreenCaptureService")
        try {
            startForegroundServiceNotification()

            val resultCode = intent?.getIntExtra("RESULT_CODE", 0) ?: 0
            val resultData = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                intent?.getParcelableExtra("RESULT_DATA", Intent::class.java)
            } else {
                @Suppress("DEPRECATION")
                intent?.getParcelableExtra("RESULT_DATA")
            }

            val width = intent?.getIntExtra("WIDTH", 1080) ?: 1080
            val height = intent?.getIntExtra("HEIGHT", 2400) ?: 2400
            val densityDpi = intent?.getIntExtra("DPI", 400) ?: 400

            Log.d(TAG, "onStartCommand: Display Metrics W=$width H=$height DPI=$densityDpi")

            if (resultCode != 0 && resultData != null) {
                Log.d(TAG, "onStartCommand: Valid result code ($resultCode) and data. Getting MediaProjection...")
                val projectionManager = getSystemService(Context.MEDIA_PROJECTION_SERVICE) as MediaProjectionManager
                val projection = projectionManager.getMediaProjection(resultCode, resultData)
                if (projection != null) {
                    Log.d(TAG, "onStartCommand: MediaProjection successfully acquired.")
                    // Register mandatory callback required on Android 14+ (API 34+) to avoid process termination
                    projection.registerCallback(object : MediaProjection.Callback() {
                        override fun onStop() {
                            Log.d(TAG, "MediaProjection.Callback: onStop triggered. Stopping capture.")
                            stopCapture()
                        }
                    }, Handler(Looper.getMainLooper()))

                    startCapture(projection, width, height, densityDpi)
                } else {
                    Log.e(TAG, "onStartCommand: MediaProjection returned null!")
                }
            } else {
                Log.e(TAG, "onStartCommand: Invalid result code or null intent data")
            }
        } catch (e: Exception) {
            Log.e(TAG, "onStartCommand: Exception caught during service start", e)
            // Prevent crash if ForegroundServiceStartNotAllowedException or SecurityException is thrown
            stopSelf()
        }

        return START_STICKY
    }

    private fun startForegroundServiceNotification() {
        Log.d(TAG, "startForegroundServiceNotification: Building notification...")
        val channelId = "emoji_tracker_service_channel"
        val channelName = "Emoji Game Tracker Background Service"

        val notificationManager = getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val channel = NotificationChannel(channelId, channelName, NotificationManager.IMPORTANCE_LOW)
            notificationManager.createNotificationChannel(channel)
        }

        val notification: Notification = NotificationCompat.Builder(this, channelId)
            .setContentTitle("Bounce Tracker Active")
            .setContentText("Tracking Instagram Emoji Game in background...")
            .setSmallIcon(android.R.drawable.ic_menu_camera)
            .setPriority(NotificationCompat.PRIORITY_LOW)
            .build()

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            Log.d(TAG, "startForegroundServiceNotification: Calling startForeground with MEDIA_PROJECTION type")
            startForeground(1001, notification, ServiceInfo.FOREGROUND_SERVICE_TYPE_MEDIA_PROJECTION)
        } else {
            Log.d(TAG, "startForegroundServiceNotification: Calling startForeground (legacy)")
            startForeground(1001, notification)
        }
    }

    private fun startCapture(projection: MediaProjection, width: Int, height: Int, densityDpi: Int) {
        Log.d(TAG, "startCapture: Initializing ImageReader and VirtualDisplay")
        this.mediaProjection = projection

        // Phase 3.6: detect actual display refresh rate instead of assuming 60Hz.
        val display: Display? = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            (getSystemService(Context.DISPLAY_MANAGER) as DisplayManager).getDisplay(Display.DEFAULT_DISPLAY)
        } else {
            @Suppress("DEPRECATION")
            (getSystemService(Context.WINDOW_SERVICE) as WindowManager).defaultDisplay
        }
        var refreshHz = 60.0
        if (display != null) {
            val reported = display.refreshRate.toDouble()
            // Sanity-check the reported rate; fall back to 60Hz if implausible.
            if (reported in 30.0..240.0) refreshHz = reported
        }
        Log.d(TAG, "startCapture: Detected refresh rate ${refreshHz}Hz")

        // Push calibration (screen geometry + refresh cadence) to the native engine.
        try {
            nativeSetCalibration(width, height, 1650.0, refreshHz, 5.0, 1.0)
        } catch (e: UnsatisfiedLinkError) {
            Log.e(TAG, "startCapture: nativeSetCalibration not available", e)
        }

        imageReader = ImageReader.newInstance(width, height, PixelFormat.RGBA_8888, 2)

        try {
            virtualDisplay = mediaProjection?.createVirtualDisplay(
                "EmojiTrackerDisplay",
                width, height, densityDpi,
                DisplayManager.VIRTUAL_DISPLAY_FLAG_AUTO_MIRROR,
                imageReader?.surface, null, null
            )
            Log.d(TAG, "startCapture: VirtualDisplay created successfully.")
        } catch (e: Exception) {
            Log.e(TAG, "startCapture: Failed to create VirtualDisplay", e)
        }

        var frameCount = 0
        imageReader?.setOnImageAvailableListener({ reader ->
            val image = reader.acquireLatestImage() ?: return@setOnImageAvailableListener
            frameCount++
            if (frameCount % 120 == 0) {
                Log.d(TAG, "ImageReader: Processed $frameCount frames.")
            }
            try {
                val planes = image.planes
                val buffer = planes[0].buffer
                val pixelStride = planes[0].pixelStride
                val rowStride = planes[0].rowStride

                nativeProcessFrame(buffer, image.width, image.height, pixelStride, rowStride)
            } catch (e: Exception) {
                Log.e(TAG, "ImageReader: Unhandled exception during frame processing", e)
            } finally {
                image.close()
            }
        }, null)
    }

    private fun stopCapture() {
        Log.d(TAG, "stopCapture called")
        virtualDisplay?.release()
        virtualDisplay = null
        imageReader?.close()
        imageReader = null
        mediaProjection?.stop()
        mediaProjection = null
    }

    override fun onDestroy() {
        Log.d(TAG, "onDestroy called")
        super.onDestroy()
        stopCapture()
    }

    override fun onBind(intent: Intent?): IBinder? = null

    companion object {
        init {
            try {
                System.loadLibrary("emojitracker")
            } catch (e: Throwable) {
                // Ignore error here, we can't log using Android Log if it fails before Android classes load,
                // but this runs in companion init block.
            }
        }
    }
}
