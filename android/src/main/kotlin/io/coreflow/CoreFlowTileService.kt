package io.coreflow

import android.service.quicksettings.Tile
import android.service.quicksettings.TileService
import java.util.Locale
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.launch

/**
 * Android Quick Settings Tile for CoreFlow Engine v6.1.0-R (Zero-Profile Autonomous).
 * 1-tap toggle between Autonomous Engine and Paused (Stock OEM A/B Compare)
 * with live SoC temperature, thermal headroom, and regime subtitle.
 */
class CoreFlowTileService : TileService() {

    private val scope = CoroutineScope(SupervisorJob() + Dispatchers.Main)
    private var isPaused = false

    override fun onStartListening() {
        super.onStartListening()
        updateTileState()
    }

    override fun onClick() {
        super.onClick()
        val targetPaused = !isPaused
        scope.launch {
            val res = CoreFlowManager.setPaused(targetPaused)
            if (res.isSuccess) {
                isPaused = targetPaused
                updateTileState()
            } else {
                val tile = qsTile ?: return@launch
                tile.subtitle = "Root sync failed"
                tile.updateTile()
            }
        }
    }

    private fun updateTileState() {
        scope.launch {
            val status = CoreFlowManager.getStatus().getOrNull()
            val tile = qsTile ?: return@launch
            val isRunning = status?.isRunning == true
            val temp = status?.socTempCelsius ?: -1f

            if (isRunning && status != null) {
                isPaused = status.inspectionMode
                tile.state = if (status.inspectionMode) Tile.STATE_INACTIVE else Tile.STATE_ACTIVE
                tile.label = if (status.inspectionMode) "CoreFlow: Paused" else "CoreFlow Auto"
                tile.subtitle = when {
                    status.inspectionMode -> "Stock OEM (A/B)"
                    status.lowBatteryEco && temp > 0f ->
                        String.format(Locale.US, "Eco · %.1f°C · %d%%", temp, (status.capacityScale * 100).toInt())
                    temp > 0f ->
                        String.format(Locale.US, "%.1f°C · %d%%", temp, (status.capacityScale * 100).toInt())
                    else -> status.autonomousRegimeLabel(isIndonesian = false)
                }
            } else {
                isPaused = false
                tile.state = Tile.STATE_INACTIVE
                tile.label = "CoreFlow"
                tile.subtitle = "Stopped"
            }
            tile.updateTile()
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        scope.cancel()
    }
}
