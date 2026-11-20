package io.coreflow.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.nestedscroll.nestedScroll
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import io.coreflow.CoreFlowManager
import top.yukonga.miuix.kmp.basic.ButtonDefaults
import top.yukonga.miuix.kmp.basic.Card
import top.yukonga.miuix.kmp.basic.CardDefaults
import top.yukonga.miuix.kmp.basic.ScrollBehavior
import top.yukonga.miuix.kmp.basic.SmallTitle
import top.yukonga.miuix.kmp.basic.Text
import top.yukonga.miuix.kmp.basic.TextButton
import top.yukonga.miuix.kmp.theme.MiuixTheme
import java.util.Locale

@Composable
fun HomeTab(
    hasRoot: Boolean?,
    status: CoreFlowManager.DaemonStatus?,
    lastError: String?,
    busy: Boolean,
    scrollBehavior: ScrollBehavior,
    onSelectProfile: (CoreFlowManager.Profile) -> Unit,
    onTogglePause: (Boolean) -> Unit,
    onStartDaemon: () -> Unit,
) {
    LazyColumn(
        modifier = Modifier
            .fillMaxSize()
            .nestedScroll(scrollBehavior.nestedScrollConnection),
        contentPadding = PaddingValues(horizontal = 12.dp, vertical = 8.dp),
        verticalArrangement = Arrangement.spacedBy(10.dp),
    ) {
        if (hasRoot == false || lastError != null) {
            item {
                Card(
                    modifier = Modifier.fillMaxWidth(),
                    insideMargin = PaddingValues(16.dp),
                ) {
                    Text(
                        text = if (hasRoot == false) "Root Permission Needed" else "Daemon Connection Notice",
                        style = MiuixTheme.textStyles.headline1,
                        color = Color(0xFFEF4444),
                    )
                    Spacer(Modifier.height(4.dp))
                    Text(
                        text = lastError ?: "Grant root access to CoreFlow Companion in Magisk, KernelSU, or APatch.",
                        style = MiuixTheme.textStyles.body2,
                        color = MiuixTheme.colorScheme.onSurfaceVariantSummary,
                    )
                }
            }
        }

        // Hero Status Card
        item {
            val isRunning = status?.isRunning == true
            val isPaused = status?.inspectionMode == true
            val dotColor = when {
                !isRunning -> Color(0xFFEF4444)
                isPaused -> Color(0xFFF59E0B)
                status?.thermalGuardActive == true -> Color(0xFFF97316)
                else -> Color(0xFF22C55E)
            }
            val headline = when {
                status == null -> "Connecting to coreflowd..."
                !isRunning -> "Engine Stopped"
                isPaused -> "Paused · OEM A/B Mode"
                status.thermalGuardActive -> "Thermal Guard Active"
                else -> "${status.effectiveProfile.label} · ${status.mode}"
            }

            Card(
                modifier = Modifier.fillMaxWidth(),
                insideMargin = PaddingValues(16.dp),
            ) {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    verticalAlignment = Alignment.CenterVertically,
                    horizontalArrangement = Arrangement.SpaceBetween,
                ) {
                    Row(verticalAlignment = Alignment.CenterVertically, horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                        Box(
                            modifier = Modifier
                                .size(10.dp)
                                .clip(CircleShape)
                                .background(dotColor),
                        )
                        Text(
                            text = headline,
                            style = MiuixTheme.textStyles.headline1,
                            fontWeight = FontWeight.SemiBold,
                        )
                    }
                    Text(
                        text = status?.let { "${it.activePillars}/4 Pillars" } ?: "--/4",
                        style = MiuixTheme.textStyles.footnote1,
                        color = MiuixTheme.colorScheme.primary,
                        fontWeight = FontWeight.Medium,
                    )
                }

                Spacer(Modifier.height(6.dp))
                Text(
                    text = status?.foregroundPackage?.takeIf { it.isNotBlank() }?.let { "Foreground: $it" }
                        ?: status?.hardware?.socModel
                        ?: "Waiting for CoreFlow Engine v5.1.0 telemetry",
                    style = MiuixTheme.textStyles.body2,
                    color = MiuixTheme.colorScheme.onSurfaceVariantSummary,
                )

                Spacer(Modifier.height(14.dp))
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(8.dp),
                ) {
                    MetricPill(
                        label = "SoC Temp",
                        value = status?.socTempCelsius?.takeIf { it >= 0f }
                            ?.let { String.format(Locale.US, "%.1f°C", it) } ?: "--",
                        modifier = Modifier.weight(1f),
                    )
                    MetricPill(
                        label = "Render Load",
                        value = status?.let { "${(it.renderPressure * 100f).toInt().coerceIn(0, 100)}%" } ?: "--",
                        modifier = Modifier.weight(1f),
                    )
                    MetricPill(
                        label = "UClamp Min",
                        value = status?.let { "${it.uclampMin}" } ?: "--",
                        modifier = Modifier.weight(1f),
                    )
                    MetricPill(
                        label = "Headroom",
                        value = status?.let { "${(it.capacityScale * 100f).toInt().coerceIn(0, 100)}%" } ?: "--",
                        modifier = Modifier.weight(1f),
                    )
                }

                Spacer(Modifier.height(14.dp))
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(10.dp),
                ) {
                    if (isRunning) {
                        TextButton(
                            text = if (isPaused) "Resume CoreFlow" else "Pause (A/B Compare)",
                            onClick = { onTogglePause(!isPaused) },
                            enabled = !busy,
                            colors = if (isPaused) {
                                ButtonDefaults.textButtonColorsPrimary()
                            } else {
                                ButtonDefaults.textButtonColors()
                            },
                            modifier = Modifier.weight(1f),
                        )
                    } else {
                        TextButton(
                            text = "Start CoreFlow Engine",
                            onClick = onStartDaemon,
                            enabled = !busy && hasRoot == true,
                            colors = ButtonDefaults.textButtonColorsPrimary(),
                            modifier = Modifier.weight(1f),
                        )
                    }
                }
            }
        }

        // 4-Profile Selector
        item {
            SmallTitle(text = "AUTONOMOUS PROFILE")
        }

        item {
            val activeProfile = status?.profile
            Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
                val pairs = CoreFlowManager.Profile.primaryProfiles.chunked(2)
                for (rowProfiles in pairs) {
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.spacedBy(8.dp),
                    ) {
                        for (profile in rowProfiles) {
                            val selected = activeProfile == profile
                            val cardColors = if (selected) {
                                CardDefaults.defaultColors(
                                    color = MiuixTheme.colorScheme.primary.copy(alpha = 0.14f),
                                )
                            } else {
                                CardDefaults.defaultColors()
                            }
                            Card(
                                modifier = Modifier.weight(1f),
                                insideMargin = PaddingValues(14.dp),
                                colors = cardColors,
                                onClick = { if (!busy) onSelectProfile(profile) },
                            ) {
                                Text(
                                    text = profile.label,
                                    style = MiuixTheme.textStyles.headline2,
                                    fontWeight = if (selected) FontWeight.Bold else FontWeight.Medium,
                                    color = if (selected) {
                                        MiuixTheme.colorScheme.primary
                                    } else {
                                        MiuixTheme.colorScheme.onSurface
                                    },
                                )
                                Spacer(Modifier.height(3.dp))
                                Text(
                                    text = profile.tagline,
                                    style = MiuixTheme.textStyles.footnote1,
                                    color = MiuixTheme.colorScheme.onSurfaceVariantSummary,
                                )
                            }
                        }
                    }
                }
            }
        }

        // Session Impact Card
        item {
            SmallTitle(text = "SESSION IMPACT")
        }

        item {
            val session = status?.session
            val prereadMb = (status?.prereadBytes ?: 0L) / (1024.0 * 1024.0)
            Card(
                modifier = Modifier.fillMaxWidth(),
                insideMargin = PaddingValues(16.dp),
            ) {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(8.dp),
                ) {
                    MetricPill(
                        label = "Uptime",
                        value = formatUptime(session?.uptimeSec ?: 0L),
                        modifier = Modifier.weight(1f),
                    )
                    MetricPill(
                        label = "Launch Bursts",
                        value = "${session?.launchBursts ?: 0}",
                        modifier = Modifier.weight(1f),
                    )
                    MetricPill(
                        label = "Pre-Read",
                        value = String.format(Locale.US, "%.1f MB", prereadMb),
                        modifier = Modifier.weight(1f),
                    )
                }
                Spacer(Modifier.height(8.dp))
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(8.dp),
                ) {
                    MetricPill(
                        label = "Avg / Peak",
                        value = if (session != null && session.avgSocTempCelsius >= 0f) {
                            String.format(
                                Locale.US,
                                "%.0f/%.0f°C",
                                session.avgSocTempCelsius,
                                session.peakSocTempCelsius.coerceAtLeast(session.avgSocTempCelsius),
                            )
                        } else {
                            "--"
                        },
                        modifier = Modifier.weight(1f),
                    )
                    MetricPill(
                        label = "Thermal Guard",
                        value = "${session?.thermalGuardEvents ?: 0}",
                        modifier = Modifier.weight(1f),
                    )
                    MetricPill(
                        label = "CFJ4 Nodes",
                        value = "${status?.baselineNodesSnapshotted ?: 0}",
                        modifier = Modifier.weight(1f),
                    )
                }
            }
        }

        item { Spacer(Modifier.height(12.dp)) }
    }
}

@Composable
private fun MetricPill(
    label: String,
    value: String,
    modifier: Modifier = Modifier,
) {
    Column(
        modifier = modifier
            .clip(RoundedCornerShape(12.dp))
            .background(MiuixTheme.colorScheme.secondaryContainer.copy(alpha = 0.45f))
            .padding(horizontal = 10.dp, vertical = 8.dp),
    ) {
        Text(
            text = label,
            fontSize = 10.sp,
            color = MiuixTheme.colorScheme.onSurfaceVariantSummary,
        )
        Spacer(Modifier.height(2.dp))
        Text(
            text = value,
            style = MiuixTheme.textStyles.body2,
            fontWeight = FontWeight.SemiBold,
            color = MiuixTheme.colorScheme.onSurface,
        )
    }
}

private fun formatUptime(sec: Long): String {
    if (sec <= 0L) return "0m"
    val hours = sec / 3600L
    val mins = (sec % 3600L) / 60L
    return if (hours > 0L) "${hours}h ${mins}m" else "${mins}m"
}
