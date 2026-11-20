package io.coreflow

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.content.Context
import android.content.Intent
import android.graphics.Color
import android.graphics.PixelFormat
import android.graphics.Typeface
import android.graphics.drawable.GradientDrawable
import android.os.IBinder
import android.provider.Settings
import android.util.TypedValue
import android.view.Gravity
import android.view.MotionEvent
import android.view.View
import android.view.WindowManager
import android.widget.LinearLayout
import android.widget.TextView
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch
import java.util.Locale
import kotlin.math.abs

/**
 * Native Floating Game HUD overlay (SYSTEM_ALERT_WINDOW).
 * Shows live SoC temperature, render pressure, UClamp floor, active profile,
 * and a 1-tap A/B Pause/Resume button directly on top of any app or game.
 */
class CoreFlowOverlayService : Service() {

    companion object {
        private const val CHANNEL_ID = "coreflow_hud_channel"
        private const val NOTIF_ID = 5101
        const val ACTION_STOP = "io.coreflow.overlay.STOP"
        const val EXTRA_INTERVAL_MS = "interval_ms"

        private val _running = MutableStateFlow(false)
        val running: StateFlow<Boolean> = _running.asStateFlow()

        fun start(context: Context, intervalMs: Long = 1500L) {
            if (!Settings.canDrawOverlays(context)) return
            val intent = Intent(context, CoreFlowOverlayService::class.java)
                .putExtra(EXTRA_INTERVAL_MS, intervalMs.coerceIn(750L, 5000L))
            context.startForegroundService(intent)
        }

        fun stop(context: Context) {
            context.stopService(Intent(context, CoreFlowOverlayService::class.java))
        }
    }

    private val scope = CoroutineScope(SupervisorJob() + Dispatchers.Main.immediate)
    private var pollJob: Job? = null
    private var windowManager: WindowManager? = null
    private var hudView: View? = null
    private var titleText: TextView? = null
    private var metricsText: TextView? = null
    private var togglePill: TextView? = null
    private var isPausedState = false
    private var intervalMs = 1500L

    override fun onBind(intent: Intent?): IBinder? = null

    override fun onCreate() {
        super.onCreate()
        createNotificationChannel()
        startForeground(NOTIF_ID, buildNotification())
        if (!Settings.canDrawOverlays(this)) {
            stopSelf()
            return
        }
        attachHudOverlay()
        _running.value = true
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        if (intent?.action == ACTION_STOP) {
            stopSelf()
            return START_NOT_STICKY
        }
        intervalMs = intent?.getLongExtra(EXTRA_INTERVAL_MS, 1500L)?.coerceIn(750L, 5000L) ?: 1500L
        startPolling()
        return START_STICKY
    }

    override fun onDestroy() {
        pollJob?.cancel()
        hudView?.let { view ->
            runCatching { windowManager?.removeView(view) }
        }
        hudView = null
        _running.value = false
        scope.cancel()
        super.onDestroy()
    }

    private fun attachHudOverlay() {
        val wm = getSystemService(WINDOW_SERVICE) as WindowManager
        windowManager = wm

        val density = resources.displayMetrics.density
        fun dp(v: Int): Int = (v * density).toInt()

        val bg = GradientDrawable().apply {
            shape = GradientDrawable.RECTANGLE
            cornerRadius = dp(16).toFloat()
            setColor(Color.argb(215, 18, 22, 30))
            setStroke(dp(1), Color.argb(90, 96, 165, 250))
        }

        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            background = bg
            setPadding(dp(12), dp(8), dp(12), dp(8))
        }

        val topRow = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
        }

        val title = TextView(this).apply {
            text = "CoreFlow · Live"
            setTextColor(Color.WHITE)
            setTextSize(TypedValue.COMPLEX_UNIT_SP, 11f)
            typeface = Typeface.create(Typeface.DEFAULT, Typeface.BOLD)
        }
        titleText = title

        val pillBg = GradientDrawable().apply {
            shape = GradientDrawable.RECTANGLE
            cornerRadius = dp(10).toFloat()
            setColor(Color.argb(200, 37, 99, 235))
        }
        val pill = TextView(this).apply {
            text = "A/B"
            setTextColor(Color.WHITE)
            setTextSize(TypedValue.COMPLEX_UNIT_SP, 10f)
            typeface = Typeface.create(Typeface.DEFAULT, Typeface.BOLD)
            background = pillBg
            setPadding(dp(8), dp(3), dp(8), dp(3))
            val lp = LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT,
                LinearLayout.LayoutParams.WRAP_CONTENT,
            ).apply {
                marginStart = dp(10)
            }
            layoutParams = lp
            setOnClickListener {
                scope.launch {
                    val targetPaused = !isPausedState
                    CoreFlowManager.setPaused(targetPaused)
                    refreshOnce()
                }
            }
        }
        togglePill = pill

        topRow.addView(title)
        topRow.addView(pill)

        val metrics = TextView(this).apply {
            text = "Connecting..."
            setTextColor(Color.rgb(203, 213, 225))
            setTextSize(TypedValue.COMPLEX_UNIT_SP, 10.5f)
            typeface = Typeface.MONOSPACE
            setPadding(0, dp(3), 0, 0)
        }
        metricsText = metrics

        root.addView(topRow)
        root.addView(metrics)

        val params = WindowManager.LayoutParams(
            WindowManager.LayoutParams.WRAP_CONTENT,
            WindowManager.LayoutParams.WRAP_CONTENT,
            WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY,
            WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE or WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN,
            PixelFormat.TRANSLUCENT,
        ).apply {
            gravity = Gravity.TOP or Gravity.START
            x = dp(16)
            y = dp(96)
        }

        var startX = 0
        var startY = 0
        var downRawX = 0f
        var downRawY = 0f

        root.setOnTouchListener { _, event ->
            when (event.action) {
                MotionEvent.ACTION_DOWN -> {
                    startX = params.x
                    startY = params.y
                    downRawX = event.rawX
                    downRawY = event.rawY
                    true
                }

                MotionEvent.ACTION_MOVE -> {
                    val dx = (event.rawX - downRawX).toInt()
                    val dy = (event.rawY - downRawY).toInt()
                    if (abs(dx) > 3 || abs(dy) > 3) {
                        params.x = startX + dx
                        params.y = startY + dy
                        runCatching { wm.updateViewLayout(root, params) }
                    }
                    true
                }

                else -> false
            }
        }

        hudView = root
        wm.addView(root, params)
    }

    private fun startPolling() {
        pollJob?.cancel()
        pollJob = scope.launch {
            while (isActive) {
                refreshOnce()
                delay(intervalMs)
            }
        }
    }

    private suspend fun refreshOnce() {
        val status = CoreFlowManager.getStatus().getOrNull()
        if (status == null) {
            titleText?.text = "CoreFlow · Offline"
            metricsText?.text = "Daemon not responding"
            return
        }

        isPausedState = status.inspectionMode
        val stateLabel = when {
            !status.isRunning -> "STOPPED"
            status.inspectionMode -> "PAUSED (OEM)"
            status.thermalGuardActive -> "THERMAL GUARD"
            else -> status.effectiveProfile.label.uppercase(Locale.US)
        }
        titleText?.text = "CF · $stateLabel"

        val pillBg = togglePill?.background as? GradientDrawable
        if (status.inspectionMode) {
            togglePill?.text = "RESUME"
            pillBg?.setColor(Color.argb(220, 217, 119, 6))
        } else {
            togglePill?.text = "PAUSE"
            pillBg?.setColor(Color.argb(200, 37, 99, 235))
        }

        val tempStr = if (status.socTempCelsius >= 0f) {
            String.format(Locale.US, "%.1f°C", status.socTempCelsius)
        } else {
            "--°C"
        }
        val rpPct = (status.renderPressure * 100f).toInt().coerceIn(0, 100)
        val scalePct = (status.capacityScale * 100f).toInt().coerceIn(0, 100)
        metricsText?.text = "$tempStr | RP $rpPct% | UC ${status.uclampMin} | $scalePct%"
    }

    private fun createNotificationChannel() {
        val nm = getSystemService(NotificationManager::class.java)
        val channel = NotificationChannel(
            CHANNEL_ID,
            "CoreFlow Live HUD",
            NotificationManager.IMPORTANCE_LOW,
        ).apply {
            description = "Keeps the draggable CoreFlow Game HUD overlay active."
            setShowBadge(false)
        }
        nm.createNotificationChannel(channel)
    }

    private fun buildNotification(): Notification {
        val openIntent = PendingIntent.getActivity(
            this,
            0,
            Intent(this, MainActivity::class.java),
            PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT,
        )
        val stopIntent = PendingIntent.getService(
            this,
            1,
            Intent(this, CoreFlowOverlayService::class.java).setAction(ACTION_STOP),
            PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT,
        )
        return Notification.Builder(this, CHANNEL_ID)
            .setSmallIcon(R.drawable.ic_coreflow_tile)
            .setContentTitle("CoreFlow Live HUD Active")
            .setContentText("Drag the floating HUD or tap Stop to dismiss.")
            .setContentIntent(openIntent)
            .setOngoing(true)
            .addAction(
                Notification.Action.Builder(null, "Stop HUD", stopIntent).build(),
            )
            .build()
    }
}
