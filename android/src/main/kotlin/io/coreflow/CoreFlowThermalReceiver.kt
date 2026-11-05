package io.coreflow

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.os.BatteryManager
import android.os.PowerManager
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.launch

/**
 * Android BroadcastReceiver for power-save mode and charger state transitions.
 * Queries sticky ACTION_BATTERY_CHANGED at runtime and coordinates with coreflowd.
 */
class CoreFlowThermalReceiver : BroadcastReceiver() {

    private val scope = CoroutineScope(SupervisorJob() + Dispatchers.IO)

    override fun onReceive(context: Context, intent: Intent) {
        val pendingResult = goAsync()
        scope.launch {
            try {
                when (intent.action) {
                    PowerManager.ACTION_POWER_SAVE_MODE_CHANGED -> {
                        val pm = context.getSystemService(Context::class.java)
                            ?.getSystemService(PowerManager::class.java)
                            ?: context.getSystemService(Context.POWER_SERVICE) as? PowerManager
                        val profile = if (pm?.isPowerSaveMode == true) {
                            CoreFlowManager.Profile.BATTERY
                        } else {
                            CoreFlowManager.Profile.BALANCED
                        }
                        CoreFlowManager.applyProfile(profile)
                    }
                    Intent.ACTION_POWER_CONNECTED,
                    Intent.ACTION_POWER_DISCONNECTED,
                    Intent.ACTION_BATTERY_CHANGED -> {
                        val batteryIntent = if (intent.action == Intent.ACTION_BATTERY_CHANGED) {
                            intent
                        } else {
                            context.registerReceiver(null, IntentFilter(Intent.ACTION_BATTERY_CHANGED))
                        }
                        val tempTenths = batteryIntent?.getIntExtra(BatteryManager.EXTRA_TEMPERATURE, -1) ?: -1
                        if (tempTenths > 0) {
                            val batteryCelsius = tempTenths / 10.0f
                            if (batteryCelsius >= 44.0f) {
                                CoreFlowManager.applyProfile(CoreFlowManager.Profile.THERMAL_GUARD)
                            }
                        }
                    }
                }
            } finally {
                pendingResult.finish()
            }
        }
    }
}
