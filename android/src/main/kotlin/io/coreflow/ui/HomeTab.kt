package io.coreflow.ui

import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.Canvas
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
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.Stroke
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
    strings: AppStrings,
    hasRoot: Boolean?,
    status: CoreFlowManager.DaemonStatus?,
    lastError: String?,
    busy: Boolean,
    scrollBehavior: ScrollBehavior,
    onSelectProfile: (CoreFlowManager.Profile) -> Unit,
    onTogglePause: (Boolean) -> Unit,
    onToggleBypass: (Boolean) -> Unit,
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
                status == null -> if (strings.isIndonesian) "Menghubungkan..." else "Connecting..."
                !isRunning -> if (strings.isIndonesian) "Engine Berhenti" else "Engine Stopped"
                isPaused -> if (strings.isIndonesian) "Dijeda · Mode OEM" else "Paused · OEM Mode"
                status.thermalGuardActive -> strings.humanizeMode("THERMAL_GUARD")
                else -> "${status.effectiveProfile.label} · ${strings.humanizeMode(status.mode)}"
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
                        ?: "Waiting for CoreFlow v6.0.0 telemetry",
                    style = MiuixTheme.textStyles.footnote1,
                    color = MiuixTheme.colorScheme.onSurfaceVariantSummary,
                    maxLines = 1,
                    overflow = TextOverflow.Ellipsis,
                )

                // Uniform 4-Metric Row
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

                // Action Row: Pause A/B + Capability-Driven Bypass Quick Toggle
                Spacer(Modifier.height(14.dp))
                val bypassSupported = status?.batteryPower?.bypassSupported == true
                val bypassActive = status?.batteryPower?.bypassActive == true
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(8.dp),
                ) {
                    if (isRunning) {
                        TextButton(
                            text = if (isPaused) strings.resumeEngine else strings.pauseCompare,
                            onClick = { onTogglePause(!isPaused) },
                            enabled = !busy,
                            colors = if (isPaused) {
                                ButtonDefaults.textButtonColorsPrimary()
                            } else {
                                ButtonDefaults.textButtonColors()
                            },
                            modifier = Modifier.weight(1f),
                        )
                        if (bypassSupported) {
                            TextButton(
                                text = if (bypassActive) strings.bypassOn else strings.bypassOff,
                                onClick = { onToggleBypass(!(status?.powerConfig?.bypassCharging == true)) },
                                enabled = !busy,
                                colors = if (bypassActive) {
                                    ButtonDefaults.textButtonColorsPrimary()
                                } else {
                                    ButtonDefaults.textButtonColors()
                                },
                                modifier = Modifier.weight(0.75f),
                            )
                        }
                    } else {
                        TextButton(
                            text = strings.startEngine,
                            onClick = onStartDaemon,
                            enabled = !busy && hasRoot == true,
                            colors = ButtonDefaults.textButtonColorsPrimary(),
                            modifier = Modifier.weight(1f),
                        )
                    }
                }
            }
        }

        // Live Telemetry Sparkline Card (shown once history has >= 2 samples)
        val history = status?.history.orEmpty()
        if (history.size >= 2) {
            item {
                SmallTitle(text = strings.sectionSparkline)
            }
            item {
                LiveSparklineCard(
                    history = history,
                    trendLabel = strings.formatTrend(status?.thermalVelocityCpm ?: 0f),
                )
            }
        }

        // 4-Profile Selector (Strictly Equal-Height 2x2 Cards)
        item {
            SmallTitle(text = strings.sectionProfile)
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

        // Session Impact Card
        item {
            SmallTitle(text = strings.sectionSession)
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
private fun LiveSparklineCard(
    history: List<CoreFlowManager.HistorySample>,
    trendLabel: String,
) {
    val temps = history.mapNotNull { it.temp?.takeIf { t -> t > 0f } }
    val scales = history.map { it.scale.coerceIn(0.25f, 1f) }
    val latestTemp = temps.lastOrNull()
    val latestScale = scales.lastOrNull() ?: 1f

    Card(
        modifier = Modifier.fillMaxWidth(),
        insideMargin = PaddingValues(14.dp),
    ) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically,
        ) {
            Row(verticalAlignment = Alignment.CenterVertically, horizontalArrangement = Arrangement.spacedBy(10.dp)) {
                LegendDot(
                    color = Color(0xFFF59E0B),
                    text = latestTemp?.let { String.format(Locale.US, "SoC %.1f°C", it) } ?: "SoC --",
                )
                LegendDot(
                    color = Color(0xFF38BDF8),
                    text = "Headroom ${(latestScale * 100f).toInt()}%",
                )
            }
            Text(
                text = "$trendLabel · ${history.size}s",
                fontSize = 10.sp,
                fontWeight = FontWeight.Medium,
                color = MiuixTheme.colorScheme.onSurfaceVariantSummary,
                maxLines = 1,
            )
        }
        Spacer(Modifier.height(10.dp))
        Canvas(
            modifier = Modifier
                .fillMaxWidth()
                .height(64.dp),
        ) {
            val w = size.width
            val h = size.height
            if (w <= 0f || h <= 0f) return@Canvas

            // Subtle horizontal guide lines
            val guideColor = Color.Gray.copy(alpha = 0.16f)
            drawLine(guideColor, Offset(0f, h * 0.25f), Offset(w, h * 0.25f), strokeWidth = 1f)
            drawLine(guideColor, Offset(0f, h * 0.75f), Offset(w, h * 0.75f), strokeWidth = 1f)

            // Draw Headroom curve (blue, 0..1)
            if (scales.size >= 2) {
                val scalePath = Path()
                scales.forEachIndexed { idx, v ->
                    val x = w * idx.toFloat() / (scales.size - 1).toFloat()
                    val y = h - (v * h * 0.85f) - h * 0.05f
                    if (idx == 0) scalePath.moveTo(x, y) else scalePath.lineTo(x, y)
                }
                drawPath(
                    path = scalePath,
                    color = Color(0xFF38BDF8),
                    style = Stroke(width = 3.5f, cap = StrokeCap.Round),
                )
            }

            // Draw SoC Temp curve (amber, normalized 25..75°C)
            if (temps.size >= 2) {
                val minT = (temps.min() - 3f).coerceAtLeast(20f)
                val maxT = (temps.max() + 3f).coerceAtLeast(minT + 10f)
                val linePath = Path()
                val fillPath = Path()
                temps.forEachIndexed { idx, t ->
                    val x = w * idx.toFloat() / (temps.size - 1).toFloat()
                    val norm = ((t - minT) / (maxT - minT)).coerceIn(0f, 1f)
                    val y = h - (norm * h * 0.85f) - h * 0.05f
                    if (idx == 0) {
                        linePath.moveTo(x, y)
                        fillPath.moveTo(x, h)
                        fillPath.lineTo(x, y)
                    } else {
                        linePath.lineTo(x, y)
                        fillPath.lineTo(x, y)
                    }
                }
                fillPath.lineTo(w, h)
                fillPath.close()
                drawPath(
                    path = fillPath,
                    brush = Brush.verticalGradient(
                        colors = listOf(Color(0xFFF59E0B).copy(alpha = 0.22f), Color.Transparent),
                    ),
                )
                drawPath(
                    path = linePath,
                    color = Color(0xFFF59E0B),
                    style = Stroke(width = 4f, cap = StrokeCap.Round),
                )
            }
        }
    }
}

@Composable
private fun LegendDot(color: Color, text: String) {
    Row(verticalAlignment = Alignment.CenterVertically, horizontalArrangement = Arrangement.spacedBy(5.dp)) {
        Box(
            modifier = Modifier
                .size(7.dp)
                .clip(CircleShape)
                .background(color),
        )
        Text(
            text = text,
            fontSize = 11.sp,
            fontWeight = FontWeight.Medium,
            color = MiuixTheme.colorScheme.onSurface,
        )
    }
}

@Composable
internal fun MetricPill(
    label: String,
    value: String,
    modifier: Modifier = Modifier,
    accent: Color? = null,
) {
    Column(
        modifier = modifier
            .clip(RoundedCornerShape(12.dp))
            .background(MiuixTheme.colorScheme.secondaryContainer.copy(alpha = 0.45f))
            .padding(horizontal = 9.dp, vertical = 8.dp),
        verticalArrangement = Arrangement.SpaceBetween,
    ) {
        Row(
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.spacedBy(4.dp),
        ) {
            if (accent != null) {
                Box(
                    modifier = Modifier
                        .size(5.dp)
                        .clip(CircleShape)
                        .background(accent),
                )
            }
            Text(
                text = label,
                fontSize = 10.sp,
                color = MiuixTheme.colorScheme.onSurfaceVariantSummary,
                maxLines = 1,
                softWrap = false,
                overflow = TextOverflow.Ellipsis,
            )
        }
        Spacer(Modifier.height(3.dp))
        Text(
            text = value,
            fontSize = 13.sp,
            fontWeight = FontWeight.Bold,
            color = MiuixTheme.colorScheme.onSurface,
            maxLines = 1,
            softWrap = false,
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
