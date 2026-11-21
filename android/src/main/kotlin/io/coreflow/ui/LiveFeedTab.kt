package io.coreflow.ui

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
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
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
import top.yukonga.miuix.kmp.basic.Card
import top.yukonga.miuix.kmp.basic.ScrollBehavior
import top.yukonga.miuix.kmp.basic.SmallTitle
import top.yukonga.miuix.kmp.basic.Text
import top.yukonga.miuix.kmp.theme.MiuixTheme
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

@Composable
fun LiveFeedTab(
    strings: AppStrings,
    status: CoreFlowManager.DaemonStatus?,
    scrollBehavior: ScrollBehavior,
) {
    val episodes = status?.episodes.orEmpty()
    val timeFmt = remember { SimpleDateFormat("HH:mm:ss", Locale.US) }

    LazyColumn(
        modifier = Modifier
            .fillMaxSize()
            .nestedScroll(scrollBehavior.nestedScrollConnection),
        contentPadding = PaddingValues(horizontal = 12.dp, vertical = 8.dp),
        verticalArrangement = Arrangement.spacedBy(10.dp),
    ) {
        item { SmallTitle(text = strings.sectionAiBrain) }

        item {
            val contendedCount = status?.contendedPaths?.size ?: 0
            val sourceLabel = status?.let {
                strings.humanizeThermalSource(it.thermalSource, contendedCount)
            } ?: "--"
            val sourceColor = when {
                contendedCount > 0 -> Color(0xFFF59E0B)
                status?.isRunning == true -> Color(0xFF22C55E)
                else -> Color(0xFF64748B)
            }
            val vel = status?.thermalVelocityCpm ?: 0f
            val trendColor = when {
                vel >= 0.6f -> Color(0xFFEF4444)
                vel <= -0.6f -> Color(0xFF38BDF8)
                else -> Color(0xFF22C55E)
            }

            Card(
                modifier = Modifier.fillMaxWidth(),
                insideMargin = PaddingValues(15.dp),
            ) {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween,
                    verticalAlignment = Alignment.CenterVertically,
                ) {
                    Row(
                        modifier = Modifier.weight(1f),
                        verticalAlignment = Alignment.CenterVertically,
                        horizontalArrangement = Arrangement.spacedBy(8.dp),
                    ) {
                        Box(
                            modifier = Modifier
                                .size(9.dp)
                                .clip(CircleShape)
                                .background(sourceColor),
                        )
                        Text(
                            text = status?.let {
                                "${strings.humanizeMode(it.mode)} · ${it.effectiveProfile.label}"
                            } ?: "Offline",
                            style = MiuixTheme.textStyles.headline1,
                            fontWeight = FontWeight.SemiBold,
                            maxLines = 1,
                            overflow = TextOverflow.Ellipsis,
                        )
                    }
                    Spacer(Modifier.size(8.dp))
                    BadgeTag(sourceLabel, sourceColor)
                }

                Spacer(Modifier.height(12.dp))
                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .height(IntrinsicSize.Min),
                    horizontalArrangement = Arrangement.spacedBy(8.dp),
                ) {
                    MetricPill(
                        label = strings.labelForecast10s,
                        value = status?.let { "${it.aiPredicted10s.toInt().coerceIn(0, 130)}%" } ?: "--",
                        accent = Color(0xFFF59E0B),
                        modifier = Modifier.weight(1f).fillMaxHeight(),
                    )
                    MetricPill(
                        label = strings.labelConfidence,
                        value = status?.let { "${(it.aiConfidence * 100f).toInt().coerceIn(0, 100)}%" } ?: "--",
                        accent = Color(0xFF38BDF8),
                        modifier = Modifier.weight(1f).fillMaxHeight(),
                    )
                    MetricPill(
                        label = strings.labelCpuLoad,
                        value = status?.let { "${it.cpuUtilPct.toInt().coerceIn(0, 100)}%" } ?: "--",
                        accent = Color(0xFFA78BFA),
                        modifier = Modifier.weight(1f).fillMaxHeight(),
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
                        label = strings.labelTrend,
                        value = strings.formatTrend(vel),
                        accent = trendColor,
                        modifier = Modifier.weight(1f).fillMaxHeight(),
                    )
                    MetricPill(
                        label = "Render",
                        value = status?.let { "${(it.renderPressure * 100f).toInt().coerceIn(0, 100)}%" } ?: "--",
                        accent = Color(0xFF10B981),
                        modifier = Modifier.weight(1f).fillMaxHeight(),
                    )
                }
            }
        }

        item {
            BenchmarkCard(strings = strings, status = status)
        }

        item { SmallTitle(text = "${strings.sectionEpisodes} (${episodes.size})") }

        if (episodes.isEmpty()) {
            item {
                Card(
                    modifier = Modifier.fillMaxWidth(),
                    insideMargin = PaddingValues(18.dp),
                ) {
                    Text(
                        text = strings.emptyEpisodes,
                        style = MiuixTheme.textStyles.body2,
                        color = MiuixTheme.colorScheme.onSurfaceVariantSummary,
                    )
                }
            }
        } else {
            items(items = episodes, key = { it.id }) { ep ->
                val domainColor = when (ep.domain) {
                    "power" -> Color(0xFF10B981)
                    "thermal" -> Color(0xFFF59E0B)
                    "storage" -> Color(0xFF38BDF8)
                    "memory" -> Color(0xFF06B6D4)
                    "uclamp" -> Color(0xFFA78BFA)
                    else -> MiuixTheme.colorScheme.primary
                }
                val outcomeColor = when (ep.outcome) {
                    "oem_yield" -> Color(0xFFF59E0B)
                    "rolled_back" -> Color(0xFFEF4444)
                    "skipped" -> Color(0xFF64748B)
                    else -> Color(0xFF22C55E)
                }
                val outcomeText = strings.humanizeOutcome(ep.outcome)
                val tsLabel = if (ep.tsMs > 0L) timeFmt.format(Date(ep.tsMs)) else "--:--:--"

                Card(
                    modifier = Modifier.fillMaxWidth(),
                    insideMargin = PaddingValues(13.dp),
                ) {
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceBetween,
                        verticalAlignment = Alignment.CenterVertically,
                    ) {
                        Row(
                            modifier = Modifier.weight(1f),
                            verticalAlignment = Alignment.CenterVertically,
                            horizontalArrangement = Arrangement.spacedBy(6.dp),
                        ) {
                            BadgeTag(ep.domain.uppercase(Locale.US), domainColor)
                            Text(
                                text = ep.trigger,
                                style = MiuixTheme.textStyles.body2,
                                fontWeight = FontWeight.SemiBold,
                                maxLines = 1,
                                overflow = TextOverflow.Ellipsis,
                            )
                        }
                        Spacer(Modifier.size(8.dp))
                        BadgeTag(outcomeText, outcomeColor)
                    }
                    Spacer(Modifier.height(6.dp))
                    Text(
                        text = ep.action,
                        fontSize = 13.sp,
                        lineHeight = 18.sp,
                        color = MiuixTheme.colorScheme.onSurface,
                        maxLines = 2,
                        overflow = TextOverflow.Ellipsis,
                    )
                    Spacer(Modifier.height(6.dp))
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceBetween,
                        verticalAlignment = Alignment.CenterVertically,
                    ) {
                        Text(
                            text = tsLabel,
                            fontSize = 10.sp,
                            color = MiuixTheme.colorScheme.onSurfaceVariantSummary,
                        )
                        val meta = buildString {
                            ep.socTempC?.let { append(String.format(Locale.US, "%.1f°C · ", it)) }
                            append("RP ${(ep.renderPressure * 100f).toInt().coerceIn(0, 100)}% · ")
                            append("Scale ${(ep.capacityScale * 100f).toInt().coerceIn(0, 100)}%")
                        }
                        Text(
                            text = meta,
                            fontSize = 10.sp,
                            color = MiuixTheme.colorScheme.onSurfaceVariantSummary,
                            maxLines = 1,
                            overflow = TextOverflow.Ellipsis,
                        )
                    }
                }
            }
        }

        item { Spacer(Modifier.height(12.dp)) }
    }
}

@Composable
private fun BadgeTag(label: String, color: Color) {
    Box(
        modifier = Modifier
            .clip(RoundedCornerShape(8.dp))
            .background(color.copy(alpha = 0.16f))
            .padding(horizontal = 8.dp, vertical = 3.dp),
    ) {
        Text(
            text = label,
            fontSize = 10.sp,
            fontWeight = FontWeight.Bold,
            color = color,
            maxLines = 1,
            softWrap = false,
        )
    }
}
