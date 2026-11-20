package io.coreflow.ui

import android.app.StatusBarManager
import android.content.ComponentName
import android.content.Intent
import android.graphics.drawable.Icon as AndroidIcon
import android.net.Uri
import android.provider.Settings
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.input.nestedscroll.nestedScroll
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import io.coreflow.CoreFlowManager
import io.coreflow.CoreFlowOverlayService
import io.coreflow.CoreFlowTileService
import io.coreflow.R
import top.yukonga.miuix.kmp.basic.Card
import top.yukonga.miuix.kmp.basic.ScrollBehavior
import top.yukonga.miuix.kmp.basic.SmallTitle
import top.yukonga.miuix.kmp.basic.Text
import top.yukonga.miuix.kmp.preference.ArrowPreference
import top.yukonga.miuix.kmp.preference.OverlayDropdownPreference
import top.yukonga.miuix.kmp.preference.SwitchPreference
import top.yukonga.miuix.kmp.theme.MiuixTheme

@Composable
fun HudTab(
    status: CoreFlowManager.DaemonStatus?,
    scrollBehavior: ScrollBehavior,
    onMessage: (String) -> Unit,
) {
    val context = LocalContext.current
    val overlayRunning by CoreFlowOverlayService.running.collectAsState()
    var canOverlay by remember { mutableStateOf(Settings.canDrawOverlays(context)) }
    var intervalIdx by rememberSaveable { mutableIntStateOf(1) }

    val intervalLabels = remember { listOf("Fast (1.0s)", "Balanced (1.5s)", "Battery (2.5s)") }
    val intervalValues = remember { listOf(1000L, 1500L, 2500L) }

    LazyColumn(
        modifier = Modifier
            .fillMaxSize()
            .nestedScroll(scrollBehavior.nestedScrollConnection),
        contentPadding = PaddingValues(horizontal = 12.dp, vertical = 8.dp),
        verticalArrangement = Arrangement.spacedBy(10.dp),
    ) {
        item {
            SmallTitle(text = "FLOATING GAME HUD (NATIVE ONLY)")
        }

        item {
            Card(modifier = Modifier.fillMaxWidth()) {
                SwitchPreference(
                    checked = overlayRunning,
                    onCheckedChange = { enable ->
                        canOverlay = Settings.canDrawOverlays(context)
                        if (enable) {
                            if (!canOverlay) {
                                val intent = Intent(
                                    Settings.ACTION_MANAGE_OVERLAY_PERMISSION,
                                    Uri.parse("package:${context.packageName}"),
                                ).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                                context.startActivity(intent)
                                onMessage("Enable 'Display over other apps' then toggle HUD on")
                            } else {
                                CoreFlowOverlayService.start(context, intervalValues[intervalIdx])
                                onMessage("Floating Game HUD started")
                            }
                        } else {
                            CoreFlowOverlayService.stop(context)
                            onMessage("Floating Game HUD stopped")
                        }
                    },
                    title = "Floating Live HUD",
                    summary = "Draggable overlay with Temp, Render Pressure, UClamp & 1-tap A/B Pause over games",
                )

                OverlayDropdownPreference(
                    items = intervalLabels,
                    selectedIndex = intervalIdx,
                    title = "HUD Polling Rate",
                    summary = "Controls how often the floating HUD refreshes telemetry",
                    onSelectedIndexChange = { newIdx ->
                        intervalIdx = newIdx
                        if (overlayRunning) {
                            CoreFlowOverlayService.start(context, intervalValues[newIdx])
                        }
                    },
                )

                ArrowPreference(
                    title = "Overlay Permission",
                    summary = if (Settings.canDrawOverlays(context)) "Granted" else "Tap to grant Display Over Other Apps",
                    onClick = {
                        val intent = Intent(
                            Settings.ACTION_MANAGE_OVERLAY_PERMISSION,
                            Uri.parse("package:${context.packageName}"),
                        ).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                        context.startActivity(intent)
                    },
                )
            }
        }

        item {
            SmallTitle(text = "ANDROID / HYPEROS CONTROL CENTER")
        }

        item {
            Card(modifier = Modifier.fillMaxWidth()) {
                ArrowPreference(
                    title = "Pin Quick Settings Tile",
                    summary = "Add 'CoreFlow Mode' directly to your Control Center to cycle all 4 profiles",
                    onClick = {
                        val sbm = context.getSystemService(StatusBarManager::class.java)
                        if (sbm == null) {
                            onMessage("StatusBarManager unavailable on this ROM")
                            return@ArrowPreference
                        }
                        sbm.requestAddTileService(
                            ComponentName(context, CoreFlowTileService::class.java),
                            context.getString(R.string.qs_tile_label),
                            AndroidIcon.createWithResource(context, R.drawable.ic_coreflow_tile),
                            context.mainExecutor,
                        ) { result ->
                            val msg = when (result) {
                                StatusBarManager.TILE_ADD_REQUEST_RESULT_TILE_ADDED -> "CoreFlow Tile added to Quick Settings"
                                StatusBarManager.TILE_ADD_REQUEST_RESULT_TILE_ALREADY_ADDED -> "CoreFlow Tile is already in Quick Settings"
                                else -> "Tile add cancelled"
                            }
                            onMessage(msg)
                        }
                    },
                )
            }
        }

        item {
            SmallTitle(text = "DISCOVERED HARDWARE TOPOLOGY")
        }

        item {
            val hw = status?.hardware
            Card(
                modifier = Modifier.fillMaxWidth(),
                insideMargin = PaddingValues(16.dp),
            ) {
                HardwareLine("SoC", hw?.socModel ?: "--")
                HardwareLine("GPU", hw?.gpuModel?.ifBlank { "Auto / Devfreq" } ?: "--")
                HardwareLine("Storage", hw?.storageType ?: "--")
                HardwareLine("Display", hw?.let { "${it.refreshHz} Hz" } ?: "--")
                HardwareLine("EAS Clusters", hw?.let { "${it.cpuClusters} clusters" } ?: "--")
                HardwareLine("Linux MGLRU", if (hw?.mglruAvailable == true) "Available" else "Not exposed")
                HardwareLine("Cgroup v2", if (hw?.cgroupV2Available == true) "Available" else "Fallback")
                HardwareLine("Kernel", hw?.kernelVersion?.ifBlank { "--" } ?: "--")
            }
        }

        item { Spacer(Modifier.height(12.dp)) }
    }
}

@Composable
private fun HardwareLine(label: String, value: String) {
    Text(
        text = "$label: $value",
        style = MiuixTheme.textStyles.body2,
        color = MiuixTheme.colorScheme.onSurface,
    )
    Spacer(Modifier.height(4.dp))
}
