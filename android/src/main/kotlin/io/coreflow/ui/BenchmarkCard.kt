package io.coreflow.ui

import android.content.Intent
import android.content.pm.PackageManager
import android.provider.MediaStore
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import io.coreflow.CoreFlowManager
import kotlinx.coroutines.NonCancellable
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import top.yukonga.miuix.kmp.basic.ButtonDefaults
import top.yukonga.miuix.kmp.basic.Card
import top.yukonga.miuix.kmp.basic.Text
import top.yukonga.miuix.kmp.basic.TextButton
import top.yukonga.miuix.kmp.theme.MiuixTheme
import top.yukonga.miuix.kmp.utils.PressFeedbackType
import java.util.Locale

private data class AbLaunchResult(
    val packageName: String,
    val oemMs: Long,
    val coreflowMs: Long,
)

private data class AbJankResult(
    val packageName: String,
    val oemJankPct: Float,
    val coreflowJankPct: Float,
    val totalFrames: Int,
)

/**
 * Real device A/B Benchmark Card inspired by HyperOS dual-bar comparisons.
 * Measures genuine Android `am start -W` cold-launch latency and `dumpsys gfxinfo` frame drops
 * comparing Stock OEM (Paused) vs CoreFlow Active — zero fabricated numbers.
 */
@Composable
fun BenchmarkCard(
    strings: AppStrings,
    status: CoreFlowManager.DaemonStatus?,
) {
    val context = LocalContext.current
    val scope = rememberCoroutineScope()
    var targetIdx by remember { mutableIntStateOf(0) } // 0 = Camera, 1 = Foreground App
    var running by remember { mutableStateOf(false) }
    var progressMsg by remember { mutableStateOf<String?>(null) }
    var launchResult by remember { mutableStateOf<AbLaunchResult?>(null) }
    var jankResult by remember { mutableStateOf<AbJankResult?>(null) }

    val defaultCameraPkg = remember(context) {
        runCatching {
            val intent = Intent(MediaStore.ACTION_IMAGE_CAPTURE)
            val resolved = context.packageManager.resolveActivity(
                intent,
                PackageManager.ResolveInfoFlags.of(PackageManager.MATCH_DEFAULT_ONLY.toLong()),
            )
            resolved?.activityInfo?.packageName?.takeIf { it.isNotBlank() && it != "android" }
        }.getOrNull() ?: "com.android.camera"
    }

    val fgPkg = status?.foregroundPackage?.takeIf {
        it.isNotBlank() &&
            it != "android" &&
            it != "com.android.systemui" &&
            it != context.packageName &&
            it != "io.coreflow.companion" &&
            !it.startsWith("io.coreflow.companion")
    }
    val fgLabel = remember(fgPkg, strings.isIndonesian) {
        if (fgPkg == null) {
            if (strings.isIndonesian) "App Aktif" else "Active App"
        } else {
            runCatching {
                val appInfo = context.packageManager.getApplicationInfo(
                    fgPkg,
                    PackageManager.ApplicationInfoFlags.of(0L),
                )
                context.packageManager.getApplicationLabel(appInfo).toString()
            }.getOrNull()?.takeIf { it.isNotBlank() && !it.equals("android", ignoreCase = true) }
                ?: run {
                    val parts = fgPkg.split('.').filter { it.isNotBlank() }
                    val tail = parts.lastOrNull().orEmpty()
                    val candidate = if (tail.lowercase(Locale.US) in setOf("android", "app", "global", "mobile", "client") && parts.size >= 2) {
                        parts[parts.size - 2]
                    } else {
                        tail
                    }
                    candidate.replaceFirstChar { it.uppercaseChar() }
                }
        }
    }
    val targetPkg = if (targetIdx == 0) {
        defaultCameraPkg
    } else {
        fgPkg ?: defaultCameraPkg
    }

    Card(
        modifier = Modifier.fillMaxWidth(),
        insideMargin = PaddingValues(15.dp),
        pressFeedbackType = PressFeedbackType.Tilt,
        onClick = {},
    ) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically,
        ) {
            Column(modifier = Modifier.weight(1f)) {
                Text(
                    text = if (strings.isIndonesian) "Uji Nyata A/B (OEM vs CoreFlow)" else "Real A/B Benchmark (OEM vs CoreFlow)",
                    style = MiuixTheme.textStyles.headline2,
                    fontWeight = FontWeight.Bold,
                )
                Text(
                    text = if (strings.isIndonesian) {
                        "Ukur langsung cold-launch (am start -W) & frame drop (gfxinfo)"
                    } else {
                        "Measures real cold-launch (am start -W) & frame drops (gfxinfo)"
                    },
                    fontSize = 11.sp,
                    color = MiuixTheme.colorScheme.onSurfaceVariantSummary,
                )
            }
        }

        Spacer(Modifier.height(10.dp))

        // Target selector pill bar
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .clip(RoundedCornerShape(12.dp))
                .background(MiuixTheme.colorScheme.secondaryContainer.copy(alpha = 0.35f))
                .padding(3.dp),
            horizontalArrangement = Arrangement.spacedBy(4.dp),
        ) {
            val targets = listOf(
                if (strings.isIndonesian) "Kamera" else "Camera",
                fgLabel,
            )
            targets.forEachIndexed { idx, label ->
                val sel = targetIdx == idx
                Box(
                    modifier = Modifier
                        .weight(1f)
                        .height(32.dp)
                        .clip(RoundedCornerShape(9.dp))
                        .background(if (sel) MiuixTheme.colorScheme.primary else Color.Transparent)
                        .clickable(enabled = !running) { targetIdx = idx }
                        .padding(horizontal = 6.dp),
                    contentAlignment = Alignment.Center,
                ) {
                    Text(
                        text = label,
                        fontSize = 11.sp,
                        fontWeight = if (sel) FontWeight.Bold else FontWeight.Medium,
                        color = if (sel) MiuixTheme.colorScheme.onPrimary else MiuixTheme.colorScheme.onSurfaceVariantSummary,
                        maxLines = 1,
                        softWrap = false,
                        overflow = TextOverflow.Ellipsis,
                    )
                }
            }
        }

        Spacer(Modifier.height(10.dp))

        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(8.dp),
        ) {
            TextButton(
                text = if (running) "..." else (if (strings.isIndonesian) "Uji Buka (ms)" else "Launch Test"),
                onClick = {
                    scope.launch {
                        running = true
                        val wasPaused = status?.inspectionMode == true
                        try {
                            progressMsg = if (strings.isIndonesian) "Mengukur Stock OEM ($targetPkg)..." else "Measuring Stock OEM ($targetPkg)..."
                            CoreFlowManager.setPaused(true)
                            delay(500L)
                            val oemMs = CoreFlowManager.measureColdLaunchMs(targetPkg).getOrNull()

                            progressMsg = if (strings.isIndonesian) "Mengukur CoreFlow HyperFluid..." else "Measuring CoreFlow HyperFluid..."
                            CoreFlowManager.setPaused(false)
                            delay(500L)
                            val cfMs = CoreFlowManager.measureColdLaunchMs(targetPkg).getOrNull()

                            if (oemMs != null && cfMs != null) {
                                launchResult = AbLaunchResult(targetPkg, oemMs, cfMs)
                                progressMsg = null
                            } else {
                                progressMsg = if (strings.isIndonesian) {
                                    "Paket $targetPkg tidak ditemukan / gagal diukur."
                                } else {
                                    "Package $targetPkg not found or could not be launched."
                                }
                            }
                        } finally {
                            withContext(NonCancellable) {
                                CoreFlowManager.setPaused(wasPaused)
                            }
                            running = false
                        }
                    }
                },
                enabled = !running,
                colors = ButtonDefaults.textButtonColorsPrimary(),
                modifier = Modifier.weight(1f),
            )
            TextButton(
                text = if (running) "..." else (if (strings.isIndonesian) "Cek Jank %" else "Read Jank %"),
                onClick = {
                    scope.launch {
                        running = true
                        try {
                            progressMsg = if (strings.isIndonesian) "Membaca dumpsys gfxinfo..." else "Reading dumpsys gfxinfo..."
                            val stats = CoreFlowManager.measureGfxFrameDrops(targetPkg).getOrNull()
                            if (stats != null && stats.totalFrames > 0) {
                                val isPausedNow = status?.inspectionMode == true
                                val prev = jankResult
                                jankResult = if (isPausedNow) {
                                    AbJankResult(
                                        packageName = stats.packageName,
                                        oemJankPct = stats.jankPct,
                                        coreflowJankPct = prev?.takeIf { it.packageName == stats.packageName }?.coreflowJankPct ?: stats.jankPct,
                                        totalFrames = stats.totalFrames,
                                    )
                                } else {
                                    AbJankResult(
                                        packageName = stats.packageName,
                                        oemJankPct = prev?.takeIf { it.packageName == stats.packageName }?.oemJankPct ?: stats.jankPct,
                                        coreflowJankPct = stats.jankPct,
                                        totalFrames = stats.totalFrames,
                                    )
                                }
                                progressMsg = null
                            } else {
                                progressMsg = if (strings.isIndonesian) {
                                    "Belum ada data frame gfxinfo untuk $targetPkg — buka & scroll dulu."
                                } else {
                                    "No gfxinfo frames yet for $targetPkg — open & scroll it first."
                                }
                            }
                        } finally {
                            running = false
                        }
                    }
                },
                enabled = !running,
                modifier = Modifier.weight(1f),
            )
        }

        if (!progressMsg.isNullOrBlank()) {
            Spacer(Modifier.height(8.dp))
            Text(
                text = progressMsg!!,
                fontSize = 11.sp,
                color = MiuixTheme.colorScheme.primary,
            )
        }

        launchResult?.let { res ->
            Spacer(Modifier.height(12.dp))
            val maxVal = maxOf(res.oemMs, res.coreflowMs, 1L).toFloat()
            val deltaPct = if (res.oemMs > 0L) ((res.oemMs - res.coreflowMs) * 100f / res.oemMs.toFloat()) else 0f
            DualComparisonBlock(
                title = "Cold Launch · ${res.packageName.substringAfterLast('.')}",
                badge = if (deltaPct >= 0f) String.format(Locale.US, "-%.1f%%", deltaPct) else String.format(Locale.US, "+%.1f%%", -deltaPct),
                badgeColor = if (deltaPct >= 0f) Color(0xFF22C55E) else Color(0xFFF59E0B),
                oemLabel = "Stock OEM",
                oemValue = "${res.oemMs} ms",
                oemFrac = (res.oemMs.toFloat() / maxVal).coerceIn(0.15f, 1f),
                cfLabel = "CoreFlow v6 HyperFluid",
                cfValue = "${res.coreflowMs} ms",
                cfFrac = (res.coreflowMs.toFloat() / maxVal).coerceIn(0.15f, 1f),
            )
        }

        jankResult?.let { res ->
            Spacer(Modifier.height(12.dp))
            val maxVal = maxOf(res.oemJankPct, res.coreflowJankPct, 1f)
            DualComparisonBlock(
                title = "Frame Drop Rate (${res.totalFrames} frames)",
                badge = String.format(Locale.US, "%.2f%%", res.coreflowJankPct),
                badgeColor = if (res.coreflowJankPct <= res.oemJankPct) Color(0xFF22C55E) else Color(0xFFF59E0B),
                oemLabel = "Stock OEM (Paused)",
                oemValue = String.format(Locale.US, "%.2f%%", res.oemJankPct),
                oemFrac = (res.oemJankPct / maxVal).coerceIn(0.08f, 1f),
                cfLabel = "CoreFlow v6 HyperFluid",
                cfValue = String.format(Locale.US, "%.2f%%", res.coreflowJankPct),
                cfFrac = (res.coreflowJankPct / maxVal).coerceIn(0.08f, 1f),
            )
        }
    }
}

@Composable
private fun DualComparisonBlock(
    title: String,
    badge: String,
    badgeColor: Color,
    oemLabel: String,
    oemValue: String,
    oemFrac: Float,
    cfLabel: String,
    cfValue: String,
    cfFrac: Float,
) {
    Column(
        modifier = Modifier
            .fillMaxWidth()
            .clip(RoundedCornerShape(14.dp))
            .background(MiuixTheme.colorScheme.secondaryContainer.copy(alpha = 0.28f))
            .padding(12.dp),
        verticalArrangement = Arrangement.spacedBy(8.dp),
    ) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically,
        ) {
            Text(
                text = title,
                fontSize = 12.sp,
                fontWeight = FontWeight.SemiBold,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis,
                modifier = Modifier.weight(1f),
            )
            Text(
                text = badge,
                fontSize = 11.sp,
                fontWeight = FontWeight.Bold,
                color = badgeColor,
            )
        }

        // OEM Grey Bar
        Column(verticalArrangement = Arrangement.spacedBy(3.dp)) {
            Text(
                text = oemLabel,
                fontSize = 10.sp,
                color = MiuixTheme.colorScheme.onSurfaceVariantSummary,
            )
            Box(
                modifier = Modifier
                    .fillMaxWidth()
                    .height(26.dp),
            ) {
                Box(
                    modifier = Modifier
                        .fillMaxWidth(oemFrac)
                        .fillMaxHeight()
                        .clip(RoundedCornerShape(8.dp))
                        .background(
                            Brush.horizontalGradient(
                                listOf(Color(0xFF334155), Color(0xFF64748B)),
                            ),
                        )
                        .padding(horizontal = 8.dp),
                    contentAlignment = Alignment.CenterEnd,
                ) {
                    Text(
                        text = oemValue,
                        fontSize = 11.sp,
                        fontWeight = FontWeight.SemiBold,
                        color = Color.White,
                        maxLines = 1,
                    )
                }
            }
        }

        // CoreFlow Emerald Gradient Bar
        Column(verticalArrangement = Arrangement.spacedBy(3.dp)) {
            Text(
                text = cfLabel,
                fontSize = 10.sp,
                color = MiuixTheme.colorScheme.onSurfaceVariantSummary,
            )
            Box(
                modifier = Modifier
                    .fillMaxWidth()
                    .height(26.dp),
            ) {
                Box(
                    modifier = Modifier
                        .fillMaxWidth(cfFrac)
                        .fillMaxHeight()
                        .clip(RoundedCornerShape(8.dp))
                        .background(
                            Brush.horizontalGradient(
                                listOf(Color(0xFF86EFAC), Color(0xFF10B981)),
                            ),
                        )
                        .padding(horizontal = 8.dp),
                    contentAlignment = Alignment.CenterEnd,
                ) {
                    Text(
                        text = cfValue,
                        fontSize = 11.sp,
                        fontWeight = FontWeight.Bold,
                        color = Color(0xFF052E16),
                        maxLines = 1,
                    )
                }
            }
        }
    }
}
