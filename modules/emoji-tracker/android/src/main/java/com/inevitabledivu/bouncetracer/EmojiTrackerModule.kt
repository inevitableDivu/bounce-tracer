package com.inevitabledivu.bouncetracer

import android.app.Activity
import android.content.Context
import android.content.Intent
import android.media.projection.MediaProjectionManager
import android.net.Uri
import android.provider.Settings
import android.util.Log
import com.facebook.react.bridge.ActivityEventListener
import com.facebook.react.bridge.BaseActivityEventListener
import com.facebook.react.bridge.Promise
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.ReactContextBaseJavaModule
import com.facebook.react.bridge.ReactMethod
import com.facebook.react.bridge.Arguments
import com.facebook.react.bridge.WritableMap

class EmojiTrackerModule(private val reactContext: ReactApplicationContext) :
    ReactContextBaseJavaModule(reactContext) {

    private var floatingHUDView: FloatingHUDView? = null
    private var pendingPromise: Promise? = null
    private val TAG = "EmojiTracker_Module"

    private var lastFps = 0.0
    private var lastLatency = 0.0
    private var lastXLand = 0.0
    private var lastVx = 0.0
    private var lastVy = 0.0
    private var lastAx = 0.0
    private var lastAy = 0.0
    private var lastTrackId = -1
    private var lastTrackCount = 0
    private var lastIsTracking = false
    private var lastAnomaly = false

    private val activityEventListener: ActivityEventListener = object : BaseActivityEventListener() {
        override fun onActivityResult(activity: Activity, requestCode: Int, resultCode: Int, data: Intent?) {
            if (requestCode == REQUEST_MEDIA_PROJECTION) {
                Log.d(TAG, "onActivityResult: REQUEST_MEDIA_PROJECTION received, resultCode=$resultCode")
                if (resultCode == Activity.RESULT_OK && data != null) {
                    Log.d(TAG, "onActivityResult: Permission granted. Creating intent for ScreenCaptureService")
                    val metrics = reactContext.resources.displayMetrics
                    val serviceIntent = Intent(reactContext, ScreenCaptureService::class.java).apply {
                        putExtra("RESULT_CODE", resultCode)
                        putExtra("RESULT_DATA", data)
                        putExtra("WIDTH", metrics.widthPixels)
                        putExtra("HEIGHT", metrics.heightPixels)
                        putExtra("DPI", metrics.densityDpi)
                    }
                    try {
                        Log.d(TAG, "onActivityResult: Starting ScreenCaptureService...")
                        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.O) {
                            reactContext.startForegroundService(serviceIntent)
                        } else {
                            reactContext.startService(serviceIntent)
                        }
                        Log.d(TAG, "onActivityResult: ScreenCaptureService started successfully.")
                    } catch (e: Exception) {
                        Log.e(TAG, "onActivityResult: Failed to start service", e)
                        // Catch ForegroundServiceStartNotAllowedException or SecurityException
                        pendingPromise?.reject("SERVICE_START_FAILED", "Failed to start background capture service")
                        pendingPromise = null
                        return
                    }
                    
                    Log.d(TAG, "onActivityResult: Showing Floating HUD...")
                    floatingHUDView?.show()
                    Log.d(TAG, "onActivityResult: Resolving promise...")
                    pendingPromise?.resolve(true)
                } else {
                    Log.e(TAG, "onActivityResult: Permission denied or data is null.")
                    pendingPromise?.reject("PROJECTION_DENIED", "Screen capture permission was denied")
                }
                pendingPromise = null
            }
        }
    }

    init {
        Log.d(TAG, "Initializing EmojiTrackerModule")
        reactContext.addActivityEventListener(activityEventListener)
        instance = this
    }

    override fun getName(): String = "EmojiTrackerModule"

    @ReactMethod
    fun isOverlayPermissionGranted(promise: Promise) {
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.M) {
            promise.resolve(Settings.canDrawOverlays(reactContext))
        } else {
            promise.resolve(true)
        }
    }

    @ReactMethod
    fun requestOverlayPermission() {
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.M && !Settings.canDrawOverlays(reactContext)) {
            val intent = Intent(
                Settings.ACTION_MANAGE_OVERLAY_PERMISSION,
                Uri.parse("package:${reactContext.packageName}")
            ).apply {
                addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
            }
            reactContext.startActivity(intent)
        }
    }

    @ReactMethod
    fun requestAccessibilityPermission() {
        val intent = Intent(Settings.ACTION_ACCESSIBILITY_SETTINGS).apply {
            addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
        }
        reactContext.startActivity(intent)
    }

    @ReactMethod
    fun startScreenCapture(width: Double, height: Double, promise: Promise) {
        Log.d(TAG, "startScreenCapture called from JS")
        val activity = reactContext.currentActivity
        if (activity == null) {
            Log.e(TAG, "startScreenCapture: Activity is null!")
            promise.reject("NO_ACTIVITY", "Activity does not exist")
            return
        }

        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.M && !Settings.canDrawOverlays(reactContext)) {
            Log.e(TAG, "startScreenCapture: Overlay permission not granted.")
            requestOverlayPermission()
            promise.reject("OVERLAY_PERMISSION_REQUIRED", "Overlay permission required before starting capture")
            return
        }

        if (floatingHUDView == null) {
            Log.d(TAG, "startScreenCapture: Initializing FloatingHUDView")
            floatingHUDView = FloatingHUDView(reactContext)
        }

        Log.d(TAG, "startScreenCapture: Requesting MediaProjection Intent")
        val projectionManager = reactContext.getSystemService(Context.MEDIA_PROJECTION_SERVICE) as MediaProjectionManager
        pendingPromise = promise
        
        try {
            activity.startActivityForResult(
                projectionManager.createScreenCaptureIntent(),
                REQUEST_MEDIA_PROJECTION
            )
            Log.d(TAG, "startScreenCapture: Intent launched successfully")
        } catch (e: Exception) {
            Log.e(TAG, "startScreenCapture: Failed to launch intent", e)
            promise.reject("INTENT_FAILED", "Failed to launch screen capture intent")
        }
    }

    @ReactMethod
    fun stopScreenCapture() {
        Log.d(TAG, "stopScreenCapture called from JS")
        val intent = Intent(reactContext, ScreenCaptureService::class.java)
        reactContext.stopService(intent)
        floatingHUDView?.hide()
    }

    @ReactMethod
    fun getLatestTelemetry(promise: Promise) {
        val map = Arguments.createMap().apply {
            putDouble("fps", lastFps)
            putDouble("processingTimeMs", lastLatency)
            putDouble("predictedXLand", lastXLand)
            putDouble("velocityX", lastVx)
            putDouble("velocityY", lastVy)
            putDouble("accelX", lastAx)
            putDouble("accelY", lastAy)
            putInt("trackId", lastTrackId)
            putInt("trackCount", lastTrackCount)
            putBoolean("isTracking", lastIsTracking)
            putBoolean("anomalyDetected", lastAnomaly)
        }
        promise.resolve(map)
    }

    @ReactMethod
    fun setPaddleWidth(width: Double) {
        // Updated in C++ NativeModule
    }

    @ReactMethod
    fun setCalibration(
        screenWidth: Double,
        screenHeight: Double,
        paddleY: Double,
        frameRateHz: Double,
        leadFrames: Double,
        restitution: Double
    ) {
        // Calibration is applied natively via JNI (nativeSetCalibration);
        // this JS entry point exists for future direct use.
    }

    companion object {
        private const val REQUEST_MEDIA_PROJECTION = 2001

        @JvmStatic
        var instance: EmojiTrackerModule? = null
            private set

        @JvmStatic
        fun updateHUD(fps: Double, latencyMs: Double, xLand: Double, isTracking: Boolean) {
            instance?.let { inst ->
                inst.lastFps = fps
                inst.lastLatency = latencyMs
                inst.lastXLand = xLand
                inst.lastIsTracking = isTracking
                inst.floatingHUDView?.updateTelemetry(fps, latencyMs, xLand, isTracking)
            }
        }

        @JvmStatic
        fun updateExtendedTelemetry(
            vx: Double,
            vy: Double,
            ax: Double,
            ay: Double,
            trackId: Int,
            trackCount: Int,
            anomaly: Boolean
        ) {
            instance?.let { inst ->
                inst.lastVx = vx
                inst.lastVy = vy
                inst.lastAx = ax
                inst.lastAy = ay
                inst.lastTrackId = trackId
                inst.lastTrackCount = trackCount
                inst.lastAnomaly = anomaly
            }
        }
    }
}
