package io.coreflow.ui

import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.background
import androidx.compose.foundation.border
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
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.nestedscroll.nestedScroll
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
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
                status == null -> "Connecting..."
                !isRunning -> "Engine Stopped"
                isPaused -> "Paused · OEM Mode"
                status.thermalGuardActive -> "Thermal Guard"
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
                    Row(
                        modifier = Modifier.weight(1f),
                        verticalAlignment = Alignment.CenterVertically,
                        horizontalArrangement = Arrangement.spacedBy(8.dp),
                    ) {
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
                            maxLines = 1,
                            overflow = TextOverflow.Ellipsis,
                        )
                    }
                    Box(
                        modifier = Modifier
                            .clip(RoundedCornerShape(999.dp))
                            .background(MiuixTheme.colorScheme.primary.copy(alpha = 0.14f))
                            .padding(horizontal = 10.dp, vertical = 4.dp),
                    ) {
                        Text(
                            text = status?.let { "${it.activePillars}/4 Pillars" } ?: "--/4",
                            fontSize = 11.sp,
                            color = MiuixTheme.colorScheme.primary,
                            fontWeight = FontWeight.SemiBold,
                            maxLines = 1,
                        )
                    }
                }

                Spacer(Modifier.height(6.dp))
                Text(
                    text = status?.foregroundPackage?.takeIf { it.isNotBlank() }?.let { "Foreground: $it" }
                        ?: status?.hardware?.socModel
                        ?: "Waiting for CoreFlow Engine v5.1.0 telemetry",
                    style = MiuixTheme.textStyles.footnote1,
                    color = MiuixTheme.colorScheme.onSurfaceVariantSummary,
                    maxLines = 1,
                    overflow = TextOverflow.Ellipsis,
                )

                // Uniform 4-Metric Row (strictly aligned equal heights & single-line labels)
                Spacer(Modifier.height(14.dp))
                val tempColor = when {
                    (status?.socTempCelsius ?: 0f) >= 55f -> Color(0xFFEF4444)
                    (status?.socTempCelsius ?: 0f) >= 46f -> Color(0xFFF59E0B)
                    else -> Color(0xFF22C55E)
                }
                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .height(IntrinsicSize.Min),
                    horizontalArrangement = Arrangement.spacedBy(8.dp),
                ) {
                    MetricPill(
                        label = "SoC Temp",
                        value = status?.socTempCelsius?.takeIf { it >= 0f }
                            ?.let { String.format(Locale.US, "%.1f°C", it) } ?: "--",
                        accent = tempColor,
                        modifier = Modifier
                            .weight(1f)
                            .fillMaxHeight(),
                    )
                    MetricPill(
                        label = "Render",
                        value = status?.let { "${(it.renderPressure * 100f).toInt().coerceIn(0, 100)}%" } ?: "--",
                        accent = Color(0xFF38BDF8),
                        modifier = Modifier
                            .weight(1f)
                            .fillMaxHeight(),
                    )
                    MetricPill(
                        label = "UClamp",
                        value = status?.let { "${it.uclampMin}" } ?: "--",
                        accent = Color(0xFFA78BFA),
                        modifier = Modifier
                            .weight(1f)
                            .fillMaxHeight(),
                    )
                    MetricPill(
                        label = "Headroom",
                        value = status?.let { "${(it.capacityScale * 100f).toInt().coerceIn(0, 100)}%" } ?: "--",
                        accent = Color(0xFF34D399),
                        modifier = Modifier
                            .weight(1f)
                            .fillMaxHeight(),
                    )
                }

                // 4-Pillar Compact Strip
                Spacer(Modifier.height(10.dp))
                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .height(IntrinsicSize.Min),
                    horizontalArrangement = Arrangement.spacedBy(6.dp),
                ) {
                    PillarChip("UClamp", status?.uclampSupported == true, Modifier.weight(1f).fillMaxHeight())
                    PillarChip(
                        "Storage",
                        (status?.readAheadKb ?: 0) > 0 || status?.storageBurstActive == true,
                        Modifier.weight(1f).fillMaxHeight(),
                    )
                    PillarChip(
                        "MGLRU",
                        status?.hardware?.mglruAvailable == true || (status?.prereadBytes ?: 0L) > 0L,
                        Modifier.weight(1f).fillMaxHeight(),
                    )
                    PillarChip(
                        "Thermal",
                        (status?.managedEnvelopeNodes ?: 0) > 0 || !status?.thermalSource.isNullOrBlank(),
                        Modifier.weight(1f).fillMaxHeight(),
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

        // 4-Profile Selector (Strictly Equal-Height 2x2 Cards)
        item {
            SmallTitle(text = "AUTONOMOUS PROFILE")
        }

        item {
            val activeProfile = status?.profile
            Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
                val pairs = CoreFlowManager.Profile.primaryProfiles.chunked(2)
                for (rowProfiles in pairs) {
                    Row(
                        modifier = Modifier
                            .fillMaxWidth()
                            .height(IntrinsicSize.Min),
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
                            val borderMod = if (selected) {
                                Modifier.border(
                                    BorderStroke(1.dp, MiuixTheme.colorScheme.primary.copy(alpha = 0.45f)),
                                    RoundedCornerShape(16.dp),
                                )
                            } else {
                                Modifier
                            }
                            Card(
                                modifier = Modifier
                                    .weight(1f)
                                    .fillMaxHeight()
                                    .then(borderMod),
                                insideMargin = PaddingValues(14.dp),
                                colors = cardColors,
                                onClick = { if (!busy) onSelectProfile(profile) },
                            ) {
                                Row(
                                    modifier = Modifier.fillMaxWidth(),
                                    verticalAlignment = Alignment.CenterVertically,
                                    horizontalArrangement = Arrangement.SpaceBetween,
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
                                        maxLines = 1,
                                        overflow = TextOverflow.Ellipsis,
                                    )
                                    if (selected) {
                                        Box(
                                            modifier = Modifier
                                                .size(8.dp)
                                                .clip(CircleShape)
                                                .background(MiuixTheme.colorScheme.primary),
                                        )
                                    }
                                }
                                Spacer(Modifier.height(4.dp))
                                Text(
                                    text = profile.tagline,
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

        // Session Impact Card (Strictly Equal-Height 3x2 Pills)
        item {
            SmallTitle(text = "SESSION IMPACT")
        }

        item {
            val session = status?.session
            val prereadMb = (status?.prereadBytes ?: 0L) / (1024.0 * 1024.0)
            Card(
                modifier = Modifier.fillMaxWidth(),
                insideMargin = PaddingValues(14.dp),
            ) {
                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .height(IntrinsicSize.Min),
                    horizontalArrangement = Arrangement.spacedBy(8.dp),
                ) {
                    MetricPill(
                        label = "Uptime",
                        value = formatUptime(session?.uptimeSec ?: 0L),
                        modifier = Modifier
                            .weight(1f)
                            .fillMaxHeight(),
                    )
                    MetricPill(
                        label = "Bursts",
                        value = "${session?.launchBursts ?: 0}",
                        modifier = Modifier
                            .weight(1f)
                            .fillMaxHeight(),
                    )
                    MetricPill(
                        label = "Pre-Read",
                        value = String.format(Locale.US, "%.1f MB", prereadMb),
                        modifier = Modifier
                            .weight(1f)
                            .fillMaxHeight(),
                    )
                }
                Spacer(Modifier.height(8.dp))
                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .height(IntrinsicSize.Min),
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
                        modifier = Modifier
                            .weight(1f)
                            .fillMaxHeight(),
                    )
                    MetricPill(
                        label = "Guards",
                        value = "${session?.thermalGuardEvents ?: 0}",
                        modifier = Modifier
                            .weight(1f)
                            .fillMaxHeight(),
                    )
                    MetricPill(
                        label = "CFJ4 Nodes",
                        value = "${status?.baselineNodesSnapshotted ?: 0}",
                        modifier = Modifier
                            .weight(1f)
                            .fillMaxHeight(),
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
    accent: Color? = null,
) {
    Column(
        modifier = modifier
            .clip(RoundedCornerShape(12.dp))
            .background(MiuixTheme.colorScheme.secondaryContainer.copy(alpha = 0.45f))
            .padding(horizontal = 10.dp, vertical = 9.dp),
        verticalArrangement = Arrangement.SpaceBetween,
    ) {
        Row(
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.spacedBy(5.dp),
        ) {
            if (accent != null) {
                Box(
                    modifier = Modifier
                        .size(6.dp)
                        .clip(CircleShape)
                        .background(accent),
                )
            }
            Text(
                text = label,
                fontSize = 10.sp,
                color = MiuixTheme.colorScheme.onSurfaceVariantSummary,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis,
            )
        }
        Spacer(Modifier.height(4.dp))
        Text(
            text = value,
            style = MiuixTheme.textStyles.body2,
            fontWeight = FontWeight.Bold,
            color = MiuixTheme.colorScheme.onSurface,
            maxLines = 1,
            overflow = TextOverflow.Ellipsis,
        )
    }
}

@Composable
private fun PillarChip(
    label: String,
    active: Boolean,
    modifier: Modifier = Modifier,
) {
    val dotColor = if (active) Color(0xFF22C55E) else Color(0xFF64748B)
    Row(
        modifier = modifier
            .clip(RoundedCornerShape(10.dp))
            .background(MiuixTheme.colorScheme.secondaryContainer.copy(alpha = 0.30f))
            .padding(horizontal = 8.dp, vertical = 6.dp),
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.Center,
    ) {
        Box(
            modifier = Modifier
                .size(6.dp)
                .clip(CircleShape)
                .background(dotColor),
        )
        Spacer(Modifier.size(5.dp))
        Text(
            text = label,
            fontSize = 10.sp,
            fontWeight = FontWeight.Medium,
            color = MiuixTheme.colorScheme.onSurface,
            maxLines = 1,
            overflow = TextOverflow.Ellipsis,
        )
    }
}

private fun formatUptime(sec: Long): String {
    if (sec <= 0L) return "0m"
    val hours = sec / 3600L
    val mins = (sec % 3600L) / 60L
    return if (hours > 0L) "${hours}h ${mins}m" else "${mins}m"
}
