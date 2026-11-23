package io.coreflow

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.content.Context
import android.content.Intent
import android.content.pm.ServiceInfo
import android.graphics.Color
import android.graphics.PixelFormat
import android.graphics.Typeface
import android.graphics.drawable.GradientDrawable
import android.os.Build
import android.os.IBinder
import android.os.SystemClock
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
import kotlin.math.roundToInt

/**
 * CoreFlow v6.0 NextGen Floating Game HUD + Expandable Mini Game Panel.
 * - Real SurfaceFlinger FPS counter (shows "--" if unavailable, never fabricates FPS).
 * - Live SoC Temp, Render Pressure, UClamp Floor, and active mode.
 * - Tap the pill to expand a quick Game Panel with 4-profile switcher, 1-tap A/B Pause,
 *   and 1-tap Bypass Charging (shown only when supported by the kernel).
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
    private var expandPanel: LinearLayout? = null
    private var pauseButton: TextView? = null
    private var bypassButton: TextView? = null

    private var isPausedState = false
    private var isBypassSupported = false
    private var isBypassConfig = false
    private var isExpanded = false
    private var intervalMs = 1500L

    private var lastFlipCount: Long? = null
    private var lastFlipTimeMs: Long = 0L
    private var lastFpsText: String = "-- FPS"

    override fun onBind(intent: Intent?): IBinder? = null

    override fun onCreate() {
        super.onCreate()
        createNotificationChannel()
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.UPSIDE_DOWN_CAKE) {
            startForeground(NOTIF_ID, buildNotification(), ServiceInfo.FOREGROUND_SERVICE_TYPE_SPECIAL_USE)
        } else {
            startForeground(NOTIF_ID, buildNotification())
        }
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

    private fun dp(v: Int): Int = (v * resources.displayMetrics.density).toInt()

    private fun makeChip(label: String, bgColor: Int, onClick: () -> Unit): TextView {
        val bg = GradientDrawable().apply {
            shape = GradientDrawable.RECTANGLE
            cornerRadius = dp(8).toFloat()
            setColor(bgColor)
        }
        return TextView(this).apply {
            text = label
            setTextColor(Color.WHITE)
            setTextSize(TypedValue.COMPLEX_UNIT_SP, 9.5f)
            typeface = Typeface.create(Typeface.DEFAULT, Typeface.BOLD)
            background = bg
            setPadding(dp(8), dp(4), dp(8), dp(4))
            val lp = LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT,
                LinearLayout.LayoutParams.WRAP_CONTENT,
            ).apply { marginEnd = dp(5) }
            layoutParams = lp
            setOnClickListener { onClick() }
        }
    }

    private fun attachHudOverlay() {
        val wm = getSystemService(WINDOW_SERVICE) as WindowManager
        windowManager = wm

        val bg = GradientDrawable().apply {
            shape = GradientDrawable.RECTANGLE
            cornerRadius = dp(16).toFloat()
            setColor(Color.argb(225, 15, 20, 29))
            setStroke(dp(1), Color.argb(105, 56, 189, 248))
        }

        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            background = bg
            setPadding(dp(12), dp(8), dp(12), dp(8))
        }

        val headerRow = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
        }

        val title = TextView(this).apply {
            text = "CF v6 · Live"
            setTextColor(Color.WHITE)
            setTextSize(TypedValue.COMPLEX_UNIT_SP, 11f)
            typeface = Typeface.create(Typeface.DEFAULT, Typeface.BOLD)
        }
        titleText = title

        val expandHint = TextView(this).apply {
            text = "▾"
            setTextColor(Color.rgb(148, 163, 184))
            setTextSize(TypedValue.COMPLEX_UNIT_SP, 11f)
            setPadding(dp(8), 0, 0, 0)
        }

        headerRow.addView(title)
        headerRow.addView(expandHint)

        val metrics = TextView(this).apply {
            text = "-- FPS · --°C · RP --%"
            setTextColor(Color.rgb(203, 213, 225))
            setTextSize(TypedValue.COMPLEX_UNIT_SP, 10.5f)
            typeface = Typeface.MONOSPACE
            setPadding(0, dp(2), 0, 0)
        }
        metricsText = metrics

        // Expandable Zero-Profile Autonomous Mini Panel (Pause A/B + Bypass Charging + Close)
        val panel = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            visibility = View.GONE
            setPadding(0, dp(6), 0, 0)
        }

        val actionRow = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
            setPadding(0, dp(2), 0, 0)
        }

        val pauseBtn = makeChip("PAUSE", Color.argb(210, 37, 99, 235)) {
            scope.launch {
                CoreFlowManager.setPaused(!isPausedState)
                refreshOnce()
            }
        }
        pauseButton = pauseBtn

        val bypassBtn = makeChip("BYPASS", Color.argb(210, 16, 185, 129)) {
            scope.launch {
                CoreFlowManager.setPowerControl("bypass_charging", if (!isBypassConfig) "true" else "false")
                refreshOnce()
            }
        }.apply {
            visibility = View.GONE
        }
        bypassButton = bypassBtn

        val closeBtn = makeChip("CLOSE", Color.argb(200, 185, 28, 28)) {
            stopSelf()
        }

        actionRow.addView(pauseBtn)
        actionRow.addView(bypassBtn)
        actionRow.addView(closeBtn)

        panel.addView(actionRow)
        expandPanel = panel

        root.addView(headerRow)
        root.addView(metrics)
        root.addView(panel)

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
        var moved = false

        root.setOnTouchListener { _, event ->
            when (event.action) {
                MotionEvent.ACTION_DOWN -> {
                    startX = params.x
                    startY = params.y
                    downRawX = event.rawX
                    downRawY = event.rawY
                    moved = false
                    true
                }

                MotionEvent.ACTION_MOVE -> {
                    val dx = (event.rawX - downRawX).toInt()
                    val dy = (event.rawY - downRawY).toInt()
                    if (abs(dx) > 6 || abs(dy) > 6) {
                        moved = true
                        params.x = startX + dx
                        params.y = startY + dy
                        runCatching { wm.updateViewLayout(root, params) }
                    }
                    true
                }

                MotionEvent.ACTION_UP -> {
                    if (!moved) {
                        isExpanded = !isExpanded
                        expandPanel?.visibility = if (isExpanded) View.VISIBLE else View.GONE
                        expandHint.text = if (isExpanded) "▴" else "▾"
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
        val nowMs = SystemClock.elapsedRealtime()
        val elapsedMs = nowMs - lastFlipTimeMs
        if (lastFlipCount == null || elapsedMs >= 450L) {
            val flips = CoreFlowManager.readSurfaceFlingerFlipCount()
            if (flips != null && lastFlipCount != null && elapsedMs >= 450L) {
                val dFrames = flips - lastFlipCount!!
                val dtSec = elapsedMs / 1000.0
                if (dFrames >= 0L) {
                    val fps = (dFrames / dtSec).roundToInt().coerceIn(0, 240)
                    lastFpsText = "$fps FPS"
                }
            } else if (flips == null) {
                lastFpsText = "-- FPS"
            }
            if (flips != null) {
                lastFlipCount = flips
                lastFlipTimeMs = nowMs
            }
        }
        val fpsText = lastFpsText

        val status = CoreFlowManager.getStatus().getOrNull()
        if (status == null) {
            titleText?.text = "CF v6 · Offline"
            metricsText?.text = "$fpsText · Daemon offline"
            return
        }

        isPausedState = status.inspectionMode
        isBypassSupported = status.batteryPower.bypassSupported
        isBypassConfig = status.powerConfig.bypassCharging

        val stateLabel = when {
            !status.isRunning -> "STOPPED"
            status.inspectionMode -> "PAUSED"
            status.thermalGuardActive -> "T-GUARD"
            status.autonomousRegime == "gaming_3d" -> "3D GAME"
            status.autonomousRegime == "low_battery_eco" -> "ECO ≤20%"
            status.autonomousRegime == "thermal_pacing" || status.autonomousRegime == "cool_glide" ->
                if (status.easTopAppCapPct < 100) "EAS ${status.easTopAppCapPct}%" else "T-PACE"
            status.autonomousRegime == "launch_burst" -> "BURST"
            status.autonomousRegime == "interactive_scroll" -> "SCROLL"
            status.autonomousRegime == "excluded_oem" -> "OEM"
            else -> "AUTO"
        }
        titleText?.text = "CF · $stateLabel" + if (status.batteryPower.bypassActive) " · BYP" else ""

        val pauseBg = pauseButton?.background as? GradientDrawable
        if (status.inspectionMode) {
            pauseButton?.text = "RESUME"
            pauseBg?.setColor(Color.argb(220, 217, 119, 6))
        } else {
            pauseButton?.text = "PAUSE"
            pauseBg?.setColor(Color.argb(210, 37, 99, 235))
        }

        bypassButton?.visibility = if (isBypassSupported) View.VISIBLE else View.GONE
        val bypassBg = bypassButton?.background as? GradientDrawable
        if (status.batteryPower.bypassActive) {
            bypassButton?.text = "BYP ON"
            bypassBg?.setColor(Color.argb(220, 16, 185, 129))
        } else {
            bypassButton?.text = "BYP OFF"
            bypassBg?.setColor(Color.argb(190, 51, 65, 85))
        }

        val tempStr = if (status.socTempCelsius >= 0f) {
            String.format(Locale.US, "%.1f°C", status.socTempCelsius)
        } else {
            "--°C"
        }
        val rpPct = (status.renderPressure * 100f).toInt().coerceIn(0, 100)
        metricsText?.text = "$fpsText · $tempStr · RP $rpPct% · UC ${status.uclampMin}"
    }

    private fun createNotificationChannel() {
        val nm = getSystemService(NotificationManager::class.java)
        val channel = NotificationChannel(
            CHANNEL_ID,
            "CoreFlow Live HUD",
            NotificationManager.IMPORTANCE_LOW,
        ).apply {
            description = "Keeps the draggable CoreFlow v6 Game HUD & mini panel active."
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
            .setContentTitle("CoreFlow v6 Live HUD Active")
            .setContentText("Tap HUD to expand Game Panel, or tap Stop to dismiss.")
            .setContentIntent(openIntent)
            .setOngoing(true)
            .addAction(
                Notification.Action.Builder(null, "Stop HUD", stopIntent).build(),
            )
            .build()
    }
}
