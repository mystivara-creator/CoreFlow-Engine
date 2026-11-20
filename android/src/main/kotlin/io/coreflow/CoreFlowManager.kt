package io.coreflow

import com.topjohnwu.superuser.Shell
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import org.json.JSONArray
import org.json.JSONObject

/**
 * CoreFlowManager: Root-bridged Android client for CoreFlow Engine v6.0.0 NextGen (coreflowd).
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

    private val ALLOWED_FEATURE_KEYS = setOf(
        "enable_frame_smoothness",
        "enable_storage_burst",
        "enable_memory_guard",
        "enable_thermal_pacing",
        "enable_battery_step_charging",
    )

    private val ALLOWED_POWER_KEYS = setOf(
        "bypass_charging",
        "bypass_on_gaming",
        "charge_limit_pct",
    )

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

    data class MemoryInfo(
        val totalBytes: Long = 0L,
        val availableBytes: Long = 0L,
        val swapTotalBytes: Long = 0L,
        val swapFreeBytes: Long = 0L,
    ) {
        val ramUsedFraction: Float
            get() = if (totalBytes > 0L) ((totalBytes - availableBytes).coerceAtLeast(0L).toFloat() / totalBytes.toFloat()) else 0f
        val zramUsedFraction: Float
            get() = if (swapTotalBytes > 0L) ((swapTotalBytes - swapFreeBytes).coerceAtLeast(0L).toFloat() / swapTotalBytes.toFloat()) else 0f
    }

    data class CpuCluster(
        val role: String,
        val governor: String,
        val curKhz: Long,
        val minKhz: Long,
        val maxKhz: Long,
        val capacity: Long,
        val availableGovernors: List<String>,
    )

    data class ThermalZone(
        val id: String,
        val type: String,
        val category: String,
        val celsius: Float,
    )

    data class DecisionEpisode(
        val id: Long,
        val tsMs: Long,
        val domain: String,
        val trigger: String,
        val action: String,
        val outcome: String,
        val socTempC: Float?,
        val renderPressure: Float,
        val capacityScale: Float,
    )

    data class HistorySample(
        val tsMs: Long,
        val temp: Float?,
        val pressure: Float,
        val cpu: Float,
        val scale: Float,
        val mode: String,
    )

    data class PowerConfig(
        val bypassCharging: Boolean = false,
        val bypassOnGaming: Boolean = false,
        val chargeLimitPct: Int = 0,
    )

    data class BatteryPower(
        val available: Boolean = false,
        val charging: Boolean = false,
        val fastCharging: Boolean = false,
        val thermalStepActive: Boolean = false,
        val temperatureCelsius: Float? = null,
        val currentMa: Float = 0f,
        val voltageMv: Float = 0f,
        val powerMw: Float = 0f,
        val capacityPct: Int? = null,
        val bypassSupported: Boolean = false,
        val bypassActive: Boolean = false,
        val bypassReason: String = "off",
        val chargeLimitPct: Int = 0,
        val status: String = "Unknown",
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

    data class EngineFeatures(
        val frameSmoothness: Boolean = true,
        val storageBurst: Boolean = true,
        val memoryGuard: Boolean = true,
        val thermalPacing: Boolean = true,
        val batteryStepCharging: Boolean = true,
    )

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
        val gpuBusyPct: Float?,
        val cpuPsi: Float?,
        val memPsi: Float?,
        val ioPsi: Float?,
        val aiPredicted10s: Float,
        val aiConfidence: Float,
        val baselineNodesSnapshotted: Int,
        val recoveryPending: Boolean,
        val thermalSource: String,
        val thermalVelocityCpm: Float = 0f,
        val thermalGuardActive: Boolean = false,
        val features: EngineFeatures = EngineFeatures(),
        val powerConfig: PowerConfig = PowerConfig(),
        val batteryPower: BatteryPower = BatteryPower(),
        val hardware: HardwareInfo = HardwareInfo(),
        val memory: MemoryInfo = MemoryInfo(),
        val cpuClusters: List<CpuCluster> = emptyList(),
        val thermalZones: List<ThermalZone> = emptyList(),
        val episodes: List<DecisionEpisode> = emptyList(),
        val history: List<HistorySample> = emptyList(),
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

    data class ProfileImportOutcome(
        val appliedCount: Int,
        val sourceSoc: String,
        val socMismatchWarning: Boolean,
    )

    suspend fun checkRootAccess(): Boolean = withContext(Dispatchers.IO) {
        runCatching { Shell.getShell().isRoot }.getOrDefault(false)
    }

    suspend fun getStatus(): Result<DaemonStatus> = withContext(Dispatchers.IO) {
        runCatching {
            val output = execRootCommand("$RESOLVE_DAEMON; \"\$BIN\" --status")
            val lastLine = output.trim().lines().lastOrNull { it.trim().startsWith("{") }
                ?: throw IllegalStateException("Empty daemon output")
            val json = JSONObject(lastLine)
            val actuators = json.optJSONObject("actuators")
            val featuresJson = json.optJSONObject("features")
            val powerCfgJson = json.optJSONObject("powerConfig")
            val battJson = json.optJSONObject("batteryPower")
            val hardwareJson = json.optJSONObject("hardware")
            val memJson = json.optJSONObject("memory")
            val sessionJson = json.optJSONObject("session")
            val appsJson = json.optJSONObject("apps")

            fun JSONObject?.optFiniteFloat(key: String, fallback: Float = -1f): Float {
                if (this == null || isNull(key)) return fallback
                val v = optDouble(key, fallback.toDouble())
                return if (v.isFinite()) v.toFloat() else fallback
            }

            fun JSONObject?.optNullableFloat(key: String): Float? {
                if (this == null || isNull(key)) return null
                val v = optDouble(key, Double.NaN)
                return if (v.isFinite()) v.toFloat() else null
            }

            fun JSONObject?.optNullableInt(key: String): Int? {
                if (this == null || isNull(key)) return null
                val v = optInt(key, Int.MIN_VALUE)
                return if (v == Int.MIN_VALUE) null else v
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

            val clusters = buildList {
                val arr = json.optJSONArray("cpuClusters") ?: return@buildList
                for (i in 0 until arr.length()) {
                    val obj = arr.optJSONObject(i) ?: continue
                    add(
                        CpuCluster(
                            role = obj.optString("role", "unknown"),
                            governor = obj.optString("governor", "unknown"),
                            curKhz = obj.optLong("curKhz", 0L),
                            minKhz = obj.optLong("minKhz", 0L),
                            maxKhz = obj.optLong("maxKhz", 0L),
                            capacity = obj.optLong("capacity", 0L),
                            availableGovernors = obj.optJSONArray("availableGovernors").toStringList(),
                        ),
                    )
                }
            }

            val thermalZones = buildList {
                val arr = json.optJSONArray("thermalZones") ?: return@buildList
                for (i in 0 until arr.length()) {
                    val obj = arr.optJSONObject(i) ?: continue
                    val c = obj.optFiniteFloat("celsius")
                    if (c > 0f) {
                        add(
                            ThermalZone(
                                id = obj.optString("id", "zone$i"),
                                type = obj.optString("type", "sensor"),
                                category = obj.optString("category", "sensor"),
                                celsius = c,
                            ),
                        )
                    }
                }
            }

            val episodes = buildList {
                val arr = json.optJSONArray("episodes") ?: return@buildList
                for (i in 0 until arr.length()) {
                    val obj = arr.optJSONObject(i) ?: continue
                    add(
                        DecisionEpisode(
                            id = obj.optLong("id", 0L),
                            tsMs = obj.optLong("tsMs", 0L),
                            domain = obj.optString("domain", "context"),
                            trigger = obj.optString("trigger", ""),
                            action = obj.optString("action", ""),
                            outcome = obj.optString("outcome", "verified"),
                            socTempC = obj.optNullableFloat("socTempC"),
                            renderPressure = obj.optFiniteFloat("renderPressure", 0f),
                            capacityScale = obj.optFiniteFloat("capacityScale", 1f),
                        ),
                    )
                }
            }

            val history = buildList {
                val arr = json.optJSONArray("history") ?: return@buildList
                for (i in 0 until arr.length()) {
                    val obj = arr.optJSONObject(i) ?: continue
                    add(
                        HistorySample(
                            tsMs = obj.optLong("ts", 0L),
                            temp = obj.optNullableFloat("temp"),
                            pressure = obj.optFiniteFloat("pressure", 0f),
                            cpu = obj.optFiniteFloat("cpu", 0f),
                            scale = obj.optFiniteFloat("scale", 1f),
                            mode = obj.optString("mode", "EFFICIENCY"),
                        ),
                    )
                }
            }

            DaemonStatus(
                version = json.optString("version", "6.0.0"),
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
                gpuBusyPct = json.optNullableFloat("gpuBusyPct"),
                cpuPsi = json.optNullableFloat("cpuPsi"),
                memPsi = json.optNullableFloat("memPsi"),
                ioPsi = json.optNullableFloat("ioPsi"),
                aiPredicted10s = json.optFiniteFloat("aiPredicted10s", 0f),
                aiConfidence = json.optFiniteFloat("aiConfidence", 0f),
                baselineNodesSnapshotted = json.optInt("baselineNodesSnapshotted", 0),
                recoveryPending = json.optBoolean("recoveryPending", false),
                thermalSource = json.optString("thermalSource", "oem_policy"),
                thermalVelocityCpm = json.optFiniteFloat("thermalVelocityCpm", 0f),
                thermalGuardActive = json.optBoolean("thermalGuard", false),
                features = EngineFeatures(
                    frameSmoothness = featuresJson?.optBoolean("frameSmoothness", true) ?: true,
                    storageBurst = featuresJson?.optBoolean("storageBurst", true) ?: true,
                    memoryGuard = featuresJson?.optBoolean("memoryGuard", true) ?: true,
                    thermalPacing = featuresJson?.optBoolean("thermalPacing", true) ?: true,
                    batteryStepCharging = featuresJson?.optBoolean("batteryStepCharging", true) ?: true,
                ),
                powerConfig = PowerConfig(
                    bypassCharging = powerCfgJson?.optBoolean("bypassCharging", false) ?: false,
                    bypassOnGaming = powerCfgJson?.optBoolean("bypassOnGaming", false) ?: false,
                    chargeLimitPct = powerCfgJson?.optInt("chargeLimitPct", 0) ?: 0,
                ),
                batteryPower = BatteryPower(
                    available = battJson?.optBoolean("available", false) ?: false,
                    charging = battJson?.optBoolean("charging", false) ?: false,
                    fastCharging = battJson?.optBoolean("fastCharging", false) ?: false,
                    thermalStepActive = battJson?.optBoolean("thermalStepActive", false) ?: false,
                    temperatureCelsius = battJson.optNullableFloat("temperatureCelsius"),
                    currentMa = battJson.optFiniteFloat("currentMa", 0f),
                    voltageMv = battJson.optFiniteFloat("voltageMv", 0f),
                    powerMw = battJson.optFiniteFloat("powerMw", 0f),
                    capacityPct = battJson.optNullableInt("capacityPct"),
                    bypassSupported = battJson?.optBoolean("bypassSupported", false) ?: false,
                    bypassActive = battJson?.optBoolean("bypassActive", false) ?: false,
                    bypassReason = battJson?.optString("bypassReason", "off").orEmpty(),
                    chargeLimitPct = battJson?.optInt("chargeLimitPct", 0) ?: 0,
                    status = battJson?.optString("status", "Unknown").orEmpty(),
                ),
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
                memory = MemoryInfo(
                    totalBytes = memJson?.optLong("totalBytes", 0L) ?: 0L,
                    availableBytes = memJson?.optLong("availableBytes", 0L) ?: 0L,
                    swapTotalBytes = memJson?.optLong("swapTotalBytes", 0L) ?: 0L,
                    swapFreeBytes = memJson?.optLong("swapFreeBytes", 0L) ?: 0L,
                ),
                cpuClusters = clusters,
                thermalZones = thermalZones,
                episodes = episodes,
                history = history,
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

    suspend fun setFeatureToggle(key: String, enabled: Boolean): Result<Unit> = withContext(Dispatchers.IO) {
        runCatching {
            require(key in ALLOWED_FEATURE_KEYS) { "Unsupported feature key: $key" }
            val valStr = if (enabled) "true" else "false"
            execRootCommand("$RESOLVE_DAEMON; \"\$BIN\" --set-config '$key=$valStr'")
            Unit
        }
    }

    suspend fun setPowerControl(key: String, value: String): Result<Unit> = withContext(Dispatchers.IO) {
        runCatching {
            require(key in ALLOWED_POWER_KEYS) { "Unsupported power key: $key" }
            val safeVal = when (key) {
                "bypass_charging", "bypass_on_gaming" -> if (value == "true") "true" else "false"
                "charge_limit_pct" -> {
                    val n = value.toIntOrNull() ?: 0
                    if (n in 60..95) n.toString() else "0"
                }
                else -> error("Invalid key")
            }
            execRootCommand("$RESOLVE_DAEMON; \"\$BIN\" --set-config '$key=$safeVal'")
            Unit
        }
    }

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

    fun buildShareableProfileJson(status: DaemonStatus): String {
        val root = JSONObject().apply {
            put("format", "coreflow-profile-v6")
            put("version", status.version)
            put("sourceSoc", status.hardware.socModel)
            put("exportedAtMs", System.currentTimeMillis())
            put("profile", status.profile.configValue)
            put(
                "features",
                JSONObject().apply {
                    put("enable_frame_smoothness", status.features.frameSmoothness)
                    put("enable_storage_burst", status.features.storageBurst)
                    put("enable_memory_guard", status.features.memoryGuard)
                    put("enable_thermal_pacing", status.features.thermalPacing)
                    put("enable_battery_step_charging", status.features.batteryStepCharging)
                },
            )
            put(
                "power",
                JSONObject().apply {
                    put("bypass_on_gaming", status.powerConfig.bypassOnGaming)
                    put("charge_limit_pct", status.powerConfig.chargeLimitPct)
                },
            )
            put(
                "apps",
                JSONObject().apply {
                    put("gaming_apps", JSONArray(status.apps.gaming))
                    put("smoothness_apps", JSONArray(status.apps.smoothness))
                    put("battery_apps", JSONArray(status.apps.battery))
                    put("excluded_apps", JSONArray(status.apps.excluded))
                },
            )
        }
        return root.toString(2)
    }

    suspend fun importShareableProfileJson(
        rawJson: String,
        currentSoc: String,
    ): Result<ProfileImportOutcome> = withContext(Dispatchers.IO) {
        runCatching {
            val root = JSONObject(rawJson)
            require(root.optString("format").startsWith("coreflow-profile")) {
                "Unsupported profile format"
            }
            val sourceSoc = root.optString("sourceSoc", "Unknown")
            val socMismatch = currentSoc.isNotBlank() &&
                sourceSoc.isNotBlank() &&
                !sourceSoc.equals(currentSoc, ignoreCase = true)

            val updates = mutableListOf<String>()
            val prof = Profile.fromConfigValue(root.optString("profile", "balanced"))
            updates.add("profile=${prof.configValue}")

            root.optJSONObject("features")?.let { feat ->
                for (key in ALLOWED_FEATURE_KEYS) {
                    if (feat.has(key)) {
                        updates.add("$key=${if (feat.optBoolean(key, true)) "true" else "false"}")
                    }
                }
            }
            root.optJSONObject("power")?.let { pwr ->
                if (pwr.has("bypass_on_gaming")) {
                    updates.add("bypass_on_gaming=${if (pwr.optBoolean("bypass_on_gaming", false)) "true" else "false"}")
                }
                if (pwr.has("charge_limit_pct")) {
                    val limit = pwr.optInt("charge_limit_pct", 0)
                    updates.add("charge_limit_pct=${if (limit in 60..95) limit else 0}")
                }
            }
            root.optJSONObject("apps")?.let { apps ->
                for (key in listOf("gaming_apps", "smoothness_apps", "battery_apps", "excluded_apps")) {
                    val arr = apps.optJSONArray(key) ?: continue
                    val validPkgs = mutableListOf<String>()
                    for (i in 0 until arr.length()) {
                        val pkg = arr.optString(i, "").trim()
                        if (PACKAGE_REGEX.matches(pkg)) validPkgs.add(pkg)
                    }
                    updates.add("$key=${validPkgs.distinct().joinToString(",")}")
                }
            }

            if (updates.isNotEmpty()) {
                val cmd = buildString {
                    append(RESOLVE_DAEMON)
                    for (u in updates) {
                        append("; \"\$BIN\" --set-config '")
                        append(u)
                        append("'")
                    }
                }
                execRootCommand(cmd)
            }
            ProfileImportOutcome(
                appliedCount = updates.size,
                sourceSoc = sourceSoc,
                socMismatchWarning = socMismatch,
            )
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

    suspend fun getAtlasJson(): Result<String> = withContext(Dispatchers.IO) {
        runCatching {
            execRootCommand("$RESOLVE_DAEMON; \"\$BIN\" --atlas").trim()
        }
    }

    suspend fun getDiagJson(): Result<String> = withContext(Dispatchers.IO) {
        runCatching {
            execRootCommand("$RESOLVE_DAEMON; \"\$BIN\" --diag").trim()
        }
    }

    /**
     * Reads raw SurfaceFlinger frame flip counter via libsu for real FPS delta calculation.
     * Returns null when unavailable — never fabricates a fake FPS number.
     */
    suspend fun readSurfaceFlingerFlipCount(): Long? = withContext(Dispatchers.IO) {
        runCatching {
            val res = Shell.cmd("service call SurfaceFlinger 1013").exec()
            if (!res.isSuccess) return@runCatching null
            val text = res.out.joinToString(" ")
            val match = Regex("Parcel\\(\\s*([0-9a-fA-F]{8})").find(text) ?: return@runCatching null
            match.groupValues[1].toLong(16)
        }.getOrNull()
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
