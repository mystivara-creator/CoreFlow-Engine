package io.coreflow

import java.util.Locale

/**
 * Pure deterministic Telemetry-Driven Intent Dialogue Engine for CFAE System Framework / CoreFlow Engine.
 * Zero LLM overhead: parses Indonesian & English operator queries directly against live daemon telemetry,
 * tamper forensics, coupled orchestration state, and zero-wear CFDNA2 metrics.
 */
object CoreFlowDialogueEngine {

    enum class DialogueIntent {
        CONTEXT_SHIFT_GAMING,
        CONTEXT_SHIFT_BATTERY,
        CONTEXT_SHIFT_SMOOTHNESS,
        CONTEXT_SHIFT_BALANCED,
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
        val requestedProfile: CoreFlowManager.Profile? = null,
        val requestedTamperResyncNode: String? = null,
        val telemetryTags: List<String> = emptyList(),
    )

    fun quickPrompts(isIndonesian: Boolean): List<String> = if (isIndonesian) {
        listOf(
            "Kenapa barusan agak patah?",
            "Status suhu & beban sekarang?",
            "Ada konflik modul atau intervensi?",
            "Apa yang udah dipelajari di HP ini?",
            "Aku mau push rank / gaming berat",
            "Mode hemat baterai, HP lagi panas",
            "Balik ke mode otomatis seimbang",
        )
    } else {
        listOf(
            "Why did frames drop just now?",
            "Current thermal & load status?",
            "Any conflicting modules or tampering?",
            "What has CFAE learned on this device?",
            "Switch to heavy gaming mode",
            "Battery saver mode, phone feels warm",
            "Return to balanced automatic mode",
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
            containsAny(q, "push rank", "gaming berat", "mode game", "main game", "heavy gaming", "gaming mode", "switch to gaming") ->
                DialogueIntent.CONTEXT_SHIFT_GAMING
            containsAny(q, "hemat baterai", "mode hemat", "baterai awet", "lagi panas", "battery saver", "battery mode", "cool down", "phone feels warm") ->
                DialogueIntent.CONTEXT_SHIFT_BATTERY
            containsAny(q, "mode halus", "anti patah", "scroll halus", "smoothness", "fluid ui") ->
                DialogueIntent.CONTEXT_SHIFT_SMOOTHNESS
            containsAny(q, "otomatis", "seimbang", "balanced", "balik normal", "mode normal", "auto mode", "default mode") ->
                DialogueIntent.CONTEXT_SHIFT_BALANCED
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
                    "Permintaan ditolak oleh CFAE System Framework. CoreFlow dirancang sebagai penjaga sistem berdaulat — bukan alat tweaking manual. Slider frekuensi manual dan celah kompromi YIELD tidak tersedia karena dapat merusak keseimbangan termal, kurva PID, serta penjadwalan lintas-domain."
                } else {
                    "Request rejected by CFAE System Framework. CoreFlow operates as a sovereign autonomous guardian—never a manual tweaking tool. Manual frequency sliders and YIELD compromises are intentionally absent to protect thermal and scheduling equilibrium."
                }
                DialogueResolution(
                    intent = DialogueIntent.REJECT_MANUAL_TWEAK,
                    reply = reply,
                    telemetryTags = listOf("CFAE Sovereign", "Zero Compromise"),
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
            DialogueIntent.CONTEXT_SHIFT_GAMING -> {
                val tempStr = status?.socTempCelsius?.takeIf { it >= 0f }?.let { String.format(Locale.US, "%.1f°C", it) } ?: "--"
                val busNodes = status?.coupledOrchestration?.memoryBusNodes ?: 0
                val reply = if (isIndonesian) {
                    "Siap. Konteks CFAE dialihkan ke profil Gaming. Lantai UClamp per-TID, respons penjadwal, dan bus memori ($busNodes node devfreq) disiagakan untuk frame pacing stabil. Thermal Guard tetap terkunci penuh (suhu SoC saat ini $tempStr)."
                } else {
                    "Confirmed. CFAE context shifted to Gaming profile. Per-TID UClamp floor, scheduler responsiveness, and coupled memory bus ($busNodes devfreq nodes) are armed for steady frame pacing. Thermal Guard remains strictly locked (SoC currently $tempStr)."
                }
                DialogueResolution(
                    intent = DialogueIntent.CONTEXT_SHIFT_GAMING,
                    reply = reply,
                    requestedProfile = CoreFlowManager.Profile.GAMING,
                    telemetryTags = listOf("Profile -> Gaming", "Thermal Guard Locked"),
                )
            }
            DialogueIntent.CONTEXT_SHIFT_BATTERY -> {
                val tempStr = status?.socTempCelsius?.takeIf { it >= 0f }?.let { String.format(Locale.US, "%.1f°C", it) } ?: "--"
                val reply = if (isIndonesian) {
                    "Konteks CFAE dialihkan ke profil Battery. Ceiling termal & lantai UClamp diperketat agar SoC ($tempStr) segera mendingin dengan konsumsi daya minimal tanpa mengorbankan kestabilan sentuhan."
                } else {
                    "CFAE context shifted to Battery profile. Thermal ceiling and UClamp floors are tightened so SoC ($tempStr) cools rapidly with minimal power draw while keeping touch response steady."
                }
                DialogueResolution(
                    intent = DialogueIntent.CONTEXT_SHIFT_BATTERY,
                    reply = reply,
                    requestedProfile = CoreFlowManager.Profile.BATTERY,
                    telemetryTags = listOf("Profile -> Battery", "Thermal Cool"),
                )
            }
            DialogueIntent.CONTEXT_SHIFT_SMOOTHNESS -> {
                val reply = if (isIndonesian) {
                    "Konteks CFAE dialihkan ke profil Smoothness. Prioritas utama diberikan pada RenderThread, SurfaceFlinger, dan respons penjadwal untuk navigasi antarmuka yang mulus, dengan Thermal Guard tetap aktif."
                } else {
                    "CFAE context shifted to Smoothness profile. RenderThread, SurfaceFlinger, and scheduler responsiveness are prioritized for fluid UI scrolling with Thermal Guard active."
                }
                DialogueResolution(
                    intent = DialogueIntent.CONTEXT_SHIFT_SMOOTHNESS,
                    reply = reply,
                    requestedProfile = CoreFlowManager.Profile.SMOOTHNESS,
                    telemetryTags = listOf("Profile -> Smoothness", "UI Priority"),
                )
            }
            DialogueIntent.CONTEXT_SHIFT_BALANCED -> {
                val reply = if (isIndonesian) {
                    "Konteks CFAE dikembalikan ke profil Balanced (otomatis seimbang). Orkestrasi lintas-domain akan menyesuaikan headroom secara dinamis mengikuti beban real-time dan suhu perangkat."
                } else {
                    "CFAE context restored to Balanced automatic profile. Coupled multi-domain orchestration will modulate headroom dynamically based on live load and thermals."
                }
                DialogueResolution(
                    intent = DialogueIntent.CONTEXT_SHIFT_BALANCED,
                    reply = reply,
                    requestedProfile = CoreFlowManager.Profile.BALANCED,
                    telemetryTags = listOf("Profile -> Balanced", "Autonomous"),
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

        val reply = if (isIndonesian) {
            buildString {
                append("Ringkasan Termal & Beban Live (${status.mode} · ${status.effectiveProfile.label}):\n")
                append("• Suhu SoC: ${formatTemp(status.socTempCelsius)} (${formatVel(status.thermalVelocityCpm)}) · Baterai: ${formatTemp(status.batteryTempCelsius)}\n")
                append("• Beban: CPU ${status.cpuUtilPct.toInt()}% · GPU $gpuStr · Render ${(status.renderPressure * 100f).toInt()}% · Kelas: ${status.aiWorkloadClass}\n")
                append("• Headroom PID: $scalePct% · Prediksi 10d: ${status.aiPredicted10s.toInt()}% (akurasi ${(status.aiConfidence * 100f).toInt()}%, Jank Risk $jankPct%)\n")
                append("• Orkestrasi Lintas-Domain: Bus Memori $busStr (${co.memoryBusNodes} node) · Respons Sched $schedStr · Thermal Lead $leadStr")
            }
        } else {
            buildString {
                append("Live Thermal & Load Report (${status.mode} · ${status.effectiveProfile.label}):\n")
                append("• SoC Temp: ${formatTemp(status.socTempCelsius)} (${formatVel(status.thermalVelocityCpm)}) · Battery: ${formatTemp(status.batteryTempCelsius)}\n")
                append("• Utilization: CPU ${status.cpuUtilPct.toInt()}% · GPU $gpuStr · Render ${(status.renderPressure * 100f).toInt()}% · Workload: ${status.aiWorkloadClass}\n")
                append("• PID Headroom: $scalePct% · 10s Forecast: ${status.aiPredicted10s.toInt()}% (conf ${(status.aiConfidence * 100f).toInt()}%, Jank Risk $jankPct%)\n")
                append("• Coupled Orchestration: MemBus $busStr (${co.memoryBusNodes} nodes) · Sched $schedStr · Thermal Lead $leadStr")
            }
        }
        return DialogueResolution(
            intent = DialogueIntent.DIAGNOSE_THERMAL_LOAD,
            reply = reply,
            telemetryTags = listOf("SoC ${formatTemp(status.socTempCelsius)}", "Headroom $scalePct%", "Bus $busStr"),
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
        val ver = status?.version ?: "6.0.0-R"
        val mode = status?.mode ?: "OFFLINE"
        val temp = formatTemp(status?.socTempCelsius ?: -1f)
        val journalPart = journalEntryCount?.let { " · CFJ4: $it node" }.orEmpty()
        val reply = if (isIndonesian) {
            "CFAE System Framework v$ver siap ($mode · SoC $temp$journalPart). Tanyakan bebas dalam Bahasa Indonesia/Inggris soal stutter, suhu/beban, konflik modul, CFDNA2/AI, atau minta pindah konteks (Gaming / Battery / Smoothness / Balanced)."
        } else {
            "CFAE System Framework v$ver ready ($mode · SoC $temp$journalPart). Ask freely about frame drops, thermal/load status, conflicts/tampering, CFDNA2/AI engine, or request a safe context shift (Gaming / Battery / Smoothness / Balanced)."
        }
        return DialogueResolution(
            intent = DialogueIntent.GENERAL_SUMMARY,
            reply = reply,
            telemetryTags = listOf("v$ver", mode),
        )
    }

    private fun containsAny(text: String, vararg needles: String): Boolean =
        needles.any { text.contains(it) }

    private fun containsWord(text: String, word: String): Boolean =
        Regex("(?<![a-z0-9])${Regex.escape(word)}(?![a-z0-9])").containsMatchIn(text)

    private fun formatTemp(c: Float): String =
        if (c > 0f && c.isFinite()) String.format(Locale.US, "%.1f°C", c) else "--"

    private fun formatVel(v: Float): String =
        if (v.isFinite()) String.format(Locale.US, "%+.1f°C/m", v) else "0.0°C/m"

    private fun format1(v: Float): String =
        if (v.isFinite()) String.format(Locale.US, "%.1f", v) else "0.0"
}
