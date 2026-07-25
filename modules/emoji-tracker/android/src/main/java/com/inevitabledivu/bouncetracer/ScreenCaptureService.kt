package com.inevitabledivu.bouncetracer

import android.app.Service
import android.content.Intent
import android.graphics.PixelFormat
import android.hardware.display.DisplayManager
import android.hardware.display.VirtualDisplay
import android.media.ImageReader
import android.media.projection.MediaProjection
import android.os.IBinder
import java.nio.ByteBuffer

class ScreenCaptureService : Service() {

    private var mediaProjection: MediaProjection? = null
    private var virtualDisplay: VirtualDisplay? = null
    private var imageReader: ImageReader? = null

    private external fun nativeProcessFrame(
        buffer: ByteBuffer,
        width: Int,
        height: Int,
        pixelStride: Int,
        rowStride: Int
    )

    fun startCapture(projection: MediaProjection, width: Int, height: Int, densityDpi: Int) {
        this.mediaProjection = projection
        imageReader = ImageReader.newInstance(width, height, PixelFormat.RGBA_8888, 2)

        virtualDisplay = mediaProjection?.createVirtualDisplay(
            "EmojiTrackerDisplay",
            width, height, densityDpi,
            DisplayManager.VIRTUAL_DISPLAY_FLAG_AUTO_MIRROR,
            imageReader?.surface, null, null
        )

        imageReader?.setOnImageAvailableListener({ reader ->
            val image = reader.acquireLatestImage() ?: return@setOnImageAvailableListener
            try {
                val planes = image.planes
                val buffer = planes[0].buffer
                val pixelStride = planes[0].pixelStride
                val rowStride = planes[0].rowStride

                // Pass direct ByteBuffer pointer to C++ NDK pipeline
                nativeProcessFrame(buffer, image.width, image.height, pixelStride, rowStride)
            } finally {
                image.close()
            }
        }, null)
    }

    override fun onBind(intent: Intent?): IBinder? = null

    companion object {
        init {
            System.loadLibrary("emojitracker")
        }
    }
}
