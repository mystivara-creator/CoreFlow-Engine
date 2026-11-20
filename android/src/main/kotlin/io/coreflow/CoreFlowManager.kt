package io.coreflow

import com.topjohnwu.superuser.Shell
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import org.json.JSONArray
import org.json.JSONObject

/**
 * CoreFlowManager: Root-bridged Android client for CoreFlow Engine v5.1.0 daemon (coreflowd).
 * Uses libsu persistent root shell for fast, zero-intermediary access on Magisk, KernelSU,
 * KernelSU-Next, and APatch.
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

    private const val RESOLVE_SERVICE_SCRIPT =
        "SVC=\"/data/adb/modules_update/cfae_framework/service.sh\"; " +
            "[ -f \"\$SVC\" ] || SVC=\"/data/adb/modules/cfae_framework/service.sh\"; " +
            "[ -f \"\$SVC\" ] || SVC=\"/data/adb/modules/coreflow_engine/service.sh\""

    private const val CONFIG_PATH = "/data/adb/coreflow/coreflow.conf"
    private val PACKAGE_REGEX = Regex("^[A-Za-z][A-Za-z0-9_]*(\\.[A-Za-z0-9_]+)+$")

    enum class Profile(val label: String, val configValue: String, val tagline: String) {
        BALANCED("Balanced", "balanced", "Smart daily speed & battery"),
        SMOOTHNESS("Smoothness", "smoothness", "Locked UI & scroll fluidity"),
        GAMING("Gaming", "gaming", "Max frame pacing & render floor"),
        BATTERY("Battery", "battery", "Cool thermal & max endurance"),
        THERMAL_GUARD("Thermal Guard", "battery", "Thermal protection fallback");

        companion object {
            val primaryProfiles: List<Profile> = listOf(BALANCED, SMOOTHNESS, GAMING, BATTERY)

            fun fromConfigValue(raw: String): Profile =
                when (raw.trim().lowercase()) {
                    "smoothness", "smooth" -> SMOOTHNESS
                    "gaming", "performance" -> GAMING
                    "battery", "efficiency", "powersave" -> BATTERY
                    else -> BALANCED
                }
        }
    }

    enum class AppRuleBucket(val label: String, val configKey: String?) {
        AUTO("Auto", null),
        GAMING("Gaming", "gaming_apps"),
        SMOOTHNESS("Smooth", "smoothness_apps"),
        BATTERY("Battery", "battery_apps"),
        EXCLUDED("Stock", "excluded_apps");
    }

    data class SessionMetrics(
        val uptimeSec: Long = 0L,
        val launchBursts: Long = 0L,
        val thermalGuardEvents: Long = 0L,
        val avgSocTempCelsius: Float = -1f,
        val peakSocTempCelsius: Float = -1f,
        val avgCapacityScale: Float = 1f,
    )

    data class HardwareInfo(
        val socModel: String = "Unknown SoC",
        val kernelVersion: String = "",
        val storageType: String = "UFS",
        val gpuModel: String = "",
        val refreshHz: Int = 60,
        val cpuClusters: Int = 0,
        val mglruAvailable: Boolean = false,
        val cgroupV2Available: Boolean = false,
    )

    data class AppRules(
        val gaming: List<String> = emptyList(),
        val smoothness: List<String> = emptyList(),
        val battery: List<String> = emptyList(),
        val excluded: List<String> = emptyList(),
    ) {
        fun bucketFor(packageName: String): AppRuleBucket =
            when {
                packageName in gaming -> AppRuleBucket.GAMING
                packageName in smoothness -> AppRuleBucket.SMOOTHNESS
                packageName in battery -> AppRuleBucket.BATTERY
                packageName in excluded -> AppRuleBucket.EXCLUDED
                else -> AppRuleBucket.AUTO
            }

        val totalConfigured: Int
            get() = gaming.size + smoothness.size + battery.size + excluded.size
    }

    data class DaemonStatus(
        val version: String,
        val isRunning: Boolean,
        val pid: Int,
        val profile: Profile,
        val effectiveProfile: Profile,
        val mode: String,
        val inspectionMode: Boolean,
        val foregroundPackage: String,
        val uclampMin: Int,
        val uclampMax: Int,
        val uclampSupported: Boolean,
        val boostedTids: Int,
        val storageBurstActive: Boolean,
        val readAheadKb: Int,
        val prereadBytes: Long,
        val managedEnvelopeNodes: Int,
        val contendedPaths: List<String>,
        val socTempCelsius: Float,
        val batteryTempCelsius: Float,
        val capacityScale: Float,
        val cpuUtilPct: Float,
        val renderPressure: Float,
        val baselineNodesSnapshotted: Int,
        val recoveryPending: Boolean,
        val thermalSource: String,
        val thermalGuardActive: Boolean = false,
        val hardware: HardwareInfo = HardwareInfo(),
        val session: SessionMetrics = SessionMetrics(),
        val apps: AppRules = AppRules(),
    ) {
        val activePillars: Int
            get() {
                var count = 0
                if (uclampSupported) count++
                if (readAheadKb > 0 || storageBurstActive) count++
                if (hardware.mglruAvailable || prereadBytes > 0L) count++
                if (managedEnvelopeNodes > 0 || thermalSource.isNotBlank()) count++
                return count.coerceIn(0, 4)
            }
    }

    suspend fun checkRootAccess(): Boolean = withContext(Dispatchers.IO) {
        runCatching {
            val shell = Shell.getShell()
            shell.isRoot
        }.getOrDefault(false)
    }

    suspend fun getStatus(): Result<DaemonStatus> = withContext(Dispatchers.IO) {
        runCatching {
            val output = execRootCommand("$RESOLVE_DAEMON; \"\$BIN\" --status")
            val lastLine = output.trim().lines().lastOrNull { it.trim().startsWith("{") }
                ?: throw IllegalStateException("Empty daemon output")
            val json = JSONObject(lastLine)
            val actuators = json.optJSONObject("actuators")
            val hardwareJson = json.optJSONObject("hardware")
            val sessionJson = json.optJSONObject("session")
            val appsJson = json.optJSONObject("apps")

            fun JSONObject?.optFiniteFloat(key: String, fallback: Float = -1f): Float {
                if (this == null || isNull(key)) return fallback
                val v = optDouble(key, fallback.toDouble())
                return if (v.isFinite()) v.toFloat() else fallback
            }

            fun JSONArray?.toStringList(): List<String> {
                if (this == null) return emptyList()
                val out = ArrayList<String>(length())
                for (i in 0 until length()) {
                    val item = optString(i, "").trim()
                    if (item.isNotEmpty()) out.add(item)
                }
                return out
            }

            DaemonStatus(
                version = json.optString("version", "5.1.0"),
                isRunning = json.optBoolean("running", false),
                pid = json.optInt("pid", 0),
                profile = Profile.fromConfigValue(json.optString("profile", "balanced")),
                effectiveProfile = Profile.fromConfigValue(
                    json.optString("effectiveProfile", json.optString("profile", "balanced")),
                ),
                mode = json.optString("mode", "EFFICIENCY"),
                inspectionMode = json.optBoolean("inspectionMode", false),
                foregroundPackage = json.optString("foregroundPackage", ""),
                uclampMin = actuators?.optInt("threadUclampMin", 0) ?: 0,
                uclampMax = actuators?.optInt("threadUclampMax", 1024) ?: 1024,
                uclampSupported = actuators?.optBoolean("perThreadUclampSupported", false) ?: false,
                boostedTids = actuators?.optInt("boostedTids", 0) ?: 0,
                storageBurstActive = actuators?.optBoolean("storageBurstActive", false) ?: false,
                readAheadKb = actuators?.optInt("readAheadKb", 0) ?: 0,
                prereadBytes = actuators?.optLong("prereadBytes", 0L) ?: 0L,
                managedEnvelopeNodes = actuators?.optInt("managedEnvelopeNodes", 0) ?: 0,
                contendedPaths = actuators?.optJSONArray("contendedPaths").toStringList(),
                socTempCelsius = json.optFiniteFloat("socTempCelsius"),
                batteryTempCelsius = json.optFiniteFloat("batteryTempCelsius"),
                capacityScale = json.optFiniteFloat("capacityScale", 1.0f),
                cpuUtilPct = json.optFiniteFloat("cpuUtilPct", 0f),
                renderPressure = json.optFiniteFloat("renderPressure", 0f),
                baselineNodesSnapshotted = json.optInt("baselineNodesSnapshotted", 0),
                recoveryPending = json.optBoolean("recoveryPending", false),
                thermalSource = json.optString("thermalSource", "oem_policy"),
                thermalGuardActive = json.optBoolean("thermalGuard", false),
                hardware = HardwareInfo(
                    socModel = hardwareJson?.optString("socModel", "Unknown SoC").orEmpty().ifBlank { "Unknown SoC" },
                    kernelVersion = hardwareJson?.optString("kernelVersion", "").orEmpty(),
                    storageType = hardwareJson?.optString("storageType", "UFS").orEmpty(),
                    gpuModel = hardwareJson?.optString("gpuModel", "").orEmpty(),
                    refreshHz = hardwareJson?.optInt("refreshHz", 60) ?: 60,
                    cpuClusters = hardwareJson?.optInt("cpuClusters", 0) ?: 0,
                    mglruAvailable = hardwareJson?.optBoolean("mglruAvailable", false) ?: false,
                    cgroupV2Available = hardwareJson?.optBoolean("cgroupV2Available", false) ?: false,
                ),
                session = SessionMetrics(
                    uptimeSec = sessionJson?.optLong("uptimeSec", 0L) ?: 0L,
                    launchBursts = sessionJson?.optLong("launchBursts", 0L) ?: 0L,
                    thermalGuardEvents = sessionJson?.optLong("thermalGuardEvents", 0L) ?: 0L,
                    avgSocTempCelsius = sessionJson.optFiniteFloat("avgSocTempCelsius"),
                    peakSocTempCelsius = sessionJson.optFiniteFloat("peakSocTempCelsius"),
                    avgCapacityScale = sessionJson.optFiniteFloat("avgCapacityScale", 1.0f),
                ),
                apps = AppRules(
                    gaming = appsJson?.optJSONArray("gaming").toStringList(),
                    smoothness = appsJson?.optJSONArray("smoothness").toStringList(),
                    battery = appsJson?.optJSONArray("battery").toStringList(),
                    excluded = appsJson?.optJSONArray("excluded").toStringList(),
                ),
            )
        }
    }

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

    suspend fun applyProfile(profile: Profile): Result<Unit> = withContext(Dispatchers.IO) {
        runCatching {
            execRootCommand("$RESOLVE_DAEMON; \"\$BIN\" --set-config 'profile=${profile.configValue}'")
            Unit
        }
    }

    suspend fun setPaused(paused: Boolean): Result<Unit> = withContext(Dispatchers.IO) {
        runCatching {
            val valStr = if (paused) "true" else "false"
            execRootCommand("$RESOLVE_DAEMON; \"\$BIN\" --set-config 'inspection_mode=$valStr'")
            Unit
        }
    }

    /**
     * Mutually exclusive per-app rule update across all 4 CoreFlow v5.1.0 buckets.
     */
    suspend fun setAppRule(
        packageName: String,
        targetBucket: AppRuleBucket,
        currentRules: AppRules,
    ): Result<AppRules> = withContext(Dispatchers.IO) {
        runCatching {
            require(PACKAGE_REGEX.matches(packageName)) { "Invalid package name: $packageName" }
            val gaming = currentRules.gaming.filterNot { it == packageName }.toMutableList()
            val smoothness = currentRules.smoothness.filterNot { it == packageName }.toMutableList()
            val battery = currentRules.battery.filterNot { it == packageName }.toMutableList()
            val excluded = currentRules.excluded.filterNot { it == packageName }.toMutableList()

            when (targetBucket) {
                AppRuleBucket.GAMING -> gaming.add(packageName)
                AppRuleBucket.SMOOTHNESS -> smoothness.add(packageName)
                AppRuleBucket.BATTERY -> battery.add(packageName)
                AppRuleBucket.EXCLUDED -> excluded.add(packageName)
                AppRuleBucket.AUTO -> Unit
            }

            val updated = AppRules(
                gaming = gaming.distinct().sorted(),
                smoothness = smoothness.distinct().sorted(),
                battery = battery.distinct().sorted(),
                excluded = excluded.distinct().sorted(),
            )

            val updates = buildList {
                if (updated.gaming != currentRules.gaming) add("gaming_apps=${updated.gaming.joinToString(",")}")
                if (updated.smoothness != currentRules.smoothness) add("smoothness_apps=${updated.smoothness.joinToString(",")}")
                if (updated.battery != currentRules.battery) add("battery_apps=${updated.battery.joinToString(",")}")
                if (updated.excluded != currentRules.excluded) add("excluded_apps=${updated.excluded.joinToString(",")}")
            }

            if (updates.isNotEmpty()) {
                val cmd = buildString {
                    append(RESOLVE_DAEMON)
                    for (arg in updates) {
                        append("; \"\$BIN\" --set-config '")
                        append(arg)
                        append("'")
                    }
                }
                execRootCommand(cmd)
            }
            updated
        }
    }

    suspend fun startDaemon(): Result<Unit> = withContext(Dispatchers.IO) {
        runCatching {
            execRootCommand(
                "$RESOLVE_SERVICE_SCRIPT; $RESOLVE_DAEMON; " +
                    "if [ -f \"\$SVC\" ]; then sh \"\$SVC\" >/dev/null 2>&1 & " +
                    "elif [ -x \"\$BIN\" ]; then nohup \"\$BIN\" >/dev/null 2>&1 & fi",
            )
            Unit
        }
    }

    suspend fun rollbackBaseline(): Result<Unit> = withContext(Dispatchers.IO) {
        runCatching {
            execRootCommand("$RESOLVE_DAEMON; \"\$BIN\" --rollback-baseline")
            Unit
        }
    }

    suspend fun getJournalJson(): Result<String> = withContext(Dispatchers.IO) {
        runCatching {
            execRootCommand("$RESOLVE_DAEMON; \"\$BIN\" --journal").trim()
        }
    }

    suspend fun getDiagJson(): Result<String> = withContext(Dispatchers.IO) {
        runCatching {
            execRootCommand("$RESOLVE_DAEMON; \"\$BIN\" --diag").trim()
        }
    }

    private fun execRootCommand(command: String): String {
        val result = Shell.cmd(command).exec()
        val combined = result.out.joinToString("\n")
        if (!result.isSuccess) {
            throw IllegalStateException(
                "Root command failed (${result.code}): ${combined.ifBlank { "no output" }}",
            )
        }
        return combined
    }
}
