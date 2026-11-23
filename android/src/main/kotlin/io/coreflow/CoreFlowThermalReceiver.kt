package io.coreflow

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent

/**
 * Android BroadcastReceiver retained for manifest compatibility.
 *
 * Since CoreFlow Engine v6.1.0-R (Zero-Profile Autonomous), coreflowd natively evaluates
 * real-time battery SOC (Low-Battery Auto-Eco <= 20%), dual-domain chassis/die thermals
 * (Proactive 38°C Cool-Glide + Thermal Guard), and charging state directly in kernel space
 * without mutating manual profiles from Android broadcasts.
 */
class CoreFlowThermalReceiver : BroadcastReceiver() {
    override fun onReceive(context: Context, intent: Intent) {
        // Intentionally passive: coreflowd owns thermal & battery eco autonomously.
    }
}
