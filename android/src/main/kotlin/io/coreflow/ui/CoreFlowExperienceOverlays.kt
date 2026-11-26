package io.coreflow.ui

import androidx.compose.animation.core.FastOutSlowInEasing
import androidx.compose.animation.core.LinearEasing
import androidx.compose.animation.core.RepeatMode
import androidx.compose.animation.core.animateFloat
import androidx.compose.animation.core.infiniteRepeatable
import androidx.compose.animation.core.rememberInfiniteTransition
import androidx.compose.animation.core.tween
import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.navigationBarsPadding
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.statusBarsPadding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.window.Dialog
import androidx.compose.ui.window.DialogProperties
import io.coreflow.BuildConfig
import io.coreflow.CoreFlowUpdateChecker
import top.yukonga.miuix.kmp.basic.ButtonDefaults
import top.yukonga.miuix.kmp.basic.Card
import top.yukonga.miuix.kmp.basic.Text
import top.yukonga.miuix.kmp.basic.TextButton
import top.yukonga.miuix.kmp.theme.MiuixTheme

@Composable
fun CoreFlowStartupSplash(
    isIndonesian: Boolean,
    statusLabel: String,
    modifier: Modifier = Modifier,
) {
    val transition = rememberInfiniteTransition(label = "CoreFlowSplashAnim")
    val sweepAngle by transition.animateFloat(
        initialValue = 0f,
        targetValue = 360f,
        animationSpec = infiniteRepeatable(
            animation = tween(durationMillis = 1250, easing = LinearEasing),
            repeatMode = RepeatMode.Restart,
        ),
        label = "RingSweep",
    )
    val pulse by transition.animateFloat(
        initialValue = 0.72f,
        targetValue = 1.0f,
        animationSpec = infiniteRepeatable(
            animation = tween(durationMillis = 900, easing = FastOutSlowInEasing),
            repeatMode = RepeatMode.Reverse,
        ),
        label = "CorePulse",
    )

    val bgColor = MiuixTheme.colorScheme.background
    val cyan = Color(0xFF00E5FF)
    val emerald = Color(0xFF10B981)
    val violet = Color(0xFF818CF8)

    Box(
        modifier = modifier
            .fillMaxSize()
            .background(bgColor),
        contentAlignment = Alignment.Center,
    ) {
        Column(
            horizontalAlignment = Alignment.CenterHorizontally,
            verticalArrangement = Arrangement.Center,
            modifier = Modifier.padding(horizontal = 32.dp),
        ) {
            Box(
                modifier = Modifier.size(92.dp),
                contentAlignment = Alignment.Center,
            ) {
                Canvas(modifier = Modifier.fillMaxSize()) {
                    val strokeWidth = 4.5.dp.toPx()
                    val pad = strokeWidth / 2f
                    val arcSize = Size(size.width - strokeWidth, size.height - strokeWidth)
                    drawCircle(
                        color = cyan.copy(alpha = 0.14f * pulse),
                        radius = size.minDimension * 0.48f,
                    )
                    drawArc(
                        brush = Brush.sweepGradient(
                            colors = listOf(
                                Color.Transparent,
                                cyan.copy(alpha = 0.35f),
                                cyan,
                                emerald,
                                violet,
                                Color.Transparent,
                            ),
                        ),
                        startAngle = sweepAngle,
                        sweepAngle = 250f,
                        useCenter = false,
                        topLeft = Offset(pad, pad),
                        size = arcSize,
                        style = Stroke(width = strokeWidth, cap = StrokeCap.Round),
                    )
                }
                Text(
                    text = "CF",
                    fontSize = 24.sp,
                    fontWeight = FontWeight.ExtraBold,
                    color = MiuixTheme.colorScheme.onBackground,
                )
            }

            Spacer(Modifier.height(22.dp))
            Text(
                text = "CoreFlow Engine",
                style = MiuixTheme.textStyles.title2,
                fontWeight = FontWeight.Bold,
                color = MiuixTheme.colorScheme.onBackground,
            )
            Spacer(Modifier.height(6.dp))
            Box(
                modifier = Modifier
                    .clip(RoundedCornerShape(999.dp))
                    .background(cyan.copy(alpha = 0.14f))
                    .border(BorderStroke(1.dp, cyan.copy(alpha = 0.35f)), RoundedCornerShape(999.dp))
                    .padding(horizontal = 12.dp, vertical = 4.dp),
            ) {
                Text(
                    text = "v${BuildConfig.VERSION_NAME} · Zero-Profile Autonomous",
                    style = MiuixTheme.textStyles.footnote1,
                    fontWeight = FontWeight.SemiBold,
                    color = cyan,
                )
            }
            Spacer(Modifier.height(20.dp))
            Text(
                text = statusLabel.ifBlank {
                    if (isIndonesian) "Menyiapkan telemetri otonom..." else "Initializing autonomous telemetry..."
                },
                style = MiuixTheme.textStyles.footnote1,
                color = MiuixTheme.colorScheme.onSurfaceVariantSummary,
                textAlign = TextAlign.Center,
            )
        }
    }
}

private data class OnboardingPillar(
    val badge: String,
    val title: String,
    val description: String,
    val accent: Color,
)

@Composable
fun CoreFlowOnboardingScreen(
    isIndonesian: Boolean,
    onFinish: () -> Unit,
    modifier: Modifier = Modifier,
) {
    val pillars = if (isIndonesian) {
        listOf(
            OnboardingPillar(
                badge = "OTONOM 100%",
                title = "Tanpa Profil Manual (Zero-Profile)",
                description = "CoreFlow adalah daemon kernel C++23 + Rust yang membaca sentuhan, scroll, beban aplikasi, dan game 3D secara real-time — cukup pasang dan pakai tanpa gonta-ganti profil.",
                accent = Color(0xFF00E5FF),
            ),
            OnboardingPillar(
                badge = "FISIKA TERMAL & EDP",
                title = "Model RC Termal & Lutut Silikon",
                description = "Mempelajari karakteristik panas casing (R_θ, τ) dan kurva daya silikon (CFEDP2) langsung dari perangkatmu. Otomatis menyesuaikan anggaran daya di suhu ruangan maupun saat memakai cooler.",
                accent = Color(0xFF10B981),
            ),
            OnboardingPillar(
                badge = "P99 & KAUSAL",
                title = "Kendali Frame p99 & Verifikasi Kausal",
                description = "Menjaga respons layar 60–144Hz tetap mulus dengan kendali deadline p99, sekaligus menguji efektivitas tiap knob kernel (CFEM2) agar fitur tanpa manfaat otomatis dilewati.",
                accent = Color(0xFFA78BFA),
            ),
            OnboardingPillar(
                badge = "AMAN & ZERO-WEAR",
                title = "Rollback Penuh & Co-op OEM",
                description = "Setiap perubahan tercatat di jurnal CFJ4 untuk pemulihan instan, bekerja kooperatif dengan daemon OEM tanpa perang tulis, serta mendukung eksklusi Stock OEM per aplikasi.",
                accent = Color(0xFFF59E0B),
            ),
        )
    } else {
        listOf(
            OnboardingPillar(
                badge = "100% AUTONOMOUS",
                title = "Zero-Profile Adaptive Control",
                description = "CoreFlow is a native C++23 + Rust kernel daemon that adapts in real time to touch, scrolling, app launches, and 3D games — install once and let it work with zero manual profile switching.",
                accent = Color(0xFF00E5FF),
            ),
            OnboardingPillar(
                badge = "RC THERMAL & EDP",
                title = "Online Thermal Physics & Silicon Knee",
                description = "Learns your device's real chassis thermal model (R_θ, τ) and silicon power curve (CFEDP2) on-device — automatically scaling power headroom for ambient heat or external coolers.",
                accent = Color(0xFF10B981),
            ),
            OnboardingPillar(
                badge = "P99 & CAUSAL",
                title = "Per-Frame p99 & Causal Verification",
                description = "Protects 60–144Hz fluidity via p99 frame deadline control while continuously verifying each kernel actuator (CFEM2) and auto-bypassing ineffective OEM knobs.",
                accent = Color(0xFFA78BFA),
            ),
            OnboardingPillar(
                badge = "SAFE & ZERO-WEAR",
                title = "CFJ4 Rollback & OEM Co-op",
                description = "Every change is journaled in CFJ4 for clean recovery, cooperates peacefully with vendor daemons without write-wars, and supports per-app Stock OEM exclusion.",
                accent = Color(0xFFF59E0B),
            ),
        )
    }

    Column(
        modifier = modifier
            .fillMaxSize()
            .background(MiuixTheme.colorScheme.background)
            .statusBarsPadding()
            .navigationBarsPadding()
            .padding(horizontal = 16.dp, vertical = 14.dp),
    ) {
        Column(
            modifier = Modifier
                .weight(1f)
                .verticalScroll(rememberScrollState()),
            verticalArrangement = Arrangement.spacedBy(12.dp),
        ) {
            Spacer(Modifier.height(6.dp))
            Box(
                modifier = Modifier
                    .clip(RoundedCornerShape(999.dp))
                    .background(Color(0xFF00E5FF).copy(alpha = 0.14f))
                    .padding(horizontal = 12.dp, vertical = 5.dp),
            ) {
                Text(
                    text = if (isIndonesian) "SELAMAT DATANG · v${BuildConfig.VERSION_NAME}" else "WELCOME · v${BuildConfig.VERSION_NAME}",
                    style = MiuixTheme.textStyles.footnote1,
                    fontWeight = FontWeight.Bold,
                    color = Color(0xFF00E5FF),
                )
            }
            Text(
                text = if (isIndonesian) "Kenalan dengan CoreFlow Engine" else "Meet CoreFlow Engine",
                style = MiuixTheme.textStyles.title1,
                fontWeight = FontWeight.Bold,
                color = MiuixTheme.colorScheme.onBackground,
            )
            Text(
                text = if (isIndonesian) {
                    "Mesin optimasi kernel Android otonom berbasis fisika untuk performa stabil, suhu terjaga, dan efisiensi daya nyata."
                } else {
                    "An autonomous, physics-driven Android kernel framework built for steady frame pacing, controlled thermals, and real power efficiency."
                },
                style = MiuixTheme.textStyles.body2,
                color = MiuixTheme.colorScheme.onSurfaceVariantSummary,
            )
            Spacer(Modifier.height(4.dp))

            for (pillar in pillars) {
                Card(
                    modifier = Modifier
                        .fillMaxWidth()
                        .border(
                            BorderStroke(1.dp, pillar.accent.copy(alpha = 0.28f)),
                            RoundedCornerShape(16.dp),
                        ),
                    insideMargin = PaddingValues(14.dp),
                ) {
                    Row(
                        verticalAlignment = Alignment.CenterVertically,
                        horizontalArrangement = Arrangement.spacedBy(8.dp),
                    ) {
                        Box(
                            modifier = Modifier
                                .size(8.dp)
                                .clip(CircleShape)
                                .background(pillar.accent),
                        )
                        Text(
                            text = pillar.badge,
                            style = MiuixTheme.textStyles.footnote1,
                            fontWeight = FontWeight.Bold,
                            color = pillar.accent,
                        )
                    }
                    Spacer(Modifier.height(5.dp))
                    Text(
                        text = pillar.title,
                        style = MiuixTheme.textStyles.headline2,
                        fontWeight = FontWeight.SemiBold,
                        color = MiuixTheme.colorScheme.onSurface,
                    )
                    Spacer(Modifier.height(4.dp))
                    Text(
                        text = pillar.description,
                        style = MiuixTheme.textStyles.footnote1,
                        color = MiuixTheme.colorScheme.onSurfaceVariantSummary,
                    )
                }
            }
            Spacer(Modifier.height(8.dp))
        }

        Spacer(Modifier.height(10.dp))
        TextButton(
            text = if (isIndonesian) "Mulai Gunakan CoreFlow" else "Get Started",
            onClick = onFinish,
            colors = ButtonDefaults.textButtonColorsPrimary(),
            modifier = Modifier.fillMaxWidth(),
        )
    }
}

@Composable
fun CoreFlowUpdatePopupDialog(
    isIndonesian: Boolean,
    report: CoreFlowUpdateChecker.UpdateCheckReport,
    onOpenUpdateCenter: () -> Unit,
    onDirectDownload: (String) -> Unit,
    onDismiss: () -> Unit,
) {
    val accent = Color(0xFF00E5FF)
    val directUrl = when {
        report.appUpdateAvailable && !report.apkDownloadUrl.isNullOrBlank() -> report.apkDownloadUrl
        report.moduleUpdateAvailable && !report.zipDownloadUrl.isNullOrBlank() -> report.zipDownloadUrl
        else -> null
    }
    val primaryLabel = when {
        report.appUpdateAvailable && !report.apkDownloadUrl.isNullOrBlank() ->
            if (isIndonesian) "Unduh Companion APK" else "Download Companion APK"
        report.moduleUpdateAvailable && !report.zipDownloadUrl.isNullOrBlank() ->
            if (isIndonesian) "Unduh Modul ZIP" else "Download Module ZIP"
        else -> if (isIndonesian) "Buka Update Center" else "Open Update Center"
    }

    Dialog(
        onDismissRequest = onDismiss,
        properties = DialogProperties(usePlatformDefaultWidth = false),
    ) {
        Card(
            modifier = Modifier
                .fillMaxWidth()
                .padding(horizontal = 20.dp)
                .border(BorderStroke(1.dp, accent.copy(alpha = 0.40f)), RoundedCornerShape(20.dp)),
            insideMargin = PaddingValues(18.dp),
        ) {
            Row(
                verticalAlignment = Alignment.CenterVertically,
                horizontalArrangement = Arrangement.spacedBy(8.dp),
            ) {
                Box(
                    modifier = Modifier
                        .size(9.dp)
                        .clip(CircleShape)
                        .background(accent),
                )
                Text(
                    text = if (isIndonesian) "PEMBARUAN TERSEDIA" else "UPDATE AVAILABLE",
                    style = MiuixTheme.textStyles.footnote1,
                    fontWeight = FontWeight.Bold,
                    color = accent,
                )
            }

            Spacer(Modifier.height(6.dp))
            Text(
                text = "CoreFlow Engine v${report.latestVersion}",
                style = MiuixTheme.textStyles.headline1,
                fontWeight = FontWeight.Bold,
                color = MiuixTheme.colorScheme.onSurface,
            )

            Spacer(Modifier.height(8.dp))
            if (report.moduleUpdateAvailable) {
                val fromMod = report.installedModuleVersion.ifBlank { "?" }
                Text(
                    text = if (isIndonesian) {
                        "• Modul Engine: v$fromMod → v${report.latestVersion}"
                    } else {
                        "• Engine Module: v$fromMod → v${report.latestVersion}"
                    },
                    style = MiuixTheme.textStyles.body2,
                    color = MiuixTheme.colorScheme.onSurface,
                )
            }
            if (report.appUpdateAvailable) {
                val fromApp = report.installedAppVersion.ifBlank { "?" }
                Text(
                    text = if (isIndonesian) {
                        "• Companion App: v$fromApp → v${report.latestAppVersion.ifBlank { report.latestVersion }}"
                    } else {
                        "• Companion App: v$fromApp → v${report.latestAppVersion.ifBlank { report.latestVersion }}"
                    },
                    style = MiuixTheme.textStyles.body2,
                    color = MiuixTheme.colorScheme.onSurface,
                )
            }

            if (report.changelogSummary.isNotBlank()) {
                Spacer(Modifier.height(10.dp))
                Box(
                    modifier = Modifier
                        .fillMaxWidth()
                        .clip(RoundedCornerShape(12.dp))
                        .background(MiuixTheme.colorScheme.surfaceVariant.copy(alpha = 0.55f))
                        .padding(10.dp),
                ) {
                    Text(
                        text = report.changelogSummary,
                        style = MiuixTheme.textStyles.footnote1,
                        color = MiuixTheme.colorScheme.onSurfaceVariantSummary,
                    )
                }
            }

            Spacer(Modifier.height(14.dp))
            TextButton(
                text = primaryLabel,
                onClick = {
                    if (directUrl != null) {
                        onDirectDownload(directUrl)
                    } else {
                        onOpenUpdateCenter()
                    }
                    onDismiss()
                },
                colors = ButtonDefaults.textButtonColorsPrimary(),
                modifier = Modifier.fillMaxWidth(),
            )
            Spacer(Modifier.height(8.dp))
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.spacedBy(8.dp),
            ) {
                TextButton(
                    text = if (isIndonesian) "Update Center" else "Update Center",
                    onClick = {
                        onDismiss()
                        onOpenUpdateCenter()
                    },
                    modifier = Modifier.weight(1f),
                )
                TextButton(
                    text = if (isIndonesian) "Nanti Saja" else "Later",
                    onClick = onDismiss,
                    modifier = Modifier.weight(1f),
                )
            }
        }
    }
}
