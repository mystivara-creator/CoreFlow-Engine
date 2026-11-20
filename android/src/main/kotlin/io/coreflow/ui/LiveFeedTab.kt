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
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
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
    val timeFmt = SimpleDateFormat("HH:mm:ss", Locale.US)

    LazyColumn(
        modifier = Modifier
            .fillMaxSize()
            .nestedScroll(scrollBehavior.nestedScrollConnection),
        contentPadding = PaddingValues(horizontal = 12.dp, vertical = 8.dp),
        verticalArrangement = Arrangement.spacedBy(10.dp),
    ) {
        item { SmallTitle(text = strings.sectionAiBrain) }

        item {
            Card(
                modifier = Modifier.fillMaxWidth(),
                insideMargin = PaddingValues(16.dp),
            ) {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween,
                    verticalAlignment = Alignment.CenterVertically,
                ) {
                    Text(
                        text = status?.let { "${it.mode} · ${it.effectiveProfile.label}" } ?: "Offline",
                        style = MiuixTheme.textStyles.headline1,
                        fontWeight = FontWeight.SemiBold,
                    )
                    Text(
                        text = status?.thermalSource ?: "--",
                        fontSize = 11.sp,
                        color = MiuixTheme.colorScheme.primary,
                    )
                }
                Spacer(Modifier.height(10.dp))
                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .height(IntrinsicSize.Min),
                    horizontalArrangement = Arrangement.spacedBy(8.dp),
                ) {
                    MetricPill(
                        label = "10s Forecast",
                        value = status?.let { String.format(Locale.US, "%.0f idx", it.aiPredicted10s) } ?: "--",
                        accent = Color(0xFFF59E0B),
                        modifier = Modifier.weight(1f).fillMaxHeight(),
                    )
                    MetricPill(
                        label = "Confidence",
                        value = status?.let { "${(it.aiConfidence * 100f).toInt().coerceIn(0, 100)}%" } ?: "--",
                        accent = Color(0xFF38BDF8),
                        modifier = Modifier.weight(1f).fillMaxHeight(),
                    )
                    MetricPill(
                        label = "CPU Util",
                        value = status?.let { "${it.cpuUtilPct.toInt().coerceIn(0, 100)}%" } ?: "--",
                        accent = Color(0xFFA78BFA),
                        modifier = Modifier.weight(1f).fillMaxHeight(),
                    )
                }
            }
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
                    "uclamp" -> Color(0xFFA78BFA)
                    else -> MiuixTheme.colorScheme.primary
                }
                val (outcomeText, outcomeColor) = when (ep.outcome) {
                    "oem_yield" -> "OEM Yield" to Color(0xFFF59E0B)
                    "rolled_back" -> "Rolled Back" to Color(0xFFEF4444)
                    "skipped" -> "Skipped" to Color(0xFF64748B)
                    else -> "Verified" to Color(0xFF22C55E)
                }
                val tsLabel = if (ep.tsMs > 0L) timeFmt.format(Date(ep.tsMs)) else "--:--:--"

                Card(
                    modifier = Modifier.fillMaxWidth(),
                    insideMargin = PaddingValues(14.dp),
                ) {
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceBetween,
                        verticalAlignment = Alignment.CenterVertically,
                    ) {
                        Row(
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
                        BadgeTag(outcomeText, outcomeColor)
                    }
                    Spacer(Modifier.height(6.dp))
                    Text(
                        text = ep.action,
                        style = MiuixTheme.textStyles.body2,
                        color = MiuixTheme.colorScheme.onSurface,
                    )
                    Spacer(Modifier.height(6.dp))
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceBetween,
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
        )
    }
}
