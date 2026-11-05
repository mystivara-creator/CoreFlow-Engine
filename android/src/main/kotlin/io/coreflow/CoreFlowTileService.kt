package io.coreflow

import android.service.quicksettings.Tile
import android.service.quicksettings.TileService
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.launch

/**
 * Android Quick Settings Tile for CoreFlow Engine.
 * Allows cycling through Balanced, Performance (Responsiveness), and Battery profiles
 * with verified root persistence and live state synchronization.
 */
class CoreFlowTileService : TileService() {

    private val scope = CoroutineScope(SupervisorJob() + Dispatchers.Main)
    private var currentProfileIndex = 0
    private val profiles = listOf(
        CoreFlowManager.Profile.BALANCED,
        CoreFlowManager.Profile.PERFORMANCE,
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
        val tile = qsTile ?: return
        scope.launch {
            val statusResult = CoreFlowManager.getStatus()
            val status = statusResult.getOrNull()
            val isRunning = status?.isRunning == true
            val temp = status?.socCompositeCelsius ?: -1f

            if (isRunning) {
                val profile = profiles[currentProfileIndex]
                tile.state = Tile.STATE_ACTIVE
                tile.label = "CoreFlow: ${profile.label}"
                tile.subtitle = if (temp > 0) String.format("%.1f°C", temp) else "Active"
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
