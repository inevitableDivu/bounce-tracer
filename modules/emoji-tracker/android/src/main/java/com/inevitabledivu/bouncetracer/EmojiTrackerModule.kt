package com.inevitabledivu.bouncetracer

import android.app.Activity
import android.content.Context
import android.content.Intent
import android.media.projection.MediaProjectionManager
import android.net.Uri
import android.provider.Settings
import com.facebook.react.bridge.ActivityEventListener
import com.facebook.react.bridge.BaseActivityEventListener
import com.facebook.react.bridge.Promise
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.ReactContextBaseJavaModule
import com.facebook.react.bridge.ReactMethod

class EmojiTrackerModule(private val reactContext: ReactApplicationContext) :
    ReactContextBaseJavaModule(reactContext) {

    private var floatingHUDView: FloatingHUDView? = null
    private var pendingPromise: Promise? = null

    private val activityEventListener: ActivityEventListener = object : BaseActivityEventListener() {
        override fun onActivityResult(activity: Activity, requestCode: Int, resultCode: Int, data: Intent?) {
            if (requestCode == REQUEST_MEDIA_PROJECTION) {
                if (resultCode == Activity.RESULT_OK && data != null) {
                    val metrics = reactContext.resources.displayMetrics
                    val serviceIntent = Intent(reactContext, ScreenCaptureService::class.java).apply {
                        putExtra("RESULT_CODE", resultCode)
                        putExtra("RESULT_DATA", data)
                        putExtra("WIDTH", metrics.widthPixels)
                        putExtra("HEIGHT", metrics.heightPixels)
                        putExtra("DPI", metrics.densityDpi)
                    }
                    if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.O) {
                        reactContext.startForegroundService(serviceIntent)
                    } else {
                        reactContext.startService(serviceIntent)
                    }
                    floatingHUDView?.show()
                    pendingPromise?.resolve(true)
                } else {
                    pendingPromise?.reject("PROJECTION_DENIED", "Screen capture permission was denied")
                }
                pendingPromise = null
            }
        }
    }

    init {
        reactContext.addActivityEventListener(activityEventListener)
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
        val activity = reactContext.currentActivity
        if (activity == null) {
            promise.reject("NO_ACTIVITY", "Activity does not exist")
            return
        }

        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.M && !Settings.canDrawOverlays(reactContext)) {
            requestOverlayPermission()
            promise.reject("OVERLAY_PERMISSION_REQUIRED", "Overlay permission required before starting capture")
            return
        }

        if (floatingHUDView == null) {
            floatingHUDView = FloatingHUDView(reactContext)
        }

        val projectionManager = reactContext.getSystemService(Context.MEDIA_PROJECTION_SERVICE) as MediaProjectionManager
        pendingPromise = promise
        activity.startActivityForResult(
            projectionManager.createScreenCaptureIntent(),
            REQUEST_MEDIA_PROJECTION
        )
    }

    @ReactMethod
    fun stopScreenCapture() {
        val intent = Intent(reactContext, ScreenCaptureService::class.java)
        reactContext.stopService(intent)
        floatingHUDView?.hide()
    }

    @ReactMethod
    fun setPaddleWidth(width: Double) {
        // Updated in C++ NativeModule
    }

    companion object {
        private const val REQUEST_MEDIA_PROJECTION = 2001
    }
}
