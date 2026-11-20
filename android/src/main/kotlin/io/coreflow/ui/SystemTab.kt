package io.coreflow.ui

import android.content.Context
import android.content.Intent
import android.net.Uri
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.IntrinsicSize
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.nestedscroll.nestedScroll
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import io.coreflow.AppPreferences
import io.coreflow.BuildConfig
import io.coreflow.CoreFlowManager
import top.yukonga.miuix.kmp.basic.ButtonDefaults
import top.yukonga.miuix.kmp.basic.Card
import top.yukonga.miuix.kmp.basic.ScrollBehavior
import top.yukonga.miuix.kmp.basic.SmallTitle
import top.yukonga.miuix.kmp.basic.Text
import top.yukonga.miuix.kmp.basic.TextButton
import top.yukonga.miuix.kmp.preference.ArrowPreference
import top.yukonga.miuix.kmp.preference.OverlayDropdownPreference
import top.yukonga.miuix.kmp.preference.SwitchPreference
import top.yukonga.miuix.kmp.theme.MiuixTheme

private data class SponsorEntry(
    val name: String,
    val role: String,
    val badge: String,
    val accent: Color,
    val url: String,
)

private val AI_AND_PLATFORM_SPONSORS = listOf(
    SponsorEntry(
        name = "Claude",
        role = "Anthropic · Systems & Architecture",
        badge = "CL",
        accent = Color(0xFFD97757),
        url = "https://claude.ai",
    ),
    SponsorEntry(
        name = "ChatGPT",
        role = "OpenAI · Prototyping & Ideation",
        badge = "AI",
        accent = Color(0xFF10A37F),
        url = "https://chatgpt.com",
    ),
    SponsorEntry(
        name = "Gemini",
        role = "Google DeepMind · Engine & Miuix UI",
        badge = "GM",
        accent = Color(0xFF4285F4),
        url = "https://gemini.google.com",
    ),
    SponsorEntry(
        name = "GitHub",
        role = "Actions CI/CD & Open Source",
        badge = "GH",
        accent = Color(0xFF8B5CF6),
        url = "https://github.com/mystivara-creator/CoreFlow-Engine",
    ),
)

@Composable
fun SystemTab(
    status: CoreFlowManager.DaemonStatus?,
    journalText: String?,
    busy: Boolean,
    themeModeIndex: Int,
    pollIntervalIndex: Int,
    scrollBehavior: ScrollBehavior,
    onThemeModeChange: (Int) -> Unit,
    onPollIntervalChange: (Int) -> Unit,
    onToggleFeature: (key: String, label: String, enabled: Boolean) -> Unit,
    onLoadJournal: () -> Unit,
    onShareDiag: () -> Unit,
    onRollback: () -> Unit,
) {
    val context = LocalContext.current
    var confirmRollback by remember { mutableStateOf(false) }
    val features = status?.features ?: CoreFlowManager.EngineFeatures()

    LazyColumn(
        modifier = Modifier
            .fillMaxSize()
            .nestedScroll(scrollBehavior.nestedScrollConnection),
        contentPadding = PaddingValues(horizontal = 12.dp, vertical = 8.dp),
        verticalArrangement = Arrangement.spacedBy(10.dp),
    ) {
        // App Preferences
        item {
            SmallTitle(text = "APP SETTINGS")
        }

        item {
            Card(modifier = Modifier.fillMaxWidth()) {
                OverlayDropdownPreference(
                    items = AppPreferences.themeModeLabels,
                    selectedIndex = themeModeIndex,
                    title = "Appearance & Monet Theme",
                    summary = "Customize Miuix light, dark, or dynamic wallpaper palette",
                    onSelectedIndexChange = onThemeModeChange,
                )
                OverlayDropdownPreference(
                    items = AppPreferences.pollIntervalLabels,
                    selectedIndex = pollIntervalIndex,
                    title = "Telemetry Refresh Rate",
                    summary = "How often live daemon stats update while app is open",
                    onSelectedIndexChange = onPollIntervalChange,
                )
            }
        }

        // Live Engine Pillar Toggles
        item {
            SmallTitle(text = "ENGINE PILLAR CONTROLS")
        }

        item {
            Card(modifier = Modifier.fillMaxWidth()) {
                SwitchPreference(
                    checked = features.frameSmoothness,
                    onCheckedChange = { onToggleFeature("enable_frame_smoothness", "Frame Smoothness", it) },
                    title = "Per-TID Frame Smoothness",
                    summary = "Closed-loop RenderThread & GameThread UClamp boost",
                    enabled = !busy,
                )
                SwitchPreference(
                    checked = features.storageBurst,
                    onCheckedChange = { onToggleFeature("enable_storage_burst", "Storage Launch Burst", it) },
                    title = "Dynamic Storage Launch Burst",
                    summary = "UFS/eMMC read-ahead burst & background I/O isolation",
                    enabled = !busy,
                )
                SwitchPreference(
                    checked = features.memoryGuard,
                    onCheckedChange = { onToggleFeature("enable_memory_guard", "Memory Guard", it) },
                    title = "MGLRU & Smart Pre-Read",
                    summary = "Anti-stutter working-set protection & library warmup",
                    enabled = !busy,
                )
                SwitchPreference(
                    checked = features.thermalPacing,
                    onCheckedChange = { onToggleFeature("enable_thermal_pacing", "Thermal Pacing", it) },
                    title = "Rust PID Thermal Pacing",
                    summary = "Smooth predictive thermal envelope before OEM throttling",
                    enabled = !busy,
                )
                SwitchPreference(
                    checked = features.batteryStepCharging,
                    onCheckedChange = { onToggleFeature("enable_battery_step_charging", "Battery Step Charging", it) },
                    title = "Thermal Step Charging Guard",
                    summary = "Protects battery longevity during hot fast-charging",
                    enabled = !busy,
                )
            }
        }

        // OEM Coexistence & Diagnostics
        item {
            SmallTitle(text = "OEM COEXISTENCE & DIAGNOSTICS")
        }

        item {
            val contended = status?.contendedPaths.orEmpty()
            Card(
                modifier = Modifier.fillMaxWidth(),
                insideMargin = PaddingValues(16.dp),
            ) {
                Text(
                    text = if (contended.isEmpty()) {
                        "Zero OEM Contention · Full Ownership"
                    } else {
                        "${contended.size} Node(s) in Auto-Cooldown Backoff"
                    },
                    style = MiuixTheme.textStyles.headline2,
                    color = if (contended.isEmpty()) Color(0xFF22C55E) else Color(0xFFF59E0B),
                )
                Spacer(Modifier.height(4.dp))
                Text(
                    text = if (contended.isEmpty()) {
                        "CoreFlow v5.1.0 is cleanly cooperating with OEM thermal & storage policies."
                    } else {
                        contended.joinToString("\n")
                    },
                    style = MiuixTheme.textStyles.footnote1,
                    color = MiuixTheme.colorScheme.onSurfaceVariantSummary,
                )
            }
        }

        item {
            Card(modifier = Modifier.fillMaxWidth()) {
                ArrowPreference(
                    title = "Export & Share Diagnostics (--diag)",
                    summary = "Generate coreflow-diag-v5.1.0.json and open Android Share Sheet",
                    enabled = !busy,
                    onClick = onShareDiag,
                )
                ArrowPreference(
                    title = "Inspect CFJ4 Baseline Journal",
                    summary = "${status?.baselineNodesSnapshotted ?: 0} OEM sysfs nodes safely snapshotted",
                    enabled = !busy,
                    onClick = onLoadJournal,
                )
            }
        }

        if (!journalText.isNullOrBlank()) {
            item {
                Card(
                    modifier = Modifier.fillMaxWidth(),
                    insideMargin = PaddingValues(14.dp),
                ) {
                    Text(
                        text = journalText.take(2400),
                        fontFamily = FontFamily.Monospace,
                        fontSize = 11.sp,
                        color = MiuixTheme.colorScheme.onSurfaceVariantSummary,
                    )
                }
            }
        }

        // Emergency Recovery
        item {
            SmallTitle(text = "RECOVERY")
        }

        item {
            Card(
                modifier = Modifier.fillMaxWidth(),
                insideMargin = PaddingValues(16.dp),
            ) {
                Text(
                    text = "CFJ4 Zero-Trace OEM Rollback",
                    style = MiuixTheme.textStyles.headline2,
                )
                Spacer(Modifier.height(4.dp))
                Text(
                    text = "Stops CoreFlow actuation and restores every modified sysfs node to its exact factory OEM value.",
                    style = MiuixTheme.textStyles.body2,
                    color = MiuixTheme.colorScheme.onSurfaceVariantSummary,
                )
                Spacer(Modifier.height(12.dp))
                if (!confirmRollback) {
                    TextButton(
                        text = "Restore OEM Baseline",
                        onClick = { confirmRollback = true },
                        enabled = !busy,
                        modifier = Modifier.fillMaxWidth(),
                    )
                } else {
                    TextButton(
                        text = "Confirm Rollback Now",
                        onClick = {
                            confirmRollback = false
                            onRollback()
                        },
                        enabled = !busy,
                        colors = ButtonDefaults.textButtonColorsPrimary(),
                        modifier = Modifier.fillMaxWidth(),
                    )
                }
            }
        }

        // Creator Identity & GitHub Profile
        item {
            SmallTitle(text = "CREATOR & MAINTAINER")
        }

        item {
            Card(modifier = Modifier.fillMaxWidth()) {
                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(horizontal = 16.dp, vertical = 14.dp),
                    verticalAlignment = Alignment.CenterVertically,
                    horizontalArrangement = Arrangement.spacedBy(12.dp),
                ) {
                    Box(
                        modifier = Modifier
                            .size(46.dp)
                            .clip(CircleShape)
                            .background(MiuixTheme.colorScheme.primary.copy(alpha = 0.16f)),
                        contentAlignment = Alignment.Center,
                    ) {
                        Text(
                            text = "MC",
                            style = MiuixTheme.textStyles.headline1,
                            fontWeight = FontWeight.Bold,
                            color = MiuixTheme.colorScheme.primary,
                        )
                    }
                    Column(modifier = Modifier.weight(1f)) {
                        Text(
                            text = "Mystivara Creator",
                            style = MiuixTheme.textStyles.headline1,
                            fontWeight = FontWeight.Bold,
                        )
                        Spacer(Modifier.height(2.dp))
                        Text(
                            text = "@mystivara-creator · Author of CoreFlow Engine & CFAE Framework",
                            style = MiuixTheme.textStyles.footnote1,
                            color = MiuixTheme.colorScheme.onSurfaceVariantSummary,
                        )
                    }
                }
                ArrowPreference(
                    title = "GitHub Profile",
                    summary = "github.com/mystivara-creator",
                    onClick = { openUrl(context, "https://github.com/mystivara-creator") },
                )
                ArrowPreference(
                    title = "CoreFlow-Engine Repository",
                    summary = "Source code, issues & releases (v${BuildConfig.VERSION_NAME})",
                    onClick = { openUrl(context, "https://github.com/mystivara-creator/CoreFlow-Engine") },
                )
            }
        }

        // AI Partners & Platform Sponsors
        item {
            SmallTitle(text = "POWERED BY · AI & PLATFORM SPONSORS")
        }

        item {
            Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
                val rows = AI_AND_PLATFORM_SPONSORS.chunked(2)
                for (pair in rows) {
                    Row(
                        modifier = Modifier
                            .fillMaxWidth()
                            .height(IntrinsicSize.Min),
                        horizontalArrangement = Arrangement.spacedBy(8.dp),
                    ) {
                        for (sponsor in pair) {
                            Card(
                                modifier = Modifier
                                    .weight(1f)
                                    .fillMaxHeight(),
                                insideMargin = PaddingValues(14.dp),
                                onClick = { openUrl(context, sponsor.url) },
                            ) {
                                Row(
                                    verticalAlignment = Alignment.CenterVertically,
                                    horizontalArrangement = Arrangement.spacedBy(10.dp),
                                ) {
                                    Box(
                                        modifier = Modifier
                                            .size(32.dp)
                                            .clip(RoundedCornerShape(10.dp))
                                            .background(sponsor.accent.copy(alpha = 0.18f)),
                                        contentAlignment = Alignment.Center,
                                    ) {
                                        Text(
                                            text = sponsor.badge,
                                            fontSize = 11.sp,
                                            fontWeight = FontWeight.Bold,
                                            color = sponsor.accent,
                                        )
                                    }
                                    Text(
                                        text = sponsor.name,
                                        style = MiuixTheme.textStyles.headline2,
                                        fontWeight = FontWeight.SemiBold,
                                        maxLines = 1,
                                        overflow = TextOverflow.Ellipsis,
                                    )
                                }
                                Spacer(Modifier.height(6.dp))
                                Text(
                                    text = sponsor.role,
                                    style = MiuixTheme.textStyles.footnote1,
                                    color = MiuixTheme.colorScheme.onSurfaceVariantSummary,
                                    maxLines = 2,
                                    overflow = TextOverflow.Ellipsis,
                                )
                            }
                        }
                    }
                }
            }
        }

        item {
            Card(
                modifier = Modifier.fillMaxWidth(),
                insideMargin = PaddingValues(14.dp),
            ) {
                Text(
                    text = "CoreFlow Companion v${BuildConfig.VERSION_NAME} · Apache-2.0 License",
                    style = MiuixTheme.textStyles.footnote1,
                    color = MiuixTheme.colorScheme.onSurfaceVariantSummary,
                )
            }
        }

        item { Spacer(Modifier.height(12.dp)) }
    }
}

private fun openUrl(context: Context, url: String) {
    runCatching {
        val intent = Intent(Intent.ACTION_VIEW, Uri.parse(url))
            .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
        context.startActivity(intent)
    }
}
