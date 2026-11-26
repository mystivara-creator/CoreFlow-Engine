package io.coreflow

import java.util.Locale

/**
 * Pure deterministic Telemetry-Driven Intent Dialogue Engine for CFAE System Framework / CoreFlow Engine.
 * Zero LLM overhead: parses Indonesian & English operator queries directly against live daemon telemetry,
 * tamper forensics, coupled orchestration state, and zero-wear CFDNA2 metrics.
 */
object CoreFlowDialogueEngine {

    enum class DialogueIntent {
        EXPLAIN_GAMING_AUTONOMY,
        EXPLAIN_BATTERY_COOLING,
        EXPLAIN_SMOOTHNESS_AUTONOMY,
        PAUSE_AB_COMPARE,
        RESUME_AUTONOMOUS,
        TAMPER_RESYNC,
        DIAGNOSE_STUTTER,
        DIAGNOSE_THERMAL_LOAD,
        DIAGNOSE_CONFLICT_TAMPER,
        DIAGNOSE_LEARNING_DNA,
        REJECT_MANUAL_TWEAK,
        GENERAL_SUMMARY,
    }

    data class DialogueResolution(
        val intent: DialogueIntent,
        val reply: String,
        val requestedInspectionMode: Boolean? = null,
        val requestedTamperResyncNode: String? = null,
        val telemetryTags: List<String> = emptyList(),
    )

    fun quickPrompts(isIndonesian: Boolean): List<String> = if (isIndonesian) {
        listOf(
            "Kenapa barusan agak patah?",
            "Status suhu & beban sekarang?",
            "Ada konflik modul atau intervensi?",
            "Apa yang udah dipelajari di HP ini?",
            "Siap buat gaming berat sekarang?",
            "Kendali termal EAS & baterai gimana?",
            "Pause dulu untuk bandingkan OEM",
            "Lanjutkan CoreFlow otonom",
        )
    } else {
        listOf(
            "Why did frames drop just now?",
            "Current thermal & load status?",
            "Any conflicting modules or tampering?",
            "What has CFAE learned on this device?",
            "Is autonomous 3D gaming ready?",
            "How are EAS thermal pacing & battery eco?",
            "Pause CoreFlow for OEM A/B compare",
            "Resume autonomous engine",
        )
    }

    fun evaluate(
        rawQuery: String,
        status: CoreFlowManager.DaemonStatus?,
        isIndonesian: Boolean,
        journalEntryCount: Int? = null,
    ): DialogueResolution {
        val q = rawQuery.trim().lowercase(Locale.US)
        if (q.isEmpty()) {
            return buildGeneralSummary(status, isIndonesian, journalEntryCount)
        }

        // 1. Strict Sovereign Guardrail: ALWAYS deterministic & immune to NLU overrides
        if (containsAny(q, "slider", "manual freq", "overclock", "underclock", "yield", "bypass guard", "matikan proteksi", "disable guard", "unlock 0444")) {
            return resolveIntent(DialogueIntent.REJECT_MANUAL_TWEAK, status, isIndonesian, journalEntryCount)
        }

        // 2. Fast deterministic intent matching (word-boundary safe for short tokens)
        val deterministicIntent: DialogueIntent? = when {
            containsAny(q, "resync", "enforce", "kunci ulang", "sinkron ulang", "reclaim") ->
                DialogueIntent.TAMPER_RESYNC
            containsAny(q, "pause", "jeda", "bandingkan oem", "banding oem", "ab compare", "a/b compare", "istirahatkan") ->
                DialogueIntent.PAUSE_AB_COMPARE
            containsAny(q, "lanjutkan", "aktifkan lagi", "resume", "unpause", "jalan lagi", "exit pause") ->
                DialogueIntent.RESUME_AUTONOMOUS
            containsAny(q, "push rank", "gaming berat", "mode game", "main game", "heavy gaming", "gaming mode", "3d gaming") ->
                DialogueIntent.EXPLAIN_GAMING_AUTONOMY
            containsAny(q, "hemat baterai", "mode hemat", "baterai awet", "lagi panas", "battery saver", "battery mode", "cool down", "phone feels warm", "cool-glide", "cool glide", "kendali termal", "eas cap", "thermal pacing") ->
                DialogueIntent.EXPLAIN_BATTERY_COOLING
            containsAny(q, "mode halus", "anti patah", "scroll halus", "smoothness", "fluid ui", "scrolling halus") ->
                DialogueIntent.EXPLAIN_SMOOTHNESS_AUTONOMY
            containsAny(q, "patah", "stutter", "frame drop", "jank", "lemot", "drop frame", "ngelag", "nge-lag", "lagging") ||
                containsWord(q, "lag") || containsWord(q, "fps") ->
                DialogueIntent.DIAGNOSE_STUTTER
            containsAny(q, "konflik", "modul", "tamper", "intervensi", "fkm", "forensik", "bentrok", "conflict") ->
                DialogueIntent.DIAGNOSE_CONFLICT_TAMPER
            containsAny(q, "pelajari", "belajar", "zero-wear", "learned", "adaptasi", "memori belajar", "cfdna2") ||
                containsWord(q, "dna") || containsWord(q, "flash") ->
                DialogueIntent.DIAGNOSE_LEARNING_DNA
            containsAny(q, "suhu", "panas", "beban", "temp", "thermal", "headroom", "memory bus", "bus memori") ||
                containsWord(q, "load") || containsWord(q, "cpu") || containsWord(q, "gpu") || containsWord(q, "bus") ->
                DialogueIntent.DIAGNOSE_THERMAL_LOAD
            else -> null
        }

        if (deterministicIntent != null) {
            return resolveIntent(deterministicIntent, status, isIndonesian, journalEntryCount)
        }

        // 3. Phase B On-Demand ONNX NLU fallback for free-form Indonesian/English colloquial queries
        val nlu = CoreFlowOnnxNlu.classifyIntent(rawQuery)
        if (nlu != null && nlu.confidence >= 0.42f) {
            val pct = (nlu.confidence * 100f).toInt().coerceIn(0, 100)
            val tag = "${nlu.backendName} $pct%"
            return resolveIntent(nlu.intent, status, isIndonesian, journalEntryCount, nluTag = tag)
        }

        return buildGeneralSummary(status, isIndonesian, journalEntryCount)
    }

    private fun resolveIntent(
        intent: DialogueIntent,
        status: CoreFlowManager.DaemonStatus?,
        isIndonesian: Boolean,
        journalEntryCount: Int?,
        nluTag: String? = null,
    ): DialogueResolution {
        val base = when (intent) {
            DialogueIntent.REJECT_MANUAL_TWEAK -> {
                val reply = if (isIndonesian) {
                    "Permintaan ditolak oleh CFAE System Framework. CoreFlow v6.1 beroperasi sebagai mesin otonom murni tanpa profil manual maupun slider frekuensi. Celah kompromi YIELD juga ditiadakan demi menjaga kendali daya EAS multi-cgroup dan penjadwalan lintas-domain."
                } else {
                    "Request rejected by CFAE System Framework. CoreFlow v6.1 runs as a pure Zero-Profile autonomous guardian—never a manual tweaking tool. Manual frequency sliders and YIELD compromises are intentionally absent to protect multi-cgroup EAS thermal-power pacing."
                }
                DialogueResolution(
                    intent = DialogueIntent.REJECT_MANUAL_TWEAK,
                    reply = reply,
                    telemetryTags = listOf("CFAE Sovereign", "Zero-Profile"),
                )
            }
            DialogueIntent.TAMPER_RESYNC -> {
                val locked = status?.tamperGuard?.lockedNodes ?: 0
                val reply = if (isIndonesian) {
                    "Perintah otoritatif diterima. CFAE mengeksekusi RESYNC penuh, membersihkan karantina, merebut balik seluruh node leased, dan mengunci ulang izin ke 0444 (saat ini $locked node terkunci)."
                } else {
                    "Sovereign command accepted. CFAE is executing a full RESYNC, clearing quarantine, reclaiming leased sysfs nodes, and re-locking permissions to 0444 ($locked nodes currently locked)."
                }
                DialogueResolution(
                    intent = DialogueIntent.TAMPER_RESYNC,
                    reply = reply,
                    requestedTamperResyncNode = "*",
                    telemetryTags = listOf("RESYNC", "0444 Lock"),
                )
            }
            DialogueIntent.PAUSE_AB_COMPARE -> {
                val reply = if (isIndonesian) {
                    "CoreFlow dijeda ke mode Perbandingan OEM (A/B Compare). Seluruh node kernel dan lantai UClamp dilepas bersih kembali ke bawaan pabrik agar Anda bisa membandingkan rasa layar & suhu secara objektif. Kirim \"Lanjutkan\" kapan saja untuk mengaktifkan mesin otonom lagi."
                } else {
                    "CoreFlow paused for Stock OEM A/B Comparison. All kernel nodes and UClamp floors are cleanly rolled back to factory defaults so you can compare smoothness and thermals objectively. Send \"Resume\" anytime to re-engage the autonomous loop."
                }
                DialogueResolution(
                    intent = DialogueIntent.PAUSE_AB_COMPARE,
                    reply = reply,
                    requestedInspectionMode = true,
                    telemetryTags = listOf("Paused · OEM A/B", "Baseline Restored"),
                )
            }
            DialogueIntent.RESUME_AUTONOMOUS -> {
                val reply = if (isIndonesian) {
                    "Mesin Otonom CoreFlow diaktifkan kembali. Deteksi 3D Gaming otomatis, Kendali Termal EAS multi-cgroup, sentuhan <5ms, dan App DNA kembali mengendalikan perangkat secara real-time."
                } else {
                    "CoreFlow Unified Autonomous Engine resumed. Automatic 3D Gaming detection, multi-cgroup EAS thermal pacing, <5ms touch response, and App DNA are actively pacing the device in real time."
                }
                DialogueResolution(
                    intent = DialogueIntent.RESUME_AUTONOMOUS,
                    reply = reply,
                    requestedInspectionMode = false,
                    telemetryTags = listOf("Autonomous Active", "EAS Pacing"),
                )
            }
            DialogueIntent.EXPLAIN_GAMING_AUTONOMY -> {
                val tempStr = formatTemp(status?.socTempCelsius ?: -1f)
                val scalePct = ((status?.capacityScale ?: 1f) * 100f).toInt().coerceIn(0, 100)
                val busNodes = status?.coupledOrchestration?.memoryBusNodes ?: 0
                val bypassNote = if (status?.batteryPower?.bypassSupported == true) {
                    if (isIndonesian) {
                        if (status.powerConfig.bypassOnGaming || status.batteryPower.bypassActive) " · Bypass Charging siap/aktif"
                        else " · Bypass Charging tersedia di tab Sistem"
                    } else {
                        if (status.powerConfig.bypassOnGaming || status.batteryPower.bypassActive) " · Bypass Charging armed/active"
                        else " · Bypass Charging available in System tab"
                    }
                } else ""
                val reply = if (isIndonesian) {
                    "Tidak perlu pindah profil manual — CoreFlow v6.1 otomatis mendeteksi game 3D dari thread engine (UnityMain/GameThread/Vulkan) dan beban GPU. Lantai UClamp per-TID serta bus memori ($busNodes node) langsung naik otomatis saat game berjalan, sementara Kendali Termal EAS & Thermal Guard menjaga suhu stabil (SoC $tempStr · Headroom $scalePct%$bypassNote)."
                } else {
                    "No manual profile switch needed—CoreFlow v6.1 automatically detects 3D games from engine threads (UnityMain/GameThread/Vulkan) and sustained GPU load. Per-TID UClamp and coupled memory bus ($busNodes nodes) scale up automatically while EAS Thermal Pacing & Thermal Guard protect thermals (SoC $tempStr · Headroom $scalePct%$bypassNote)."
                }
                DialogueResolution(
                    intent = DialogueIntent.EXPLAIN_GAMING_AUTONOMY,
                    reply = reply,
                    telemetryTags = listOf("Auto 3D Gaming", "Headroom $scalePct%", "SoC $tempStr"),
                )
            }
            DialogueIntent.EXPLAIN_BATTERY_COOLING -> {
                val tempStr = formatTemp(status?.socTempCelsius ?: -1f)
                val battTempStr = formatTemp(status?.batteryTempCelsius ?: -1f)
                val battPct = status?.batteryPower?.capacityPct
                val ecoActive = status?.lowBatteryEco == true
                val easCap = (status?.easTopAppCapPct ?: 100).coerceIn(20, 100)
                val easCapStr = if (easCap < 100) "$easCap%" else "Max"
                val scalePct = ((status?.capacityScale ?: 1f) * 100f).toInt().coerceIn(0, 100)
                val reply = if (isIndonesian) {
                    val ecoStatus = if (ecoActive) {
                        "Low-Battery Auto-Eco sedang AKTIF (baterai ${battPct ?: "--"}%): memangkas lonjakan daya puncak & hold fling 500ms namun scroll tetap mulus."
                    } else {
                        "Low-Battery Auto-Eco otomatis aktif saat baterai ≤20% tanpa charger (saat ini ${battPct ?: "--"}%)."
                    }
                    "CoreFlow menggunakan Kendali Termal EAS multi-cgroup nyata (tanpa profil manual): saat suhu bodi menghangat (baterai $battTempStr · SoC $tempStr), CoreFlow membatasi puncak tegangan Big-core via cpu.uclamp.max (EAS Cap $easCapStr · Headroom $scalePct%) dan mengunci background di klaster Little, lalu langsung melepas batas saat buka app atau terjadi jank. $ecoStatus"
                } else {
                    val ecoStatus = if (ecoActive) {
                        "Low-Battery Auto-Eco is ACTIVE (battery ${battPct ?: "--"}%): trimming peak voltage spikes with 500ms fling hold while keeping scrolling smooth."
                    } else {
                        "Low-Battery Auto-Eco engages automatically whenever battery ≤20% off-charger (currently ${battPct ?: "--"}%)."
                    }
                    "CoreFlow runs real multi-cgroup EAS thermal-power pacing (no manual battery profile): when the chassis warms (battery $battTempStr · SoC $tempStr), CoreFlow trims the steep Big-core voltage knee via cpu.uclamp.max (EAS Cap $easCapStr · Headroom $scalePct%) and pins background tasks to Little cores, lifting immediately on app launch or jank. $ecoStatus"
                }
                DialogueResolution(
                    intent = DialogueIntent.EXPLAIN_BATTERY_COOLING,
                    reply = reply,
                    telemetryTags = listOf(if (ecoActive) "Auto-Eco Active" else "EAS Cap $easCapStr", "Batt $battTempStr"),
                )
            }
            DialogueIntent.EXPLAIN_SMOOTHNESS_AUTONOMY -> {
                val hz = status?.hardware?.refreshHz ?: 60
                val holdMs = when {
                    status?.lowBatteryEco == true -> 500
                    hz >= 115 -> 950
                    hz >= 85 -> 850
                    else -> 750
                }
                val reply = if (isIndonesian) {
                    "Kehalusan layar aktif otomatis 100% tanpa perlu profil Smoothness. Jalur sentuh <5ms langsung mengangkat RenderThread & SurfaceFlinger dengan OverScroller fling hold adaptif ${holdMs}ms (panel ${hz}Hz) serta Jank Rescue otomatis jika frame drop ≥3%."
                } else {
                    "UI fluidity runs autonomously 100% of the time—no manual Smoothness profile needed. The <5ms touch path boosts RenderThread & SurfaceFlinger with refresh-adaptive ${holdMs}ms OverScroller fling hold (${hz}Hz panel) plus automatic Jank Rescue at ≥3% frame drops."
                }
                DialogueResolution(
                    intent = DialogueIntent.EXPLAIN_SMOOTHNESS_AUTONOMY,
                    reply = reply,
                    telemetryTags = listOf("<5ms Touch", "${hz}Hz · ${holdMs}ms Fling"),
                )
            }
            DialogueIntent.DIAGNOSE_STUTTER -> buildStutterDiagnosis(status, isIndonesian)
            DialogueIntent.DIAGNOSE_THERMAL_LOAD -> buildThermalAndLoadDiagnosis(status, isIndonesian)
            DialogueIntent.DIAGNOSE_CONFLICT_TAMPER -> buildConflictAndTamperDiagnosis(status, isIndonesian)
            DialogueIntent.DIAGNOSE_LEARNING_DNA -> buildLearningDiagnosis(status, isIndonesian)
            DialogueIntent.GENERAL_SUMMARY -> buildGeneralSummary(status, isIndonesian, journalEntryCount)
        }
        return if (nluTag != null) {
            base.copy(telemetryTags = (listOf(nluTag) + base.telemetryTags).take(3))
        } else {
            base
        }
    }

    private fun buildStutterDiagnosis(
        status: CoreFlowManager.DaemonStatus?,
        isIndonesian: Boolean,
    ): DialogueResolution {
        if (status == null || !status.isRunning) {
            val msg = if (isIndonesian) {
                "Daemon coreflowd belum berjalan sehingga telemetri frame pacing tidak tersedia. Jalankan CFAE Engine dari tab Beranda terlebih dahulu."
            } else {
                "coreflowd is currently offline, so live frame pacing telemetry is unavailable. Start CFAE Engine from the Home tab first."
            }
            return DialogueResolution(DialogueIntent.DIAGNOSE_STUTTER, msg, telemetryTags = listOf("Offline"))
        }

        val fps = status.hyperFluid.measuredFps
        val hz = status.hardware.refreshHz
        val jankPct = status.hyperFluid.jankRatio?.let { (it * 100f).coerceIn(0f, 100f) }
        val aiJankPct = (status.aiJankRisk * 100f).toInt().coerceIn(0, 100)
        val scalePct = (status.capacityScale * 100f).toInt().coerceIn(0, 100)
        val cpuPsi = status.cpuPsi ?: 0f
        val memPsi = status.memPsi ?: 0f
        val ioPsi = status.ioPsi ?: 0f

        val rootCause = when {
            status.bootSettleActive -> if (isIndonesian) {
                "Perangkat masih berada pada fase tenang BOOT_SETTLE (${status.bootSettleDurationSec.toInt()}d) untuk mencegah lonjakan suhu pasca-boot."
            } else {
                "Device is still in BOOT_SETTLE sanctuary (${status.bootSettleDurationSec.toInt()}s) to prevent post-boot thermal spikes."
            }
            status.thermalGuardActive || status.capacityScale < 0.85f -> if (isIndonesian) {
                "Thermal Guard / PID sedang membatasi headroom ke $scalePct% (SoC ${formatTemp(status.socTempCelsius)}, tren ${formatVel(status.thermalVelocityCpm)}, AI Jank Risk $aiJankPct%) demi mencegah thermal cliff."
            } else {
                "Thermal Guard / PID is actively pacing headroom to $scalePct% (SoC ${formatTemp(status.socTempCelsius)}, trend ${formatVel(status.thermalVelocityCpm)}, AI Jank Risk $aiJankPct%) to prevent a thermal cliff."
            }
            status.tamperGuard.totalEvents > 0L -> if (isIndonesian) {
                "Terdeteksi ${status.tamperGuard.totalEvents} intervensi eksternal pada node kernel yang sempat mengganggu ritme CFAE; Sovereign Guard telah mengambil alih."
            } else {
                "Detected ${status.tamperGuard.totalEvents} external kernel node tamper event(s) disrupting CFAE cadence; Sovereign Guard has reclaimed ownership."
            }
            memPsi >= 10f || ioPsi >= 10f -> if (isIndonesian) {
                "Tekanan antrean memori/I/O tinggi (Mem PSI ${format1(memPsi)}%, IO PSI ${format1(ioPsi)}%); orkestrasi bus memori & read-ahead sedang membantu menyuplai aset."
            } else {
                "Elevated memory/I/O queue stall (Mem PSI ${format1(memPsi)}%, IO PSI ${format1(ioPsi)}%); coupled memory bus & read-ahead are boosting supply."
            }
            cpuPsi >= 12f -> if (isIndonesian) {
                "Antrean CPU meningkat (CPU PSI ${format1(cpuPsi)}%, beban ${status.cpuUtilPct.toInt()}%); lantai UClamp per-TID (${status.uclampMin}..${status.uclampMax}) sedang menyesuaikan."
            } else {
                "CPU contention rose (CPU PSI ${format1(cpuPsi)}%, util ${status.cpuUtilPct.toInt()}%); per-TID UClamp (${status.uclampMin}..${status.uclampMax}) is compensating."
            }
            else -> if (isIndonesian) {
                "Tidak ada bottleneck termal atau PSI berat saat ini (Headroom $scalePct%, AI Jank Risk $aiJankPct%, CPU PSI ${format1(cpuPsi)}%). Jika sempat terasa patah sesaat, kemungkinan berasal dari kompilasi shader atau GC aplikasi ${status.foregroundPackage.ifBlank { "latar depan" }}."
            } else {
                "No severe thermal or PSI bottleneck is active right now (Headroom $scalePct%, AI Jank Risk $aiJankPct%, CPU PSI ${format1(cpuPsi)}%). Brief hitching was likely shader compilation or app GC in ${status.foregroundPackage.ifBlank { "foreground app" }}."
            }
        }

        val fpsSummary = if (fps > 0f && jankPct != null) {
            "FPS ${format1(fps)}/$hz Hz · Jank ${format1(jankPct)}%"
        } else {
            "Panel ${hz}Hz · Render ${(status.renderPressure * 100f).toInt()}%"
        }

        val reply = if (isIndonesian) {
            "Analisis Frame Pacing ($fpsSummary):\n$rootCause"
        } else {
            "Frame Pacing Diagnosis ($fpsSummary):\n$rootCause"
        }
        return DialogueResolution(
            intent = DialogueIntent.DIAGNOSE_STUTTER,
            reply = reply,
            telemetryTags = listOf(fpsSummary, "Headroom $scalePct%"),
        )
    }

    private fun buildThermalAndLoadDiagnosis(
        status: CoreFlowManager.DaemonStatus?,
        isIndonesian: Boolean,
    ): DialogueResolution {
        if (status == null) {
            val msg = if (isIndonesian) "Telemetri belum tersedia." else "Telemetry unavailable."
            return DialogueResolution(DialogueIntent.DIAGNOSE_THERMAL_LOAD, msg)
        }
        val scalePct = (status.capacityScale * 100f).toInt().coerceIn(0, 100)
        val co = status.coupledOrchestration
        val gpuStr = status.gpuBusyPct?.let { "${it.toInt()}%" } ?: "N/A"
        val busStr = String.format(Locale.US, "%.2fx", co.memoryBusScale)
        val schedStr = String.format(Locale.US, "%.2fx", co.schedulerResponsivenessScale)
        val leadStr = "${(co.thermalStepdownLead * 100f).toInt()}%"
        val jankPct = (status.aiJankRisk * 100f).toInt().coerceIn(0, 100)

        val regimeLabel = status.autonomousRegimeLabel(isIndonesian)
        val tm = status.thermalModel
        val edp = status.siliconEdp
        val cv = status.causalVerification
        val budgetStr = tm.powerBudgetMw?.let { String.format(Locale.US, "%.2fW", it / 1000f) } ?: "--"
        val rThetaStr = tm.rThetaCPerW?.let { String.format(Locale.US, "%.2f°C/W", it) } ?: "--"
        val edpStr = if (edp.calibrated) {
            String.format(Locale.US, "Knee %.2fx · α %.2f (%d%%)", edp.kneeScale, edp.effectiveExponent, edp.confidence)
        } else {
            "Probing (${edp.populatedBins}/8 bin)"
        }
        val reply = if (isIndonesian) {
            buildString {
                append("Ringkasan Termal & Beban Live (${status.mode} · $regimeLabel):\n")
                append("• Suhu SoC: ${formatTemp(status.socTempCelsius)} (${formatVel(status.thermalVelocityCpm)}) · Baterai: ${formatTemp(status.batteryTempCelsius)}\n")
                append("• Model RC Termal: ${if (tm.closedLoopActive) "Closed-Loop" else "Kalibrasi"} (${tm.confidence}%) · P_budget $budgetStr · R_θ $rThetaStr\n")
                append("• Silicon EDP (CFEDP2): $edpStr · Kausal (CFEM2): ${cv.effectiveCount} efektif / ${cv.bypassedCount} bypass\n")
                append("• Beban: CPU ${status.cpuUtilPct.toInt()}% · GPU $gpuStr · Render ${(status.renderPressure * 100f).toInt()}% · Kelas: ${status.aiWorkloadClass}\n")
                append("• Headroom PID: $scalePct% · Prediksi 10d: ${status.aiPredicted10s.toInt()}% (akurasi ${(status.aiConfidence * 100f).toInt()}%, Jank Risk $jankPct%)\n")
                append("• Orkestrasi Lintas-Domain: Bus Memori $busStr (${co.memoryBusNodes} node) · Respons Sched $schedStr · Thermal Lead $leadStr")
            }
        } else {
            buildString {
                append("Live Thermal & Load Report (${status.mode} · $regimeLabel):\n")
                append("• SoC Temp: ${formatTemp(status.socTempCelsius)} (${formatVel(status.thermalVelocityCpm)}) · Battery: ${formatTemp(status.batteryTempCelsius)}\n")
                append("• RC Thermal Model: ${if (tm.closedLoopActive) "Closed-Loop" else "Calibrating"} (${tm.confidence}%) · P_budget $budgetStr · R_θ $rThetaStr\n")
                append("• Silicon EDP (CFEDP2): $edpStr · Causal (CFEM2): ${cv.effectiveCount} effective / ${cv.bypassedCount} bypassed\n")
                append("• Utilization: CPU ${status.cpuUtilPct.toInt()}% · GPU $gpuStr · Render ${(status.renderPressure * 100f).toInt()}% · Workload: ${status.aiWorkloadClass}\n")
                append("• PID Headroom: $scalePct% · 10s Forecast: ${status.aiPredicted10s.toInt()}% (conf ${(status.aiConfidence * 100f).toInt()}%, Jank Risk $jankPct%)\n")
                append("• Coupled Orchestration: MemBus $busStr (${co.memoryBusNodes} nodes) · Sched $schedStr · Thermal Lead $leadStr")
            }
        }
        return DialogueResolution(
            intent = DialogueIntent.DIAGNOSE_THERMAL_LOAD,
            reply = reply,
            telemetryTags = listOf("SoC ${formatTemp(status.socTempCelsius)}", "Budget $budgetStr", "Headroom $scalePct%"),
        )
    }

    private fun buildConflictAndTamperDiagnosis(
        status: CoreFlowManager.DaemonStatus?,
        isIndonesian: Boolean,
    ): DialogueResolution {
        if (status == null) {
            val msg = if (isIndonesian) "Status konflik belum tersedia." else "Conflict status unavailable."
            return DialogueResolution(DialogueIntent.DIAGNOSE_CONFLICT_TAMPER, msg)
        }
        val conflicts = status.conflictingModules
        val tg = status.tamperGuard
        val latest = status.tamperEvents.lastOrNull()

        val reply = if (isIndonesian) {
            buildString {
                append("Laporan Forensik Sovereign Guard & Konflik Modul:\n")
                if (conflicts.isEmpty()) {
                    append("• Modul Pesaing: Bersih (0 modul pengganggu aktif terdeteksi).\n")
                } else {
                    append("• Modul Konflik Terdeteksi (${conflicts.size}): ${conflicts.joinToString(", ")} — nonaktifkan segera agar tidak bentrok dengan CFAE.\n")
                }
                append("• Node Terkunci 0444: ${tg.lockedNodes} · Karantina Circuit Breaker: ${tg.quarantinedNodes} · Total Insiden: ${tg.totalEvents}\n")
                append("• Kolaborasi OEM Pasif: ${status.oemYieldedNodes} node (aman dari bentrok perf-hal/mi_thermald).\n")
                if (latest != null) {
                    val leaf = latest.nodePath.substringAfterLast('/')
                    append("• Insiden Terakhir: [$leaf] dicoba=${latest.observedValue} → ditegakkan CFAE=${latest.expectedValue} ")
                    append(if (latest.circuitBreakerOpen) "(Breaker 3x/5d aktif)" else if (latest.reclaimed) "(Direbut balik)" else "(Dipantau)")
                    append(". Kirim \"RESYNC\" untuk sinkronisasi paksa.")
                } else {
                    append("• Belum ada intervensi eksternal (FKM/terminal) yang terdeteksi pada sesi ini.")
                }
            }
        } else {
            buildString {
                append("Sovereign Guard & Module Conflict Forensics:\n")
                if (conflicts.isEmpty()) {
                    append("• Competing Modules: Clean (0 active conflicting modules detected).\n")
                } else {
                    append("• Conflicting Modules (${conflicts.size}): ${conflicts.joinToString(", ")} — remove/disable them immediately.\n")
                }
                append("• 0444 Locked Nodes: ${tg.lockedNodes} · Breaker Quarantine: ${tg.quarantinedNodes} · Total Events: ${tg.totalEvents}\n")
                append("• Passive OEM Co-op: ${status.oemYieldedNodes} nodes (safe from perf-hal/mi_thermald fights).\n")
                if (latest != null) {
                    val leaf = latest.nodePath.substringAfterLast('/')
                    append("• Latest Incident: [$leaf] attempted=${latest.observedValue} → enforced=${latest.expectedValue} ")
                    append(if (latest.circuitBreakerOpen) "(3x/5s Breaker Open)" else if (latest.reclaimed) "(Reclaimed)" else "(Monitored)")
                    append(". Send \"RESYNC\" to force re-sync.")
                } else {
                    append("• Zero external user tampering (FKM/terminal) detected this session.")
                }
            }
        }
        return DialogueResolution(
            intent = DialogueIntent.DIAGNOSE_CONFLICT_TAMPER,
            reply = reply,
            telemetryTags = listOf("0444: ${tg.lockedNodes}", "Tamper: ${tg.totalEvents}", "Conflicts: ${conflicts.size}"),
        )
    }

    private fun buildLearningDiagnosis(
        status: CoreFlowManager.DaemonStatus?,
        isIndonesian: Boolean,
    ): DialogueResolution {
        if (status == null) {
            val msg = if (isIndonesian) "Data pembelajaran belum tersedia." else "Learning telemetry unavailable."
            return DialogueResolution(DialogueIntent.DIAGNOSE_LEARNING_DNA, msg)
        }
        val hf = status.hyperFluid
        val biasStr = String.format(Locale.US, "%.2fx", hf.activeAppBias)
        val effStr = String.format(Locale.US, "%.2fx", hf.activeAppEfficiencyWeight)

        val reply = if (isIndonesian) {
            buildString {
                append("Status Pembelajaran Konteks Zero-Wear (CFDNA2) & Neural Engine:\n")
                append("• Mesin AI Daemon: ${status.aiBackend} · Kelas Beban: ${status.aiWorkloadClass}.\n")
                append("• DNA Aplikasi Dipelajari: ${hf.appDnaLearned} paket (kalkulasi cost-to-gain berjalan di RAM).\n")
                append("• Aplikasi Aktif: Bias UClamp $biasStr · Bobot Efisiensi $effStr.\n")
                append("• Ketahanan Flash (Zero-Wear): ${hf.appDnaFlushCount} kali flush atomik (hanya saat ScreenOff ≥15m & ada drift signifikan).\n")
                append("• Prediksi & Memori: ${hf.predictivePreloads} preload Markov · ${hf.idleCompactions} idle compaction · ${status.semanticKnobsDiscovered} knob semantik.")
            }
        } else {
            buildString {
                append("Zero-Wear On-Device Context Learning (CFDNA2) & Neural Engine:\n")
                append("• Daemon AI Backend: ${status.aiBackend} · Workload Class: ${status.aiWorkloadClass}.\n")
                append("• Learned App DNA: ${hf.appDnaLearned} packages (cost-to-gain evaluated continuously in RAM).\n")
                append("• Active App: UClamp Bias $biasStr · Efficiency Weight $effStr.\n")
                append("• Flash Longevity (Zero-Wear): ${hf.appDnaFlushCount} atomic flushes (ScreenOff ≥15m & significant drift only).\n")
                append("• Prediction & Memory: ${hf.predictivePreloads} Markov preloads · ${hf.idleCompactions} idle compactions · ${status.semanticKnobsDiscovered} semantic knobs.")
            }
        }
        return DialogueResolution(
            intent = DialogueIntent.DIAGNOSE_LEARNING_DNA,
            reply = reply,
            telemetryTags = listOf("DNA: ${hf.appDnaLearned}", status.aiBackend, "Eff $effStr"),
        )
    }

    private fun buildGeneralSummary(
        status: CoreFlowManager.DaemonStatus?,
        isIndonesian: Boolean,
        journalEntryCount: Int?,
    ): DialogueResolution {
        val ver = status?.version ?: "6.1.1"
        val regime = status?.autonomousRegimeLabel(isIndonesian) ?: if (isIndonesian) "Offline" else "Offline"
        val temp = formatTemp(status?.socTempCelsius ?: -1f)
        val journalPart = journalEntryCount?.let { " · CFJ4: $it node" }.orEmpty()
        val reply = if (isIndonesian) {
            "CFAE System Framework v$ver siap ($regime · SoC $temp$journalPart). Semua berjalan 100% otonom tanpa profil manual. Tanyakan soal stutter, suhu/beban, konflik modul, App DNA, atau ketik \"Pause\" / \"Lanjutkan\" untuk perbandingan OEM."
        } else {
            "CFAE System Framework v$ver ready ($regime · SoC $temp$journalPart). Running 100% Zero-Profile autonomous. Ask about frame drops, thermals/load, conflicts/tampering, App DNA, or type \"Pause\" / \"Resume\" for Stock OEM A/B compare."
        }
        return DialogueResolution(
            intent = DialogueIntent.GENERAL_SUMMARY,
            reply = reply,
            telemetryTags = listOf("v$ver", regime),
        )
    }

    private val wordRegexCache = java.util.concurrent.ConcurrentHashMap<String, Regex>()

    private fun containsAny(text: String, vararg needles: String): Boolean =
        needles.any { text.contains(it) }

    private fun containsWord(text: String, word: String): Boolean {
        val regex = wordRegexCache.computeIfAbsent(word) {
            Regex("(?<![a-z0-9])${Regex.escape(it)}(?![a-z0-9])")
        }
        return regex.containsMatchIn(text)
    }

    private fun formatTemp(c: Float): String =
        if (c > 0f && c.isFinite()) String.format(Locale.US, "%.1f°C", c) else "--"

    private fun formatVel(v: Float): String =
        if (v.isFinite()) String.format(Locale.US, "%+.1f°C/m", v) else "0.0°C/m"

    private fun format1(v: Float): String =
        if (v.isFinite()) String.format(Locale.US, "%.1f", v) else "0.0"
}
