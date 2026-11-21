package io.coreflow.ui

import android.app.StatusBarManager
import android.content.ComponentName
import android.content.Intent
import android.graphics.drawable.Icon as AndroidIcon
import android.net.Uri
import android.provider.Settings
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ExperimentalLayoutApi
import androidx.compose.foundation.layout.FlowRow
import androidx.compose.foundation.layout.IntrinsicSize
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.nestedscroll.nestedScroll
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import io.coreflow.CoreFlowBypassTileService
import io.coreflow.CoreFlowManager
import io.coreflow.CoreFlowOverlayService
import io.coreflow.CoreFlowTileService
import io.coreflow.R
import top.yukonga.miuix.kmp.basic.Card
import top.yukonga.miuix.kmp.basic.PullToRefresh
import top.yukonga.miuix.kmp.basic.ScrollBehavior
import top.yukonga.miuix.kmp.basic.SmallTitle
import top.yukonga.miuix.kmp.basic.Text
import top.yukonga.miuix.kmp.basic.rememberPullToRefreshState
import top.yukonga.miuix.kmp.preference.ArrowPreference
import top.yukonga.miuix.kmp.preference.OverlayDropdownPreference
import top.yukonga.miuix.kmp.preference.SwitchPreference
import top.yukonga.miuix.kmp.theme.MiuixTheme
import top.yukonga.miuix.kmp.utils.PressFeedbackType
import top.yukonga.miuix.kmp.utils.overScrollVertical
import top.yukonga.miuix.kmp.utils.scrollEndHaptic
import java.util.Locale

/**
 * CoreFlow v6.1 Autonomous Co-Pilot Control Hub.
 * Follows the strict "Absent, Not Disabled" principle: hardware controls appear only
 * when CoreFlow Atlas proves the underlying kernel interface exists on this device.
 */
@OptIn(ExperimentalLayoutApi::class)
@Composable
fun ControlHubTab(
    strings: AppStrings,
    status: CoreFlowManager.DaemonStatus?,
    busy: Boolean,
    isRefreshing: Boolean,
    contentPadding: PaddingValues,
    scrollBehavior: ScrollBehavior,
    onRefresh: () -> Unit,
    onToggleFeature: (key: String, label: String, enabled: Boolean) -> Unit,
    onSetPowerControl: (key: String, value: String, msg: String) -> Unit,
    onMessage: (String) -> Unit,
) {
    val context = LocalContext.current
    val overlayRunning by CoreFlowOverlayService.running.collectAsState()
    var hudIntervalIdx by rememberSaveable { mutableIntStateOf(1) }
    val hudIntervalLabels = remember { listOf("Fast (1.0s)", "Balanced (1.5s)", "Battery (2.5s)") }
    val hudIntervalValues = remember { listOf(1000L, 1500L, 2500L) }

    val features = status?.features ?: CoreFlowManager.EngineFeatures()
    val powerCfg = status?.powerConfig ?: CoreFlowManager.PowerConfig()
    val batt = status?.batteryPower ?: CoreFlowManager.BatteryPower()
    val mem = status?.memory ?: CoreFlowManager.MemoryInfo()
    val clusters = status?.cpuClusters.orEmpty()
    val zones = status?.thermalZones.orEmpty()

    val chargeLimitOptions = remember { listOf("Off", "80%", "85%", "90%") }
    val chargeLimitValues = remember { listOf(0, 80, 85, 90) }
    val chargeLimitIdx = chargeLimitValues.indexOf(powerCfg.chargeLimitPct).coerceAtLeast(0)
    val pullState = rememberPullToRefreshState()

    PullToRefresh(
        isRefreshing = isRefreshing,
        onRefresh = onRefresh,
        pullToRefreshState = pullState,
        topAppBarScrollBehavior = scrollBehavior,
        contentPadding = PaddingValues(top = contentPadding.calculateTopPadding()),
        refreshTexts = strings.pullToRefreshTexts,
        modifier = Modifier.fillMaxSize(),
    ) {
        LazyColumn(
            modifier = Modifier
                .fillMaxSize()
                .overScrollVertical()
                .scrollEndHaptic()
                .nestedScroll(scrollBehavior.nestedScrollConnection),
            contentPadding = PaddingValues(
                start = 12.dp,
                end = 12.dp,
                top = contentPadding.calculateTopPadding() + 8.dp,
                bottom = contentPadding.calculateBottomPadding() + 16.dp,
            ),
            verticalArrangement = Arrangement.spacedBy(10.dp),
        ) {
            // 0. Autonomous Co-Pilot Dynamic Subsystem Knobs
            item { SmallTitle(text = strings.sectionCopilot) }
            item {
                val discovered = status?.semanticKnobsDiscovered ?: 0
                val active = status?.semanticKnobsActive ?: 0
                val yielded = status?.oemYieldedNodes ?: 0
                val leaves = status?.semanticKnobLeaves.orEmpty()

                Card(
                    modifier = Modifier.fillMaxWidth(),
                    insideMargin = PaddingValues(14.dp),
                    pressFeedbackType = PressFeedbackType.Tilt,
                    onClick = onRefresh,
                ) {
                    Row(
                        modifier = Modifier
                            .fillMaxWidth()
                            .height(IntrinsicSize.Min),
                        horizontalArrangement = Arrangement.spacedBy(8.dp),
                    ) {
                        MetricPill(
                            label = "Discovered",
                            value = "$discovered",
                            accent = Color(0xFF38BDF8),
                            modifier = Modifier.weight(1f).fillMaxHeight(),
                        )
                        MetricPill(
                            label = "Co-Pilot",
                            value = if (active > 0) "$active ${strings.copilotActive}" else strings.copilotStandby,
                            accent = if (active > 0) Color(0xFF22C55E) else Color(0xFFA78BFA),
                            modifier = Modifier.weight(1.25f).fillMaxHeight(),
                        )
                        MetricPill(
                            label = "OEM Yield",
                            value = "$yielded",
                            accent = if (yielded > 0) Color(0xFFF59E0B) else Color(0xFF22C55E),
                            modifier = Modifier.weight(0.9f).fillMaxHeight(),
                        )
                    }
                    if (leaves.isNotEmpty()) {
                        Spacer(Modifier.height(10.dp))
                        FlowRow(
                            modifier = Modifier.fillMaxWidth(),
                            horizontalArrangement = Arrangement.spacedBy(6.dp),
                            verticalArrangement = Arrangement.spacedBy(6.dp),
                        ) {
                            for (leaf in leaves.take(12)) {
                                Box(
                                    modifier = Modifier
                                        .clip(RoundedCornerShape(8.dp))
                                        .background(MiuixTheme.colorScheme.secondaryContainer.copy(alpha = 0.55f))
                                        .padding(horizontal = 8.dp, vertical = 4.dp),
                                ) {
                                    Text(
                                        text = leaf,
                                        fontSize = 10.sp,
                                        fontWeight = FontWeight.Medium,
                                        color = MiuixTheme.colorScheme.onSurface,
                                        maxLines = 1,
                                        overflow = TextOverflow.Ellipsis,
                                    )
                                }
                            }
                        }
                    }
                }
            }

            // 1. CPU & EAS Scheduler Domain
            item { SmallTitle(text = strings.sectionCpu) }
            item {
                Card(modifier = Modifier.fillMaxWidth()) {
                    if (clusters.isNotEmpty()) {
                        Column(
                            modifier = Modifier.padding(horizontal = 14.dp, vertical = 12.dp),
                            verticalArrangement = Arrangement.spacedBy(8.dp),
                        ) {
                            for (c in clusters) {
                                val curMhz = if (c.curKhz > 0L) "${c.curKhz / 1000L} MHz" else "--"
                                val rangeMhz = if (c.maxKhz > 0L) "${c.minKhz / 1000L}–${c.maxKhz / 1000L} MHz" else "--"
                                val frac = if (c.maxKhz > 0L && c.curKhz > 0L) {
                                    (c.curKhz.toFloat() / c.maxKhz.toFloat()).coerceIn(0.05f, 1f)
                                } else {
                                    0f
                                }
                                Column(verticalArrangement = Arrangement.spacedBy(4.dp)) {
                                    Row(
                                        modifier = Modifier.fillMaxWidth(),
                                        horizontalArrangement = Arrangement.SpaceBetween,
                                        verticalAlignment = Alignment.CenterVertically,
                                    ) {
                                        Text(
                                            text = "${c.role.uppercase(Locale.US)} · ${c.governor}",
                                            style = MiuixTheme.textStyles.body2,
                                            fontWeight = FontWeight.SemiBold,
                                        )
                                        Text(
                                            text = "$curMhz  ($rangeMhz)",
                                            fontSize = 11.sp,
                                            color = MiuixTheme.colorScheme.onSurfaceVariantSummary,
                                        )
                                    }
                                    UsageBar(fraction = frac, color = MiuixTheme.colorScheme.primary)
                                }
                            }
                        }
                    }
                    SwitchPreference(
                        checked = features.frameSmoothness,
                        onCheckedChange = { onToggleFeature("enable_frame_smoothness", "Frame Smoothness", it) },
                        title = "Per-TID Frame Smoothness",
                        summary = "Closed-loop RenderThread/GameThread UClamp (${status?.uclampMin ?: 0}..${status?.uclampMax ?: 1024})",
                        enabled = !busy,
                    )
                }
            }

            // 2. GPU, Display & Thermal Zones Domain
            item { SmallTitle(text = strings.sectionGpuThermal) }
            item {
                Card(modifier = Modifier.fillMaxWidth()) {
                    Column(
                        modifier = Modifier.padding(horizontal = 14.dp, vertical = 12.dp),
                        verticalArrangement = Arrangement.spacedBy(8.dp),
                    ) {
                        Row(
                            modifier = Modifier
                                .fillMaxWidth()
                                .height(IntrinsicSize.Min),
                            horizontalArrangement = Arrangement.spacedBy(8.dp),
                        ) {
                            MetricPill(
                                label = "GPU Model",
                                value = status?.hardware?.gpuModel?.ifBlank { "Devfreq" } ?: "--",
                                accent = Color(0xFF38BDF8),
                                modifier = Modifier
                                    .weight(1.25f)
                                    .fillMaxHeight(),
                            )
                            MetricPill(
                                label = "GPU Busy",
                                value = status?.gpuBusyPct?.let { "${it.toInt().coerceIn(0, 100)}%" } ?: "--",
                                accent = Color(0xFFA78BFA),
                                modifier = Modifier
                                    .weight(0.8f)
                                    .fillMaxHeight(),
                            )
                            MetricPill(
                                label = "Refresh",
                                value = status?.hardware?.refreshHz?.let { "$it Hz" } ?: "--",
                                accent = Color(0xFF22C55E),
                                modifier = Modifier
                                    .weight(0.8f)
                                    .fillMaxHeight(),
                            )
                        }
                    if (zones.isNotEmpty()) {
                        val zonePairs = zones.take(6).chunked(2)
                        for (pair in zonePairs) {
                            Row(
                                modifier = Modifier
                                    .fillMaxWidth()
                                    .height(IntrinsicSize.Min),
                                horizontalArrangement = Arrangement.spacedBy(8.dp),
                            ) {
                                for (z in pair) {
                                    val zColor = when {
                                        z.celsius >= 55f -> Color(0xFFEF4444)
                                        z.celsius >= 45f -> Color(0xFFF59E0B)
                                        else -> Color(0xFF22C55E)
                                    }
                                    MetricPill(
                                        label = z.type,
                                        value = String.format(Locale.US, "%.1f°C", z.celsius),
                                        accent = zColor,
                                        modifier = Modifier
                                            .weight(1f)
                                            .fillMaxHeight(),
                                    )
                                }
                                if (pair.size == 1) Spacer(Modifier.weight(1f))
                            }
                        }
                    }
                }
                SwitchPreference(
                    checked = features.thermalPacing,
                    onCheckedChange = { onToggleFeature("enable_thermal_pacing", "Thermal Pacing", it) },
                    title = "Rust PID + RLS Thermal Pacing",
                    summary = "Source: ${status?.thermalSource ?: "oem_policy"} · Managed: ${status?.managedEnvelopeNodes ?: 0} nodes",
                    enabled = !busy,
                )
            }
        }

        // 3. Power & CFJ4 Bypass Charging Domain
        item { SmallTitle(text = strings.sectionPower) }
        item {
            Card(modifier = Modifier.fillMaxWidth()) {
                if (batt.available) {
                    Row(
                        modifier = Modifier
                            .fillMaxWidth()
                            .padding(horizontal = 14.dp, vertical = 12.dp)
                            .height(IntrinsicSize.Min),
                        horizontalArrangement = Arrangement.spacedBy(8.dp),
                    ) {
                        MetricPill(
                            label = "Battery",
                            value = batt.capacityPct?.let { "$it%" } ?: batt.status,
                            accent = if (batt.bypassActive) Color(0xFF10B981) else Color(0xFF22C55E),
                            modifier = Modifier
                                .weight(1f)
                                .fillMaxHeight(),
                        )
                        MetricPill(
                            label = "Power",
                            value = if (batt.powerMw > 0f) String.format(Locale.US, "%.1f W", batt.powerMw / 1000f) else "--",
                            accent = Color(0xFF38BDF8),
                            modifier = Modifier
                                .weight(1f)
                                .fillMaxHeight(),
                        )
                        MetricPill(
                            label = "Batt Temp",
                            value = batt.temperatureCelsius?.let { String.format(Locale.US, "%.1f°C", it) } ?: "--",
                            accent = Color(0xFFF59E0B),
                            modifier = Modifier
                                .weight(1f)
                                .fillMaxHeight(),
                        )
                    }
                }

                // Capability-driven ("Absent, Not Disabled"): show Bypass controls ONLY if kernel supports it
                if (batt.bypassSupported) {
                    SwitchPreference(
                        checked = powerCfg.bypassCharging,
                        onCheckedChange = {
                            onSetPowerControl(
                                "bypass_charging",
                                if (it) "true" else "false",
                                if (it) "Bypass Charging enabled (CFJ4 protected)" else "Bypass Charging disabled",
                            )
                        },
                        title = "Bypass Charging",
                        summary = if (batt.bypassActive) {
                            "Active (${batt.bypassReason}) — powering phone directly without heating battery"
                        } else {
                            "Suspend battery charging while plugged in (zero-trace CFJ4 rollback)"
                        },
                        enabled = !busy,
                    )
                    SwitchPreference(
                        checked = powerCfg.bypassOnGaming,
                        onCheckedChange = {
                            onSetPowerControl(
                                "bypass_on_gaming",
                                if (it) "true" else "false",
                                if (it) "Auto Bypass in Games enabled" else "Auto Bypass in Games disabled",
                            )
                        },
                        title = "Auto Bypass in Gaming Mode",
                        summary = "Automatically engage bypass charging whenever a game is active",
                        enabled = !busy,
                    )
                    OverlayDropdownPreference(
                        items = chargeLimitOptions,
                        selectedIndex = chargeLimitIdx,
                        title = "Battery Charge Ceiling",
                        summary = "Pause charging at target % with 3% hysteresis to extend battery lifespan",
                        enabled = !busy,
                        onSelectedIndexChange = { idx ->
                            val target = chargeLimitValues[idx]
                            onSetPowerControl(
                                "charge_limit_pct",
                                target.toString(),
                                if (target == 0) "Charge Ceiling disabled" else "Charge Ceiling set to $target%",
                            )
                        },
                    )
                }

                SwitchPreference(
                    checked = features.batteryStepCharging,
                    onCheckedChange = { onToggleFeature("enable_battery_step_charging", "Thermal Step Charging", it) },
                    title = "Thermal Step Charging Guard",
                    summary = if (batt.thermalStepActive) "Stepped down due to battery heat" else "Protects battery longevity during hot fast-charging",
                    enabled = !busy,
                )
            }
        }

        // 4. Memory & Storage Domain
        item { SmallTitle(text = strings.sectionMemStorage) }
        item {
            Card(modifier = Modifier.fillMaxWidth()) {
                Column(
                    modifier = Modifier.padding(horizontal = 14.dp, vertical = 12.dp),
                    verticalArrangement = Arrangement.spacedBy(8.dp),
                ) {
                    if (mem.totalBytes > 0L) {
                        val usedGb = (mem.totalBytes - mem.availableBytes).coerceAtLeast(0L) / (1024.0 * 1024.0 * 1024.0)
                        val totGb = mem.totalBytes / (1024.0 * 1024.0 * 1024.0)
                        Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween) {
                            Text("RAM Usage", style = MiuixTheme.textStyles.body2, fontWeight = FontWeight.Medium)
                            Text(
                                String.format(Locale.US, "%.1f / %.1f GB", usedGb, totGb),
                                fontSize = 11.sp,
                                color = MiuixTheme.colorScheme.onSurfaceVariantSummary,
                            )
                        }
                        UsageBar(fraction = mem.ramUsedFraction, color = Color(0xFF38BDF8))
                    }
                    if (mem.swapTotalBytes > 0L) {
                        val usedZ = (mem.swapTotalBytes - mem.swapFreeBytes).coerceAtLeast(0L) / (1024.0 * 1024.0 * 1024.0)
                        val totZ = mem.swapTotalBytes / (1024.0 * 1024.0 * 1024.0)
                        Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween) {
                            Text("ZRAM / Swap", style = MiuixTheme.textStyles.body2, fontWeight = FontWeight.Medium)
                            Text(
                                String.format(Locale.US, "%.1f / %.1f GB", usedZ, totZ),
                                fontSize = 11.sp,
                                color = MiuixTheme.colorScheme.onSurfaceVariantSummary,
                            )
                        }
                        UsageBar(fraction = mem.zramUsedFraction, color = Color(0xFFA78BFA))
                    }
                    Row(
                        modifier = Modifier
                            .fillMaxWidth()
                            .height(IntrinsicSize.Min),
                        horizontalArrangement = Arrangement.spacedBy(8.dp),
                    ) {
                        MetricPill(
                            label = "CPU PSI",
                            value = status?.cpuPsi?.let { String.format(Locale.US, "%.1f%%", it) } ?: "--",
                            modifier = Modifier.weight(1f).fillMaxHeight(),
                        )
                        MetricPill(
                            label = "Mem PSI",
                            value = status?.memPsi?.let { String.format(Locale.US, "%.1f%%", it) } ?: "--",
                            modifier = Modifier.weight(1f).fillMaxHeight(),
                        )
                        MetricPill(
                            label = "I/O PSI",
                            value = status?.ioPsi?.let { String.format(Locale.US, "%.1f%%", it) } ?: "--",
                            modifier = Modifier.weight(1f).fillMaxHeight(),
                        )
                    }
                }
                val hf = status?.hyperFluid ?: CoreFlowManager.HyperFluidStats()
                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(horizontal = 14.dp, vertical = 6.dp)
                        .height(IntrinsicSize.Min),
                    horizontalArrangement = Arrangement.spacedBy(8.dp),
                ) {
                    MetricPill(
                        label = if (strings.isIndonesian) "Mem Health" else "Mem Health",
                        value = "${hf.memoryHealthPct}%",
                        accent = if (hf.memoryHealthPct >= 75) Color(0xFF22C55E) else Color(0xFFF59E0B),
                        modifier = Modifier.weight(1f).fillMaxHeight(),
                    )
                    MetricPill(
                        label = "Compactions",
                        value = "${hf.idleCompactions}",
                        accent = Color(0xFF38BDF8),
                        modifier = Modifier.weight(1f).fillMaxHeight(),
                    )
                    MetricPill(
                        label = "Preloads",
                        value = "${hf.predictivePreloads}",
                        accent = Color(0xFFA78BFA),
                        modifier = Modifier.weight(1f).fillMaxHeight(),
                    )
                }
                SwitchPreference(
                    checked = features.storageBurst,
                    onCheckedChange = { onToggleFeature("enable_storage_burst", "Storage Burst", it) },
                    title = "Dynamic ${status?.hardware?.storageType ?: "UFS"} Launch Burst",
                    summary = "Active read-ahead: ${status?.readAheadKb ?: 0} KB · Background I/O isolation",
                    enabled = !busy,
                )
                SwitchPreference(
                    checked = features.memoryGuard,
                    onCheckedChange = { onToggleFeature("enable_memory_guard", "Memory Guard", it) },
                    title = "MGLRU & Smart Pre-Read",
                    summary = if (status?.hardware?.mglruAvailable == true) {
                        "Linux MGLRU active · Working-set anti-stutter guard"
                    } else {
                        "Working-set anti-stutter & posix_fadvise pre-warm"
                    },
                    enabled = !busy,
                )
                SwitchPreference(
                    checked = features.idleCompaction,
                    onCheckedChange = { onToggleFeature("enable_idle_compaction", "Idle Compaction", it) },
                    title = if (strings.isIndonesian) "Anti-Aging Memory Compaction" else "9-Hour Anti-Aging Compaction",
                    summary = if (strings.isIndonesian) {
                        "Defragmentasi RAM otomatis saat layar mati & dingin (< 42°C)"
                    } else {
                        "Proactive RAM defragmentation during cool ScreenOff idle (< 42°C)"
                    },
                    enabled = !busy,
                )
                SwitchPreference(
                    checked = features.predictivePreload,
                    onCheckedChange = { onToggleFeature("enable_predictive_preload", "Predictive Preloader", it) },
                    title = if (strings.isIndonesian) "Rust Predictive App Preloader" else "Rust Predictive App Preloader",
                    summary = if (hf.lastPredictedPackage.isNotBlank()) {
                        "Last pre-warmed: ${hf.lastPredictedPackage}"
                    } else if (strings.isIndonesian) {
                        "Prediksi Markov (<1 KB RAM) menghangatkan APK/SO sebelum dibuka"
                    } else {
                        "Markov predictor (<1 KB RAM) pre-warms APK/SO pages before launch"
                    },
                    enabled = !busy,
                )
                SwitchPreference(
                    checked = features.cameraBoost,
                    onCheckedChange = { onToggleFeature("enable_camera_boost", "Camera Full-Chain", it) },
                    title = if (strings.isIndonesian) "Fast-Launch Kamera & Video Chain" else "Fast-Launch Camera & Video Chain",
                    summary = if (strings.isIndonesian) {
                        "Boost Per-TID UClamp untuk cameraserver, Camera HAL & MediaCodec"
                    } else {
                        "Per-TID UClamp full-chain boost for cameraserver, Camera HAL & MediaCodec"
                    },
                    enabled = !busy,
                )
            }
        }

        // 5. Floating Game HUD & Quick Settings Tiles
        item { SmallTitle(text = strings.sectionHudTiles) }
        item {
            Card(modifier = Modifier.fillMaxWidth()) {
                SwitchPreference(
                    checked = overlayRunning,
                    onCheckedChange = { enable ->
                        if (enable) {
                            if (!Settings.canDrawOverlays(context)) {
                                val intent = Intent(
                                    Settings.ACTION_MANAGE_OVERLAY_PERMISSION,
                                    Uri.parse("package:${context.packageName}"),
                                ).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                                context.startActivity(intent)
                                onMessage("Enable 'Display over other apps' then toggle HUD on")
                            } else {
                                CoreFlowOverlayService.start(context, hudIntervalValues[hudIntervalIdx])
                                onMessage("Real FPS Game HUD started — tap HUD pill to expand Game Panel")
                            }
                        } else {
                            CoreFlowOverlayService.stop(context)
                            onMessage("Floating Game HUD stopped")
                        }
                    },
                    title = "Floating Real FPS Game HUD",
                    summary = "Draggable HUD with live FPS, Temp, Render % & tap-to-expand Game Panel",
                )
                OverlayDropdownPreference(
                    items = hudIntervalLabels,
                    selectedIndex = hudIntervalIdx,
                    title = "HUD Polling Rate",
                    onSelectedIndexChange = { newIdx ->
                        hudIntervalIdx = newIdx
                        if (overlayRunning) CoreFlowOverlayService.start(context, hudIntervalValues[newIdx])
                    },
                )
                ArrowPreference(
                    title = "Pin Quick Settings Tile (Mode)",
                    summary = "Cycle Balanced, Smoothness, Gaming & Battery from Control Center",
                    onClick = {
                        val sbm = context.getSystemService(StatusBarManager::class.java) ?: return@ArrowPreference
                        sbm.requestAddTileService(
                            ComponentName(context, CoreFlowTileService::class.java),
                            context.getString(R.string.qs_tile_label),
                            AndroidIcon.createWithResource(context, R.drawable.ic_coreflow_tile),
                            context.mainExecutor,
                        ) { res ->
                            onMessage(
                                if (res == StatusBarManager.TILE_ADD_REQUEST_RESULT_TILE_ADDED) {
                                    "Mode Tile added to Control Center"
                                } else {
                                    "Mode Tile already added or cancelled"
                                },
                            )
                        }
                    },
                )
                if (batt.bypassSupported) {
                    ArrowPreference(
                        title = "Pin Quick Settings Tile (Bypass Charging)",
                        summary = "1-tap Bypass Charging ON/OFF right from Control Center",
                        onClick = {
                            val sbm = context.getSystemService(StatusBarManager::class.java) ?: return@ArrowPreference
                            sbm.requestAddTileService(
                                ComponentName(context, CoreFlowBypassTileService::class.java),
                                context.getString(R.string.qs_bypass_tile_label),
                                AndroidIcon.createWithResource(context, R.drawable.ic_bypass_tile),
                                context.mainExecutor,
                            ) { res ->
                                onMessage(
                                    if (res == StatusBarManager.TILE_ADD_REQUEST_RESULT_TILE_ADDED) {
                                        "Bypass Tile added to Control Center"
                                    } else {
                                        "Bypass Tile already added or cancelled"
                                    },
                                )
                            }
                        },
                    )
                }
            }
        }

        item { Spacer(Modifier.height(12.dp)) }
        }
    }
}

@Composable
private fun UsageBar(fraction: Float, color: Color) {
    val clamped = fraction.coerceIn(0f, 1f)
    Box(
        modifier = Modifier
            .fillMaxWidth()
            .height(6.dp)
            .clip(RoundedCornerShape(999.dp))
            .background(MiuixTheme.colorScheme.secondaryContainer.copy(alpha = 0.55f)),
    ) {
        if (clamped > 0f) {
            Box(
                modifier = Modifier
                    .fillMaxWidth(clamped)
                    .fillMaxHeight()
                    .clip(RoundedCornerShape(999.dp))
                    .background(color),
            )
        }
    }
}
