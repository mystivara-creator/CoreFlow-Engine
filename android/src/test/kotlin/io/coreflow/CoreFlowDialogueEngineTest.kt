package io.coreflow

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

class CoreFlowDialogueEngineTest {

    private fun sampleStatus(
        thermalGuard: Boolean = false,
        tamperCount: Long = 0L,
        conflicts: List<String> = emptyList(),
    ): CoreFlowManager.DaemonStatus = CoreFlowManager.DaemonStatus(
        version = "6.0.0-R",
        isRunning = true,
        pid = 4242,
        profile = CoreFlowManager.Profile.BALANCED,
        effectiveProfile = CoreFlowManager.Profile.BALANCED,
        mode = if (thermalGuard) "THERMAL_GUARD" else "INTERACTIVE",
        inspectionMode = false,
        foregroundPackage = "com.miHoYo.GenshinImpact",
        uclampMin = 384,
        uclampMax = 922,
        uclampSupported = true,
        boostedTids = 4,
        storageBurstActive = false,
        readAheadKb = 512,
        prereadBytes = 0L,
        managedEnvelopeNodes = 4,
        semanticKnobsDiscovered = 18,
        semanticKnobsActive = 5,
        memoryBusKnobs = 2,
        contendedPaths = emptyList(),
        socTempCelsius = 43.5f,
        batteryTempCelsius = 36.2f,
        capacityScale = if (thermalGuard) 0.72f else 0.96f,
        cpuUtilPct = 42f,
        renderPressure = 0.68f,
        gpuBusyPct = 58f,
        cpuPsi = 8.5f,
        memPsi = 3.2f,
        ioPsi = 1.4f,
        aiPredicted10s = 52f,
        aiConfidence = 0.86f,
        baselineNodesSnapshotted = 5,
        recoveryPending = true,
        thermalSource = "oem_policy",
        thermalVelocityCpm = 0.8f,
        thermalGuardActive = thermalGuard,
        hyperFluid = CoreFlowManager.HyperFluidStats(
            framePacingAvailable = true,
            measuredFps = 59.4f,
            jankRatio = 0.01f,
            appDnaLearned = 24,
            activeAppBias = 1.08f,
            activeAppEfficiencyWeight = 1.04f,
            appDnaFlushCount = 3L,
        ),
        tamperGuard = CoreFlowManager.TamperGuardStats(
            enabled = true,
            lockedNodes = 5,
            quarantinedNodes = 0,
            totalEvents = tamperCount,
        ),
        tamperEvents = if (tamperCount > 0L) {
            listOf(
                CoreFlowManager.TamperEventInfo(
                    id = 1L,
                    tsMs = 1700000000000L,
                    nodePath = "/sys/class/kgsl/kgsl-3d0/adrenoboost",
                    domain = "gpu_devfreq",
                    expectedValue = "2",
                    observedValue = "0",
                    reason = "CFAE Sovereign Thermal & Scheduling Equilibrium",
                    reclaimed = true,
                    circuitBreakerOpen = false,
                    attemptsInWindow = 1,
                ),
            )
        } else {
            emptyList()
        },
        coupledOrchestration = CoreFlowManager.CoupledOrchestrationStats(
            regime = 2,
            memoryBusScale = 1.18f,
            schedulerResponsivenessScale = 0.82f,
            thermalStepdownLead = 0.08f,
            couplingMask = 7,
            memoryBusNodes = 2,
        ),
        conflictingModules = conflicts,
    )

    @Test
    fun indonesianAndEnglishAutonomyAndPauseResumeIntentsWorkCleanly() {
        val status = sampleStatus()
        val gaming = CoreFlowDialogueEngine.evaluate("Aku mau push rank / gaming berat", status, true)
        assertEquals(CoreFlowDialogueEngine.DialogueIntent.EXPLAIN_GAMING_AUTONOMY, gaming.intent)
        assertNull(gaming.requestedInspectionMode)
        assertTrue(gaming.reply.contains("Thermal Guard"))

        val battery = CoreFlowDialogueEngine.evaluate("Mode hemat baterai, HP lagi panas", status, true)
        assertEquals(CoreFlowDialogueEngine.DialogueIntent.EXPLAIN_BATTERY_COOLING, battery.intent)
        assertNull(battery.requestedInspectionMode)
        assertTrue(battery.reply.contains("Kendali Termal EAS"))

        val pause = CoreFlowDialogueEngine.evaluate("Pause dulu untuk bandingkan OEM", status, true)
        assertEquals(CoreFlowDialogueEngine.DialogueIntent.PAUSE_AB_COMPARE, pause.intent)
        assertEquals(true, pause.requestedInspectionMode)

        val resume = CoreFlowDialogueEngine.evaluate("Resume autonomous engine", status, false)
        assertEquals(CoreFlowDialogueEngine.DialogueIntent.RESUME_AUTONOMOUS, resume.intent)
        assertEquals(false, resume.requestedInspectionMode)
    }

    @Test
    fun stutterAndThermalDiagnosticsReflectLiveTelemetry() {
        val hotStatus = sampleStatus(thermalGuard = true)
        val stutter = CoreFlowDialogueEngine.evaluate("Kenapa barusan agak patah?", hotStatus, true)
        assertEquals(CoreFlowDialogueEngine.DialogueIntent.DIAGNOSE_STUTTER, stutter.intent)
        assertTrue(stutter.reply.contains("72%"))
        assertTrue(stutter.reply.contains("43.5°C"))

        val thermal = CoreFlowDialogueEngine.evaluate("Status suhu & beban sekarang?", hotStatus, true)
        assertEquals(CoreFlowDialogueEngine.DialogueIntent.DIAGNOSE_THERMAL_LOAD, thermal.intent)
        assertTrue(thermal.reply.contains("1.18x"))
        assertTrue(thermal.reply.contains("0.82x"))
    }

    @Test
    fun tamperForensicsAndConflictDiagnosticsReportExactEvents() {
        val tampered = sampleStatus(tamperCount = 2L, conflicts = listOf("Encore Tweaks (encore_tweaks)"))
        val report = CoreFlowDialogueEngine.evaluate("Ada konflik modul atau intervensi?", tampered, true)
        assertEquals(CoreFlowDialogueEngine.DialogueIntent.DIAGNOSE_CONFLICT_TAMPER, report.intent)
        assertTrue(report.reply.contains("encore_tweaks"))
        assertTrue(report.reply.contains("adrenoboost"))
        assertTrue(report.reply.contains("0444"))

        val resync = CoreFlowDialogueEngine.evaluate("RESYNC", tampered, true)
        assertEquals(CoreFlowDialogueEngine.DialogueIntent.TAMPER_RESYNC, resync.intent)
        assertEquals("*", resync.requestedTamperResyncNode)
    }

    @Test
    fun sovereignGuardrailRejectsManualSlidersAndYieldBypass() {
        val status = sampleStatus()
        val refused = CoreFlowDialogueEngine.evaluate("Kasih slider manual freq atau yield guard", status, true)
        assertEquals(CoreFlowDialogueEngine.DialogueIntent.REJECT_MANUAL_TWEAK, refused.intent)
        assertNull(refused.requestedInspectionMode)
        assertNull(refused.requestedTamperResyncNode)
        assertTrue(refused.reply.contains("ditolak", ignoreCase = true))
    }

    @Test
    fun zeroWearLearningDiagnosisReportsCfdna2Metrics() {
        val status = sampleStatus()
        val dna = CoreFlowDialogueEngine.evaluate("Apa yang udah kamu pelajari di HP ini?", status, true)
        assertEquals(CoreFlowDialogueEngine.DialogueIntent.DIAGNOSE_LEARNING_DNA, dna.intent)
        assertTrue(dna.reply.contains("24"))
        assertTrue(dna.reply.contains("CFDNA2"))
        assertTrue(dna.reply.contains("3"))
    }
}
