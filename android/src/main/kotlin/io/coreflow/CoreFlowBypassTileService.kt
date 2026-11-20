package io.coreflow

import android.service.quicksettings.Tile
import android.service.quicksettings.TileService
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.launch

/**
 * Second Quick Settings Tile dedicated to CFJ4-protected Bypass Charging.
 * Honestly reports "Unavailable" if the kernel does not expose a safe charging control node.
 */
class CoreFlowBypassTileService : TileService() {

    private val scope = CoroutineScope(SupervisorJob() + Dispatchers.Main)
    private var lastSupported = false
    private var lastBypassConfig = false

    override fun onStartListening() {
        super.onStartListening()
        refreshTile()
    }

    override fun onClick() {
        super.onClick()
        if (!lastSupported) {
            refreshTile()
            return
        }
        val target = !lastBypassConfig
        scope.launch {
            CoreFlowManager.setPowerControl("bypass_charging", if (target) "true" else "false")
            refreshTile()
        }
    }

    private fun refreshTile() {
        val tile = qsTile ?: return
        scope.launch {
            val status = CoreFlowManager.getStatus().getOrNull()
            val supported = status?.batteryPower?.bypassSupported == true
            val active = status?.batteryPower?.bypassActive == true
            lastSupported = supported
            lastBypassConfig = status?.powerConfig?.bypassCharging == true

            when {
                status == null || !status.isRunning -> {
                    tile.state = Tile.STATE_UNAVAILABLE
                    tile.label = getString(R.string.qs_bypass_tile_label)
                    tile.subtitle = "Daemon offline"
                }

                !supported -> {
                    tile.state = Tile.STATE_UNAVAILABLE
                    tile.label = getString(R.string.qs_bypass_tile_label)
                    tile.subtitle = "Unavailable"
                }

                active -> {
                    tile.state = Tile.STATE_ACTIVE
                    tile.label = getString(R.string.qs_bypass_tile_label)
                    tile.subtitle = "Active (${status.batteryPower.bypassReason})"
                }

                else -> {
                    tile.state = Tile.STATE_INACTIVE
                    tile.label = getString(R.string.qs_bypass_tile_label)
                    tile.subtitle = status.batteryPower.capacityPct?.let { "Off · $it%" } ?: "Off"
                }
            }
            tile.updateTile()
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        scope.cancel()
    }
}
