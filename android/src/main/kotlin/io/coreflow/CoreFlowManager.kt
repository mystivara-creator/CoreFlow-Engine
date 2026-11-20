package io.coreflow

import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import org.json.JSONObject
import java.io.BufferedReader

/**
 * CoreFlowManager: Kotlin 2.1 Android client for CoreFlow Engine v5.1.0 daemon (coreflowd).
 * Provides root-bridged state querying, thermal telemetry, and 4-profile actuation.
 */
object CoreFlowManager {

    private const val RESOLVE_DAEMON =
        "BIN=\"/data/adb/modules_update/cfae_framework/system/bin/coreflowd\"; " +
        "[ -x \"\$BIN\" ] || BIN=\"/data/adb/modules/cfae_framework/system/bin/coreflowd\"; " +
        "[ -x \"\$BIN\" ] || BIN=\"/data/adb/modules/coreflow_engine/system/bin/coreflowd\"; " +
        "[ -x \"\$BIN\" ] || BIN=\"/data/adb/modules/cfae_framework/coreflowd\"; " +
        "[ -x \"\$BIN\" ] || BIN=\"/data/adb/modules/coreflow_engine/coreflowd\"; " +
        "[ -x \"\$BIN\" ] || BIN=\"/system_ext/bin/coreflowd\"; " +
        "[ -x \"\$BIN\" ] || BIN=\"/system/bin/coreflowd\""

    private const val CONFIG_PATH = "/data/adb/coreflow/coreflow.conf"

    enum class Profile(val label: String, val configValue: String) {
        BALANCED("Balanced", "balanced"),
        SMOOTHNESS("Smoothness", "smoothness"),
        GAMING("Gaming", "gaming"),
        BATTERY("Battery", "battery"),
        THERMAL_GUARD("Thermal Guard", "battery");

        companion object {
            fun fromConfigValue(raw: String): Profile {
                return when (raw.trim().lowercase()) {
                    "smoothness", "smooth" -> SMOOTHNESS
                    "gaming", "performance" -> GAMING
                    "battery", "efficiency", "powersave" -> BATTERY
                    else -> BALANCED
                }
            }
        }
    }

    data class DaemonStatus(
        val version: String,
        val isRunning: Boolean,
        val pid: Int,
        val profile: Profile,
        val mode: String,
        val inspectionMode: Boolean,
        val uclampMin: Int,
        val uclampMax: Int,
        val uclampSupported: Boolean,
        val socTempCelsius: Float,
        val batteryTempCelsius: Float,
        val capacityScale: Float,
        val recoveryPending: Boolean,
        val thermalSource: String,
        val thermalGuardActive: Boolean = false
    )

    /**
     * Query live v5.1.0 status from coreflowd using the native root bridge.
     */
    suspend fun getStatus(): Result<DaemonStatus> = withContext(Dispatchers.IO) {
        runCatching {
            val output = execRootCommand("$RESOLVE_DAEMON; \"\$BIN\" --status")
            val lastLine = output.trim().lines().lastOrNull { it.trim().startsWith("{") }
                ?: throw IllegalStateException("Empty daemon output")
            val json = JSONObject(lastLine)
            val actuators = json.optJSONObject("actuators")

            fun JSONObject.optFiniteFloat(key: String, fallback: Float = -1f): Float {
                if (isNull(key)) return fallback
                val v = optDouble(key, fallback.toDouble())
                return if (v.isFinite()) v.toFloat() else fallback
            }

            DaemonStatus(
                version = json.optString("version", "5.1.0"),
                isRunning = json.optBoolean("running", false),
                pid = json.optInt("pid", 0),
                profile = Profile.fromConfigValue(json.optString("profile", "balanced")),
                mode = json.optString("mode", "EFFICIENCY"),
                inspectionMode = json.optBoolean("inspectionMode", false),
                uclampMin = actuators?.optInt("threadUclampMin", 0) ?: 0,
                uclampMax = actuators?.optInt("threadUclampMax", 1024) ?: 1024,
                uclampSupported = actuators?.optBoolean("perThreadUclampSupported", false) ?: false,
                socTempCelsius = json.optFiniteFloat("socTempCelsius"),
                batteryTempCelsius = json.optFiniteFloat("batteryTempCelsius"),
                capacityScale = json.optFiniteFloat("capacityScale", 1.0f),
                recoveryPending = json.optBoolean("recoveryPending", false),
                thermalSource = json.optString("thermalSource", "oem_policy"),
                thermalGuardActive = json.optBoolean("thermalGuard", false)
            )
        }
    }

    /**
     * Read the active optimization profile from /data/adb/coreflow/coreflow.conf [general].
     */
    suspend fun readActiveProfile(): Result<Profile> = withContext(Dispatchers.IO) {
        runCatching {
            val raw = execRootCommand("cat $CONFIG_PATH 2>/dev/null || true")
            var profile = "balanced"
            var inGeneral = false
            for (line in raw.lineSequence()) {
                val trimmed = line.trim()
                if (trimmed.isEmpty() || trimmed.startsWith("#") || trimmed.startsWith(";")) continue
                if (trimmed.startsWith("[") && trimmed.endsWith("]")) {
                    inGeneral = trimmed.substring(1, trimmed.length - 1).trim().lowercase() == "general"
                    continue
                }
                if (!inGeneral) continue
                val eq = trimmed.indexOf('=')
                if (eq <= 0) continue
                val key = trimmed.substring(0, eq).trim()
                val value = trimmed.substring(eq + 1).trim()
                if (key == "profile") {
                    profile = value
                    break
                }
            }
            Profile.fromConfigValue(profile)
        }
    }

    /**
     * Apply a v5.1.0 optimization profile using coreflowd's atomic --set-config updater.
     */
    suspend fun applyProfile(profile: Profile): Result<Unit> = withContext(Dispatchers.IO) {
        runCatching {
            val cmd = "$RESOLVE_DAEMON; \"\$BIN\" --set-config 'profile=${profile.configValue}'"
            execRootCommand(cmd)
            Unit
        }
    }

    /**
     * Toggle live Pause/Resume (inspection_mode) for instant A/B OEM comparison.
     */
    suspend fun setPaused(paused: Boolean): Result<Unit> = withContext(Dispatchers.IO) {
        runCatching {
            val valStr = if (paused) "true" else "false"
            execRootCommand("$RESOLVE_DAEMON; \"\$BIN\" --set-config 'inspection_mode=$valStr'")
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
