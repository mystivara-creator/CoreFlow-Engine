package io.coreflow

import android.graphics.drawable.Icon
import android.service.quicksettings.Tile
import android.service.quicksettings.TileService
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.launch

/**
 * Android Quick Settings Tile for CoreFlow Engine.
 * Allows instant cycling through Balanced, Performance, and Battery profiles.
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
        updateTileState()
    }

    override fun onClick() {
        super.onClick()
        currentProfileIndex = (currentProfileIndex + 1) % profiles.size
        val nextProfile = profiles[currentProfileIndex]

        scope.launch {
            CoreFlowManager.applyProfile(nextProfile)
            updateTileState()
        }
    }

    private fun updateTileState() {
        val tile = qsTile ?: return
        scope.launch {
            val statusResult = CoreFlowManager.getStatus()
            val isRunning = statusResult.getOrNull()?.isRunning == true
            val temp = statusResult.getOrNull()?.socCompositeCelsius ?: -1f

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
