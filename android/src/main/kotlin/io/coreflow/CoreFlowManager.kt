package io.coreflow

import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import org.json.JSONObject
import java.io.BufferedReader

/**
 * CoreFlowManager: Kotlin 2.1 Android client for CFAE System Framework daemon (coreflowd).
 * Provides root-bridged state querying, multi-sensor thermal monitoring,
 * and coordinated profile actuation.
 */
object CoreFlowManager {

    private const val RESOLVE_DAEMON =
        "BIN=\"/data/adb/modules/cfae_framework/system/bin/coreflowd\"; " +
        "[ -x \"\$BIN\" ] || BIN=\"/data/adb/modules/coreflow_engine/system/bin/coreflowd\"; " +
        "[ -x \"\$BIN\" ] || BIN=\"/data/adb/modules/cfae_framework/coreflowd\"; " +
        "[ -x \"\$BIN\" ] || BIN=\"/system_ext/bin/coreflowd\"; " +
        "[ -x \"\$BIN\" ] || BIN=\"/system/bin/coreflowd\""

    private const val CONFIG_PATH = "/data/adb/coreflow/coreflow.conf"
    private const val PID_PATH = "/data/adb/coreflow/coreflowd.pid"

    enum class Profile(val label: String, val uclampMin: Int) {
        BATTERY("Battery Saver", 64),
        BALANCED("Balanced Equilibrium", 320),
        PERFORMANCE("Interactive Responsiveness", 640),
        THERMAL_GUARD("ThermalGuard (Cooling)", 0)
    }

    data class DaemonStatus(
        val isRunning: Boolean,
        val pid: Int,
        val uclampMin: Int,
        val uclampMax: Int,
        val uclampSupported: Boolean,
        val socTempCelsius: Float,
        val socCompositeCelsius: Float,
        val cpuAverageCelsius: Float,
        val cpuPeakCelsius: Float,
        val gpuTempCelsius: Float,
        val batteryTempCelsius: Float,
        val recoveryPending: Boolean,
        val thermalSource: String
    )

    /**
     * Query live status from coreflowd using the native root bridge.
     */
    suspend fun getStatus(): Result<DaemonStatus> = withContext(Dispatchers.IO) {
        runCatching {
            val output = execRootCommand("$RESOLVE_DAEMON; \"\$BIN\" --status")
            val lastLine = output.trim().lines().lastOrNull() ?: throw IllegalStateException("Empty daemon output")
            val json = JSONObject(lastLine)

            DaemonStatus(
                isRunning = json.optBoolean("running", false),
                pid = json.optInt("pid", 0),
                uclampMin = json.optInt("uclampMin", 0),
                uclampMax = json.optInt("uclampMax", 0),
                uclampSupported = json.optBoolean("uclampSupported", false),
                socTempCelsius = json.optDouble("socTempCelsius", -1.0).toFloat(),
                socCompositeCelsius = json.optDouble("socCompositeCelsius", -1.0).toFloat(),
                cpuAverageCelsius = json.optDouble("cpuAverageCelsius", -1.0).toFloat(),
                cpuPeakCelsius = json.optDouble("cpuPeakCelsius", -1.0).toFloat(),
                gpuTempCelsius = json.optDouble("gpuTempCelsius", -1.0).toFloat(),
                batteryTempCelsius = json.optDouble("batteryTempCelsius", -1.0).toFloat(),
                recoveryPending = json.optBoolean("recoveryPending", false),
                thermalSource = json.optString("thermalSource", "multi-sensor-composite")
            )
        }
    }

    /**
     * Apply a coordinated optimization profile.
     */
    suspend fun applyProfile(profile: Profile): Result<Unit> = withContext(Dispatchers.IO) {
        runCatching {
            val cmd = StringBuilder()
            cmd.append("sed -i 's/^ui_adaptive_bias=.*/ui_adaptive_bias=${profile.name.lowercase()}/' $CONFIG_PATH 2>/dev/null; ")
            cmd.append("sed -i 's/^top_app_interactive_min=.*/top_app_interactive_min=${profile.uclampMin}/' $CONFIG_PATH 2>/dev/null; ")
            cmd.append("$RESOLVE_DAEMON; killall -HUP coreflowd 2>/dev/null || true")
            execRootCommand(cmd.toString())
            Unit
        }
    }

    /**
     * Request emergency or maintenance baseline rollback.
     */
    suspend fun rollbackBaseline(): Result<Unit> = withContext(Dispatchers.IO) {
        runCatching {
            execRootCommand("$RESOLVE_DAEMON; \"\$BIN\" --rollback-baseline")
            Unit
        }
    }

    /**
     * Low-overhead root shell executor.
     */
    private fun execRootCommand(command: String): String {
        val process = ProcessBuilder("su", "-c", command)
            .redirectErrorStream(true)
            .start()

        val output = process.inputStream.bufferedReader().use(BufferedReader::readText)
        val exitCode = process.waitFor()
        if (exitCode != 0) {
            throw RuntimeException("Root command failed ($exitCode): $output")
        }
        return output
    }
}
