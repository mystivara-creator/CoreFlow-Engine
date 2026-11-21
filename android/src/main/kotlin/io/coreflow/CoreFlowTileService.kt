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
 * Android Quick Settings Tile for CoreFlow Engine v5.1.0.
 * Cycles through Balanced, Smoothness, Gaming, and Battery profiles
 * with verified root persistence and live state synchronization.
 */
class CoreFlowTileService : TileService() {

    private val scope = CoroutineScope(SupervisorJob() + Dispatchers.Main)
    private var currentProfileIndex = 0
    private val profiles = listOf(
        CoreFlowManager.Profile.BALANCED,
        CoreFlowManager.Profile.SMOOTHNESS,
        CoreFlowManager.Profile.GAMING,
        CoreFlowManager.Profile.BATTERY
    )

    override fun onStartListening() {
        super.onStartListening()
        scope.launch {
            val active = CoreFlowManager.readActiveProfile().getOrNull()
            if (active != null) {
                val idx = profiles.indexOf(active)
                if (idx >= 0) {
                    currentProfileIndex = idx
                }
            }
            updateTileState()
        }
    }

    override fun onClick() {
        super.onClick()
        val nextIndex = (currentProfileIndex + 1) % profiles.size
        val nextProfile = profiles[nextIndex]

        scope.launch {
            val applyResult = CoreFlowManager.applyProfile(nextProfile)
            if (applyResult.isSuccess) {
                currentProfileIndex = nextIndex
                updateTileState()
            } else {
                val tile = qsTile ?: return@launch
                tile.subtitle = "Apply failed"
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

            if (isRunning) {
                val profile = status?.profile ?: profiles[currentProfileIndex]
                tile.state = if (status?.inspectionMode == true) Tile.STATE_INACTIVE else Tile.STATE_ACTIVE
                tile.label = "CoreFlow: ${profile.label}"
                tile.subtitle = when {
                    status?.inspectionMode == true -> "Paused (OEM)"
                    temp > 0f -> String.format(Locale.US, "%.1f°C · %d%%", temp, ((status?.capacityScale ?: 1f) * 100).toInt())
                    else -> status?.mode ?: "Active"
                }
            } else {
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
