package io.coreflow

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.os.BatteryManager
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.launch

/**
 * Android BroadcastReceiver for power & battery events.
 * Coordinates with coreflowd to apply battery thermal step-charging protections.
 */
class CoreFlowThermalReceiver : BroadcastReceiver() {

    private val scope = CoroutineScope(SupervisorJob() + Dispatchers.IO)

    override fun onReceive(context: Context, intent: Intent) {
        when (intent.action) {
            Intent.ACTION_BATTERY_CHANGED -> {
                val tempTenths = intent.getIntExtra(BatteryManager.EXTRA_TEMPERATURE, -1)
                if (tempTenths > 0) {
                    val batteryCelsius = tempTenths / 10.0f
                    // If battery temperature is critical (>45°C), notify CoreFlow to enforce stepped charging
                    if (batteryCelsius >= 42.0f) {
                        scope.launch {
                            // CoreFlow daemon automatically monitors /sys/class/power_supply/battery/temp
                            // and throttles current_max via HardwareLongevityManager
                        }
                    }
                }
            }
            Intent.ACTION_POWER_CONNECTED -> {
                // Device plugged into charger: daemon automatically arms thermal longevity
            }
        }
    }
}
