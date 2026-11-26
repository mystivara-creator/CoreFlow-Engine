package io.coreflow.ui

import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.horizontalScroll
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
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.BasicTextField
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateListOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.SolidColor
import androidx.compose.ui.input.nestedscroll.nestedScroll
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleEventObserver
import androidx.lifecycle.compose.LocalLifecycleOwner
import io.coreflow.CoreFlowDialogueEngine
import io.coreflow.CoreFlowManager
import io.coreflow.CoreFlowOnnxNlu
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import top.yukonga.miuix.kmp.basic.ButtonDefaults
import top.yukonga.miuix.kmp.basic.Card
import top.yukonga.miuix.kmp.basic.CardDefaults
import top.yukonga.miuix.kmp.basic.PullToRefresh
import top.yukonga.miuix.kmp.basic.ScrollBehavior
import top.yukonga.miuix.kmp.basic.SmallTitle
import top.yukonga.miuix.kmp.basic.Text
import top.yukonga.miuix.kmp.basic.TextButton
import top.yukonga.miuix.kmp.basic.rememberPullToRefreshState
import top.yukonga.miuix.kmp.theme.MiuixTheme
import top.yukonga.miuix.kmp.utils.PressFeedbackType
import top.yukonga.miuix.kmp.utils.overScrollVertical
import top.yukonga.miuix.kmp.utils.scrollEndHaptic
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

private data class DialogueMessage(
    val id: Long,
    val fromUser: Boolean,
    val text: String,
    val tags: List<String> = emptyList(),
)

@Composable
fun LiveFeedTab(
    strings: AppStrings,
    status: CoreFlowManager.DaemonStatus?,
    isRefreshing: Boolean,
    contentPadding: PaddingValues,
    scrollBehavior: ScrollBehavior,
    onRefresh: () -> Unit,
    pendingPrompt: String? = null,
    onConsumePrompt: () -> Unit = {},
    onTogglePause: (Boolean) -> Unit = {},
    onResolveTamper: (String) -> Unit = {},
) {
    val context = LocalContext.current
    val lifecycleOwner = LocalLifecycleOwner.current
    val scope = rememberCoroutineScope()
    val episodes = status?.episodes.orEmpty()
    val timeFmt = remember { SimpleDateFormat("HH:mm:ss", Locale.US) }
    val pullState = rememberPullToRefreshState()

    val messages = remember { mutableStateListOf<DialogueMessage>() }
    var inputQuery by remember { mutableStateOf("") }
    var msgSeq by remember { mutableStateOf(1L) }
    var onnxNluReady by remember { mutableStateOf(false) }
    var analyzingSession by remember { mutableStateOf(false) }
    var anomalyReport by remember(strings.isIndonesian) {
        mutableStateOf<CoreFlowOnnxNlu.SessionAnomalyReport?>(null)
    }

    // Phase B Lifecycle Discipline: lazy-load ONNX off-main-thread on Live AI open,
    // deterministically close on ON_STOP / dispose so background RAM is strictly 0 MB.
    // Lifecycle.addObserver() replays ON_START immediately when already STARTED/RESUMED.
    DisposableEffect(lifecycleOwner, context) {
        var openJob: Job? = null
        val observer = LifecycleEventObserver { _, event ->
            when (event) {
                Lifecycle.Event.ON_START -> {
                    openJob?.cancel()
                    openJob = scope.launch {
                        val ready = withContext(Dispatchers.IO) { CoreFlowOnnxNlu.open(context) }
                        onnxNluReady = ready && CoreFlowOnnxNlu.isLoaded
                    }
                }
                Lifecycle.Event.ON_STOP -> {
                    openJob?.cancel()
                    openJob = null
                    CoreFlowOnnxNlu.close()
                    onnxNluReady = false
                }
                else -> Unit
            }
        }
        lifecycleOwner.lifecycle.addObserver(observer)
        onDispose {
            openJob?.cancel()
            openJob = null
            lifecycleOwner.lifecycle.removeObserver(observer)
            CoreFlowOnnxNlu.close()
            onnxNluReady = false
        }
    }

    fun submitQuery(raw: String) {
        val trimmed = raw.trim()
        if (trimmed.isEmpty()) return
        messages.add(DialogueMessage(id = msgSeq++, fromUser = true, text = trimmed))
        inputQuery = ""
        val statusSnapshot = status
        val isId = strings.isIndonesian
        scope.launch {
            val (res, loaded) = withContext(Dispatchers.IO) {
                val nluOpen = if (CoreFlowOnnxNlu.isLoaded) true else CoreFlowOnnxNlu.open(context)
                val evaluated = CoreFlowDialogueEngine.evaluate(
                    rawQuery = trimmed,
                    status = statusSnapshot,
                    isIndonesian = isId,
                    journalEntryCount = statusSnapshot?.baselineNodesSnapshotted,
                )
                evaluated to nluOpen
            }
            onnxNluReady = loaded && CoreFlowOnnxNlu.isLoaded
            messages.add(
                DialogueMessage(
                    id = msgSeq++,
                    fromUser = false,
                    text = res.reply,
                    tags = res.telemetryTags,
                ),
            )
            while (messages.size > 16) {
                messages.removeAt(0)
            }
            res.requestedInspectionMode?.let { onTogglePause(it) }
            res.requestedTamperResyncNode?.let { onResolveTamper(it) }
        }
    }

    LaunchedEffect(pendingPrompt, status?.isRunning) {
        if (!pendingPrompt.isNullOrBlank()) {
            submitQuery(pendingPrompt)
            onConsumePrompt()
        }
    }

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
            // Pillar 4: Minimalist Dark CFAE Telemetry-Driven Sovereign Dialogue Console
            item { SmallTitle(text = strings.sectionDialogueConsole) }

            item {
                val consoleBg = Color(0xFF0E131B)
                val consoleBorder = Color(0xFF223047)
                val quickPrompts = remember(strings.isIndonesian) {
                    CoreFlowDialogueEngine.quickPrompts(strings.isIndonesian)
                }
                val lockedNodes = status?.tamperGuard?.lockedNodes ?: 0
                val tamperEvents = status?.tamperGuard?.totalEvents ?: 0L
                val busScale = status?.coupledOrchestration?.memoryBusScale ?: 1.0f

                Card(
                    modifier = Modifier
                        .fillMaxWidth()
                        .border(BorderStroke(1.dp, consoleBorder), RoundedCornerShape(20.dp)),
                    colors = CardDefaults.defaultColors(color = consoleBg),
                    insideMargin = PaddingValues(14.dp),
                ) {
                    // Active live telemetry indicator bar
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceBetween,
                        verticalAlignment = Alignment.CenterVertically,
                    ) {
                        Row(
                            verticalAlignment = Alignment.CenterVertically,
                            horizontalArrangement = Arrangement.spacedBy(7.dp),
                        ) {
                            Box(
                                modifier = Modifier
                                    .size(8.dp)
                                    .clip(CircleShape)
                                    .background(
                                        if (tamperEvents > 0L) Color(0xFFF59E0B)
                                        else if (status?.isRunning == true) Color(0xFF10B981)
                                        else Color(0xFF64748B),
                                    ),
                            )
                            Text(
                                text = "CFAE TELEMETRY LINK",
                                fontSize = 11.sp,
                                fontWeight = FontWeight.Bold,
                                color = Color(0xFFE2E8F0),
                            )
                        }
                        Row(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                            if (onnxNluReady) {
                                BadgeTag("ONNX NLU", Color(0xFFA78BFA))
                            }
                            BadgeTag("0444: $lockedNodes", Color(0xFF38BDF8))
                            BadgeTag(
                                String.format(Locale.US, "BUS %.2fx", busScale),
                                if (busScale > 1.02f) Color(0xFF10B981) else Color(0xFF94A3B8),
                            )
                        }
                    }

                    Spacer(Modifier.height(10.dp))

                    // One-tap deterministic suggestion chips
                    Row(
                        modifier = Modifier
                            .fillMaxWidth()
                            .horizontalScroll(rememberScrollState()),
                        horizontalArrangement = Arrangement.spacedBy(6.dp),
                    ) {
                        for (chip in quickPrompts) {
                            Box(
                                modifier = Modifier
                                    .clip(RoundedCornerShape(10.dp))
                                    .background(Color(0xFF192231))
                                    .border(BorderStroke(0.75.dp, Color(0xFF2B3B55)), RoundedCornerShape(10.dp))
                                    .clickable { submitQuery(chip) }
                                    .padding(horizontal = 10.dp, vertical = 6.dp),
                            ) {
                                Text(
                                    text = chip,
                                    fontSize = 11.sp,
                                    color = Color(0xFFCBD5E1),
                                    maxLines = 1,
                                    softWrap = false,
                                )
                            }
                        }
                    }

                    Spacer(Modifier.height(10.dp))

                    // Dialogue transcript (or live sovereign welcome prompt)
                    if (messages.isEmpty()) {
                        val welcome = CoreFlowDialogueEngine.evaluate(
                            rawQuery = "",
                            status = status,
                            isIndonesian = strings.isIndonesian,
                            journalEntryCount = status?.baselineNodesSnapshotted,
                        )
                        Box(
                            modifier = Modifier
                                .fillMaxWidth()
                                .clip(RoundedCornerShape(12.dp))
                                .background(Color(0xFF141C28))
                                .padding(11.dp),
                        ) {
                            Text(
                                text = welcome.reply,
                                fontSize = 12.sp,
                                lineHeight = 17.sp,
                                color = Color(0xFF94A3B8),
                            )
                        }
                    } else {
                        Column(
                            modifier = Modifier.fillMaxWidth(),
                            verticalArrangement = Arrangement.spacedBy(8.dp),
                        ) {
                            for (m in messages.takeLast(6)) {
                                Box(
                                    modifier = Modifier.fillMaxWidth(),
                                    contentAlignment = if (m.fromUser) Alignment.CenterEnd else Alignment.CenterStart,
                                ) {
                                    Column(
                                        modifier = Modifier
                                            .fillMaxWidth(if (m.fromUser) 0.85f else 1f)
                                            .clip(RoundedCornerShape(12.dp))
                                            .background(
                                                if (m.fromUser) Color(0xFF1D4ED8).copy(alpha = 0.28f)
                                                else Color(0xFF141C28),
                                            )
                                            .border(
                                                BorderStroke(
                                                    0.75.dp,
                                                    if (m.fromUser) Color(0xFF3B82F6).copy(alpha = 0.45f)
                                                    else Color(0xFF233249),
                                                ),
                                                RoundedCornerShape(12.dp),
                                            )
                                            .padding(10.dp),
                                    ) {
                                        Text(
                                            text = m.text,
                                            fontSize = 12.sp,
                                            lineHeight = 17.sp,
                                            color = if (m.fromUser) Color(0xFFEFF6FF) else Color(0xFFE2E8F0),
                                        )
                                        if (m.tags.isNotEmpty()) {
                                            Spacer(Modifier.height(6.dp))
                                            Row(horizontalArrangement = Arrangement.spacedBy(5.dp)) {
                                                for (tag in m.tags.take(3)) {
                                                    BadgeTag(tag, Color(0xFF38BDF8))
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Spacer(Modifier.height(10.dp))

                    // Minimalist dark CFAE input row
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        verticalAlignment = Alignment.CenterVertically,
                        horizontalArrangement = Arrangement.spacedBy(8.dp),
                    ) {
                        Box(
                            modifier = Modifier
                                .weight(1f)
                                .clip(RoundedCornerShape(12.dp))
                                .background(Color(0xFF161F2D))
                                .border(BorderStroke(0.75.dp, Color(0xFF2B3B55)), RoundedCornerShape(12.dp))
                                .padding(horizontal = 12.dp, vertical = 10.dp),
                        ) {
                            if (inputQuery.isEmpty()) {
                                Text(
                                    text = strings.dialogueInputHint,
                                    fontSize = 12.sp,
                                    color = Color(0xFF64748B),
                                    maxLines = 1,
                                    overflow = TextOverflow.Ellipsis,
                                )
                            }
                            BasicTextField(
                                value = inputQuery,
                                onValueChange = { inputQuery = it },
                                singleLine = true,
                                textStyle = TextStyle(
                                    color = Color(0xFFF1F5F9),
                                    fontSize = 12.sp,
                                ),
                                cursorBrush = SolidColor(Color(0xFF38BDF8)),
                                modifier = Modifier.fillMaxWidth(),
                            )
                        }
                        TextButton(
                            text = strings.dialogueSend,
                            onClick = { submitQuery(inputQuery) },
                            enabled = inputQuery.isNotBlank(),
                            colors = ButtonDefaults.textButtonColorsPrimary(),
                        )
                    }
                }
            }

            item { SmallTitle(text = strings.sectionAiBrain) }

            item {
                val settledYields = status?.oemYieldedPaths.orEmpty().toSet()
                val activeContention = status?.contendedPaths.orEmpty().filterNot { it in settledYields }
                val contendedCount = maxOf(
                    (status?.contendedPaths.orEmpty() + settledYields).toSet().size,
                    status?.oemYieldedNodes ?: 0,
                )
                val sourceLabel = status?.let {
                    strings.humanizeThermalSource(it.thermalSource, contendedCount)
                } ?: "--"
                val sourceColor = when {
                    activeContention.isNotEmpty() -> Color(0xFFF59E0B)
                    (status?.oemYieldedNodes ?: 0) > 0 -> Color(0xFF38BDF8)
                    status?.isRunning == true -> Color(0xFF22C55E)
                    else -> Color(0xFF64748B)
                }
                val vel = status?.thermalVelocityCpm ?: 0f
                val trendColor = when {
                    vel >= 0.25f -> Color(0xFFEF4444)
                    vel <= -0.25f -> Color(0xFF38BDF8)
                    else -> Color(0xFF22C55E)
                }

                val jankRiskPct = status?.let { (it.aiJankRisk * 100f).toInt().coerceIn(0, 100) }
                val jankAccent = when {
                    jankRiskPct == null -> Color(0xFF64748B)
                    jankRiskPct >= 60 -> Color(0xFFEF4444)
                    jankRiskPct >= 30 -> Color(0xFFF59E0B)
                    else -> Color(0xFF22C55E)
                }
                val backendLabel = status?.let { strings.humanizeAiBackend(it.aiBackend) } ?: "--"
                val backendColor = when (status?.aiBackend) {
                    "onnx-runtime-live" -> Color(0xFFA78BFA)
                    "cfw2-micro-mlp" -> Color(0xFF38BDF8)
                    else -> sourceColor
                }

                Card(
                    modifier = Modifier.fillMaxWidth(),
                    insideMargin = PaddingValues(15.dp),
                    pressFeedbackType = PressFeedbackType.Tilt,
                    onClick = {},
                ) {
                    Row(
                        modifier = Modifier.fillMaxWidth(),
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
                                "${it.autonomousRegimeLabel(strings.isIndonesian)} · ${strings.humanizeMode(it.mode)}"
                            } ?: "Offline",
                            style = MiuixTheme.textStyles.headline1,
                            fontWeight = FontWeight.SemiBold,
                            maxLines = 1,
                            overflow = TextOverflow.Ellipsis,
                            modifier = Modifier.weight(1f),
                        )
                    }
                    Spacer(Modifier.height(8.dp))
                    Row(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                        BadgeTag(backendLabel, backendColor)
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
                            label = strings.labelJankRisk,
                            value = jankRiskPct?.let { "$it%" } ?: "--",
                            accent = jankAccent,
                            modifier = Modifier.weight(1f).fillMaxHeight(),
                        )
                        MetricPill(
                            label = strings.labelConfidence,
                            value = status?.let { "${(it.aiConfidence * 100f).toInt().coerceIn(0, 100)}%" } ?: "--",
                            accent = Color(0xFF38BDF8),
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
                            label = strings.labelWorkload,
                            value = status?.let { strings.humanizeWorkload(it.aiWorkloadClass) } ?: "--",
                            accent = Color(0xFFA78BFA),
                            modifier = Modifier.weight(1f).fillMaxHeight(),
                        )
                        MetricPill(
                            label = strings.labelTrend,
                            value = strings.formatTrend(vel),
                            accent = trendColor,
                            modifier = Modifier.weight(1f).fillMaxHeight(),
                        )
                        MetricPill(
                            label = strings.labelCpuLoad,
                            value = status?.let { "${it.cpuUtilPct.toInt().coerceIn(0, 100)}%" } ?: "--",
                            accent = Color(0xFF10B981),
                            modifier = Modifier.weight(1f).fillMaxHeight(),
                        )
                    }

                    Spacer(Modifier.height(10.dp))
                    TextButton(
                        text = strings.runSessionAnalysis,
                        onClick = {
                            if (analyzingSession) return@TextButton
                            val statusSnapshot = status
                            val isId = strings.isIndonesian
                            analyzingSession = true
                            scope.launch {
                                try {
                                    val (rep, loaded) = withContext(Dispatchers.IO) {
                                        val nluOpen = if (CoreFlowOnnxNlu.isLoaded) true else CoreFlowOnnxNlu.open(context)
                                        val report = CoreFlowOnnxNlu.analyzeSession(statusSnapshot, isId)
                                        report to nluOpen
                                    }
                                    onnxNluReady = loaded && CoreFlowOnnxNlu.isLoaded
                                    anomalyReport = rep
                                } finally {
                                    analyzingSession = false
                                }
                            }
                        },
                        enabled = status != null && !analyzingSession,
                        colors = ButtonDefaults.textButtonColorsPrimary(),
                        modifier = Modifier.fillMaxWidth(),
                    )

                    anomalyReport?.let { rep ->
                        val anomColor = when (rep.category) {
                            CoreFlowOnnxNlu.AnomalyCategory.HEALTHY -> Color(0xFF22C55E)
                            CoreFlowOnnxNlu.AnomalyCategory.THERMAL_SATURATION,
                            CoreFlowOnnxNlu.AnomalyCategory.EXTERNAL_TAMPER,
                            -> Color(0xFFEF4444)
                            else -> Color(0xFFF59E0B)
                        }
                        Spacer(Modifier.height(10.dp))
                        Column(
                            modifier = Modifier
                                .fillMaxWidth()
                                .clip(RoundedCornerShape(12.dp))
                                .background(anomColor.copy(alpha = 0.10f))
                                .border(BorderStroke(0.75.dp, anomColor.copy(alpha = 0.35f)), RoundedCornerShape(12.dp))
                                .padding(12.dp),
                        ) {
                            Row(
                                modifier = Modifier.fillMaxWidth(),
                                horizontalArrangement = Arrangement.SpaceBetween,
                                verticalAlignment = Alignment.CenterVertically,
                            ) {
                                Text(
                                    text = rep.title,
                                    style = MiuixTheme.textStyles.body2,
                                    fontWeight = FontWeight.Bold,
                                    color = anomColor,
                                    modifier = Modifier.weight(1f),
                                )
                                BadgeTag("${rep.backendName} ${(rep.confidence * 100f).toInt()}%", anomColor)
                            }
                            Spacer(Modifier.height(4.dp))
                            Text(
                                text = rep.summary,
                                style = MiuixTheme.textStyles.footnote1,
                                color = MiuixTheme.colorScheme.onSurface,
                            )
                            Spacer(Modifier.height(4.dp))
                            Text(
                                text = rep.recommendation,
                                style = MiuixTheme.textStyles.footnote1,
                                color = MiuixTheme.colorScheme.onSurfaceVariantSummary,
                            )
                        }
                    }
                }
            }

            // Phase 1-3 Physics & Silicon Intelligence Card (RC Thermal Model, Silicon EDP, p99, Causal Verification)
            if (status != null) {
                item {
                    SmallTitle(
                        text = if (strings.isIndonesian) {
                            "FISIKA TERMAL, EDP SILIKON & VERIFIKASI KAUSAL"
                        } else {
                            "THERMAL PHYSICS, SILICON EDP & CAUSAL VERIFICATION"
                        },
                    )
                }
                item {
                    val tm = status.thermalModel
                    val edp = status.siliconEdp
                    val hf = status.hyperFluid
                    val cv = status.causalVerification

                    val rcStateLabel = when {
                        tm.closedLoopActive -> if (strings.isIndonesian) "Closed-Loop Aktif" else "Closed-Loop Active"
                        tm.valid -> if (strings.isIndonesian) "Model Valid" else "Model Valid"
                        else -> if (strings.isIndonesian) "Kalibrasi (${tm.steps} step)" else "Calibrating (${tm.steps} steps)"
                    }
                    val rcColor = if (tm.closedLoopActive) Color(0xFF22C55E) else Color(0xFF38BDF8)
                    val edpColor = if (edp.calibrated) Color(0xFF06B6D4) else Color(0xFFF59E0B)
                    val budgetStr = tm.powerBudgetMw?.let { String.format(Locale.US, "%.2fW", it / 1000f) }
                        ?: tm.sustainablePowerMw?.let { String.format(Locale.US, "%.2fW", it / 1000f) }
                        ?: "--"
                    val rThetaStr = tm.rThetaCPerW?.let { String.format(Locale.US, "%.2f°/W", it) } ?: "--"
                    val tauStr = tm.tauSec?.let { "${it.toInt()}s" } ?: "--"
                    val kneeStr = if (edp.calibrated) String.format(Locale.US, "%.2fx", edp.kneeScale) else "--"
                    val alphaStr = if (edp.calibrated) String.format(Locale.US, "α %.2f", edp.effectiveExponent) else "${edp.populatedBins}/8 bin"
                    val p99Str = hf.p99FrameTimeMs?.let { String.format(Locale.US, "%.1fms", it) }
                        ?: String.format(Locale.US, "%.1fms", hf.frameBudgetMs)
                    val rqStr = if (hf.schedstatSupported) String.format(Locale.US, "%.2fms", hf.runqueueDelayMs) else "--"

                    Card(
                        modifier = Modifier.fillMaxWidth(),
                        insideMargin = PaddingValues(15.dp),
                        pressFeedbackType = PressFeedbackType.Tilt,
                        onClick = {},
                    ) {
                        Row(
                            modifier = Modifier.fillMaxWidth(),
                            horizontalArrangement = Arrangement.spacedBy(6.dp),
                        ) {
                            BadgeTag(rcStateLabel, rcColor)
                            if (tm.chassisFused) {
                                BadgeTag("Chassis Fused", Color(0xFFA78BFA))
                            }
                            BadgeTag(
                                if (edp.calibrated) "EDP ${edp.confidence}%" else "EDP Probing",
                                edpColor,
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
                                label = "P_budget",
                                value = budgetStr,
                                accent = rcColor,
                                modifier = Modifier.weight(1f).fillMaxHeight(),
                            )
                            MetricPill(
                                label = "R_θ · τ",
                                value = if (tm.rThetaCPerW != null) "$rThetaStr" else "--",
                                accent = Color(0xFFF59E0B),
                                modifier = Modifier.weight(1f).fillMaxHeight(),
                            )
                            MetricPill(
                                label = "RC Conf",
                                value = "${tm.confidence}% · $tauStr",
                                accent = Color(0xFF38BDF8),
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
                                label = "EDP Knee",
                                value = "$kneeStr · $alphaStr",
                                accent = edpColor,
                                modifier = Modifier.weight(1f).fillMaxHeight(),
                            )
                            MetricPill(
                                label = if (hf.p99Available) "p99 Frame" else "Budget",
                                value = p99Str,
                                accent = Color(0xFFA78BFA),
                                modifier = Modifier.weight(1f).fillMaxHeight(),
                            )
                            MetricPill(
                                label = "RQ Delay",
                                value = rqStr,
                                accent = Color(0xFF10B981),
                                modifier = Modifier.weight(1f).fillMaxHeight(),
                            )
                        }

                        if (cv.domains.isNotEmpty()) {
                            Spacer(Modifier.height(10.dp))
                            Text(
                                text = if (strings.isIndonesian) {
                                    "Verifikasi Kausal CFEM2 (${cv.effectiveCount} efektif · ${cv.bypassedCount} auto-bypass)"
                                } else {
                                    "CFEM2 Causal Verification (${cv.effectiveCount} effective · ${cv.bypassedCount} auto-bypassed)"
                                },
                                style = MiuixTheme.textStyles.footnote1,
                                color = MiuixTheme.colorScheme.onSurfaceVariantSummary,
                            )
                            Spacer(Modifier.height(6.dp))
                            Row(
                                modifier = Modifier
                                    .fillMaxWidth()
                                    .horizontalScroll(rememberScrollState()),
                                horizontalArrangement = Arrangement.spacedBy(6.dp),
                            ) {
                                for (dom in cv.domains) {
                                    val shortName = when (dom.key) {
                                        "thermal_uclamp_cap" -> "EAS UClamp"
                                        "thermal_gpu_cap" -> "GPU Cap"
                                        "cpu_gov_tuning" -> "CPU Gov"
                                        "semantic_cpu_gpu_boost" -> "Semantic"
                                        "memory_bus_floor" -> "MemBus"
                                        "storage_memory_tuning" -> "Storage/VM"
                                        else -> dom.key
                                    }
                                    val domColor = when (dom.state) {
                                        "effective" -> Color(0xFF22C55E)
                                        "bypassed" -> Color(0xFFF59E0B)
                                        else -> Color(0xFF38BDF8)
                                    }
                                    val icon = when (dom.state) {
                                        "effective" -> "✓"
                                        "bypassed" -> "⊘"
                                        else -> "…"
                                    }
                                    BadgeTag("$shortName $icon", domColor)
                                }
                            }
                        }
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
                        "sovereign" -> Color(0xFFEF4444)
                        "power" -> Color(0xFF10B981)
                        "thermal" -> Color(0xFFF59E0B)
                        "storage" -> Color(0xFF38BDF8)
                        "memory" -> Color(0xFF06B6D4)
                        "uclamp" -> Color(0xFFA78BFA)
                        "semantic", "copilot" -> Color(0xFF6366F1)
                        else -> MiuixTheme.colorScheme.primary
                    }
                    val outcomeColor = when (ep.outcome) {
                        "oem_yield" -> Color(0xFFF59E0B)
                        "quarantined" -> Color(0xFFF97316)
                        "rolled_back" -> Color(0xFFEF4444)
                        "skipped" -> Color(0xFF64748B)
                        else -> Color(0xFF22C55E)
                    }
                    val outcomeText = strings.humanizeOutcome(ep.outcome)
                    val tsLabel = if (ep.tsMs > 0L) timeFmt.format(Date(ep.tsMs)) else "--:--:--"

                    Card(
                        modifier = Modifier.fillMaxWidth(),
                        insideMargin = PaddingValues(13.dp),
                        pressFeedbackType = PressFeedbackType.Tilt,
                        onClick = {},
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
