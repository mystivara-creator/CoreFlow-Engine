package io.coreflow

import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test
import java.io.File
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.util.zip.CRC32

class CoreFlowOnnxNluTest {

    @After
    fun tearDown() {
        CoreFlowOnnxNlu.close()
    }

    private fun readAsset(name: String): ByteArray {
        val candidates = listOf(
            File("src/main/assets/$name"),
            File("android/src/main/assets/$name"),
        )
        val file = candidates.firstOrNull { it.isFile }
            ?: error("Missing ONNX asset $name from cwd=${File(".").absolutePath}")
        return file.readBytes()
    }

    private fun openRealAssets() {
        val ok = CoreFlowOnnxNlu.openFromBytes(
            nluBytes = readAsset(CoreFlowOnnxNlu.NLU_ASSET),
            anomalyBytes = readAsset(CoreFlowOnnxNlu.ANOMALY_ASSET),
        )
        assertTrue("ONNX assets must open cleanly", ok)
    }

    private fun statusSnapshot(
        socTemp: Float = 36f,
        battTemp: Float = 33f,
        velCpm: Float = 0.1f,
        cpuUtil: Float = 24f,
        jankRatio: Float = 0.02f,
        aiJankRisk: Float = 0.05f,
        thermalGuard: Boolean = false,
        charging: Boolean = false,
        powerMw: Float = 0f,
        tamperEvents: Long = 0L,
        conflicts: List<String> = emptyList(),
    ): CoreFlowManager.DaemonStatus = CoreFlowManager.DaemonStatus(
        version = "6.0.0-R",
        isRunning = true,
        pid = 1337,
        profile = CoreFlowManager.Profile.BALANCED,
        effectiveProfile = CoreFlowManager.Profile.BALANCED,
        mode = if (thermalGuard) "THERMAL_GUARD" else "INTERACTIVE",
        inspectionMode = false,
        foregroundPackage = "com.example.app",
        uclampMin = 256,
        uclampMax = 896,
        uclampSupported = true,
        boostedTids = 2,
        storageBurstActive = false,
        readAheadKb = 256,
        prereadBytes = 0L,
        managedEnvelopeNodes = 4,
        contendedPaths = emptyList(),
        socTempCelsius = socTemp,
        batteryTempCelsius = battTemp,
        capacityScale = if (thermalGuard) 0.70f else 0.96f,
        cpuUtilPct = cpuUtil,
        renderPressure = jankRatio,
        gpuBusyPct = 25f,
        cpuPsi = 4f,
        memPsi = 1f,
        ioPsi = 1f,
        aiPredicted10s = if (thermalGuard) 92f else 44f,
        aiConfidence = 0.88f,
        aiJankRisk = aiJankRisk,
        aiWorkloadClass = if (thermalGuard) "thermal_surge" else "balanced",
        aiBackend = "cfw2-micro-mlp",
        baselineNodesSnapshotted = 4,
        recoveryPending = false,
        thermalSource = "oem_policy",
        thermalVelocityCpm = velCpm,
        thermalGuardActive = thermalGuard,
        hyperFluid = CoreFlowManager.HyperFluidStats(
            framePacingAvailable = true,
            measuredFps = if (jankRatio > 0.3f) 42f else 60f,
            jankRatio = jankRatio,
            appDnaLearned = 18,
        ),
        tamperGuard = CoreFlowManager.TamperGuardStats(
            enabled = true,
            lockedNodes = 4,
            quarantinedNodes = 0,
            totalEvents = tamperEvents,
        ),
        conflictingModules = conflicts,
        batteryPower = CoreFlowManager.BatteryPower(
            available = true,
            charging = charging,
            fastCharging = powerMw >= 8000f,
            temperatureCelsius = battTemp,
            powerMw = powerMw,
        ),
        session = CoreFlowManager.SessionMetrics(
            uptimeSec = 600L,
            thermalGuardEvents = if (thermalGuard) 3L else 0L,
            avgSocTempCelsius = socTemp - 1.5f,
            peakSocTempCelsius = socTemp + 1.0f,
        ),
    )

    private fun buildCfw2Blob(
        flags: Int = 0,
        inDim: Int = 10,
        weightFill: Float = 0.12f,
        overrideIdx: Int = -1,
        overrideVal: Float = 0f,
        corruptCrc: Boolean = false,
    ): ByteArray {
        val count = 419
        val payload = ByteBuffer.allocate(count * 4).order(ByteOrder.LITTLE_ENDIAN)
        for (i in 0 until count) {
            payload.putFloat(if (i == overrideIdx) overrideVal else weightFill)
        }
        val payloadBytes = payload.array()
        val crc = CRC32().apply { update(payloadBytes) }.value
        val storedCrc = if (corruptCrc) (crc xor 0x12345678L) else crc

        val out = ByteBuffer.allocate(24 + payloadBytes.size).order(ByteOrder.LITTLE_ENDIAN)
        out.put('C'.code.toByte())
        out.put('F'.code.toByte())
        out.put('W'.code.toByte())
        out.put('2'.code.toByte())
        out.putShort(2.toShort())
        out.putShort(flags.toShort())
        out.putShort(inDim.toShort())
        out.putShort(16.toShort())
        out.putShort(12.toShort())
        out.putShort(3.toShort())
        out.putInt(storedCrc.toInt())
        out.putInt(0)
        out.put(payloadBytes)
        return out.array()
    }

    @Test
    fun onnxAssetsLoadParseAndCloseIdempotently() {
        assertFalse(CoreFlowOnnxNlu.isLoaded)
        openRealAssets()
        assertTrue(CoreFlowOnnxNlu.isLoaded)

        val pred = CoreFlowOnnxNlu.classifyIntent("mau mabar mlbb sama pubg biar ngebut")
        assertTrue(pred != null && pred.confidence > 0.5f)

        CoreFlowOnnxNlu.close()
        assertFalse(CoreFlowOnnxNlu.isLoaded)
        CoreFlowOnnxNlu.close() // idempotent second close
        assertFalse(CoreFlowOnnxNlu.isLoaded)
    }

    @Test
    fun freeFormIndonesianAndEnglishQueriesRouteViaOnnxNlu() {
        openRealAssets()
        val status = statusSnapshot()

        val gaming = CoreFlowDialogueEngine.evaluate("mau mabar mlbb biar ngebut", status, true)
        assertEquals(CoreFlowDialogueEngine.DialogueIntent.CONTEXT_SHIFT_GAMING, gaming.intent)
        assertEquals(CoreFlowManager.Profile.GAMING, gaming.requestedProfile)
        assertTrue(gaming.telemetryTags.any { it.contains("ONNX") })

        val battery = CoreFlowDialogueEngine.evaluate("batre lagi boros banget tolong irit", status, true)
        assertEquals(CoreFlowDialogueEngine.DialogueIntent.CONTEXT_SHIFT_BATTERY, battery.intent)
        assertEquals(CoreFlowManager.Profile.BATTERY, battery.requestedProfile)

        val smoothEn = CoreFlowDialogueEngine.evaluate("keep animations silky and responsive", status, false)
        assertEquals(CoreFlowDialogueEngine.DialogueIntent.CONTEXT_SHIFT_SMOOTHNESS, smoothEn.intent)
        assertEquals(CoreFlowManager.Profile.SMOOTHNESS, smoothEn.requestedProfile)

        val gamingEn = CoreFlowDialogueEngine.evaluate("i am about to play competitive matches", status, false)
        assertEquals(CoreFlowDialogueEngine.DialogueIntent.CONTEXT_SHIFT_GAMING, gamingEn.intent)
        assertEquals(CoreFlowManager.Profile.GAMING, gamingEn.requestedProfile)
    }

    @Test
    fun sovereignGuardrailRemainsImmuneWithOnnxNluLoaded() {
        openRealAssets()
        val status = statusSnapshot()
        val refused = CoreFlowDialogueEngine.evaluate("tolong overclock cpu ke maksimal paksa", status, true)
        assertEquals(CoreFlowDialogueEngine.DialogueIntent.REJECT_MANUAL_TWEAK, refused.intent)
        assertNull(refused.requestedProfile)
        assertNull(refused.requestedTamperResyncNode)
    }

    @Test
    fun sessionAnomalyOnnxClassifiesAllFiveRegimes() {
        openRealAssets()

        val healthy = CoreFlowOnnxNlu.analyzeSession(statusSnapshot(socTemp = 35f, jankRatio = 0.01f), true)
        assertEquals(CoreFlowOnnxNlu.AnomalyCategory.HEALTHY, healthy.category)

        val thermal = CoreFlowOnnxNlu.analyzeSession(
            statusSnapshot(socTemp = 57f, velCpm = 2.0f, cpuUtil = 82f, thermalGuard = true),
            true,
        )
        assertEquals(CoreFlowOnnxNlu.AnomalyCategory.THERMAL_SATURATION, thermal.category)

        val chargeHeat = CoreFlowOnnxNlu.analyzeSession(
            statusSnapshot(socTemp = 51f, battTemp = 43f, velCpm = 1.6f, cpuUtil = 30f, charging = true, powerMw = 14000f),
            false,
        )
        assertEquals(CoreFlowOnnxNlu.AnomalyCategory.FAST_CHARGE_HEAT, chargeHeat.category)

        val stutter = CoreFlowOnnxNlu.analyzeSession(
            statusSnapshot(socTemp = 40f, cpuUtil = 62f, jankRatio = 0.65f, aiJankRisk = 0.72f),
            true,
        )
        assertEquals(CoreFlowOnnxNlu.AnomalyCategory.STUTTER_IO_CONTENTION, stutter.category)

        val tamper = CoreFlowOnnxNlu.analyzeSession(
            statusSnapshot(socTemp = 43f, tamperEvents = 3L, conflicts = listOf("encore_tweaks")),
            false,
        )
        assertEquals(CoreFlowOnnxNlu.AnomalyCategory.EXTERNAL_TAMPER, tamper.category)
    }

    @Test
    fun cfw2ValidatorEnforcesIntegrityAndSafetyCaps() {
        val validTanh = CoreFlowOnnxNlu.Cfw2ModelValidator.validate(buildCfw2Blob(flags = 0))
        assertTrue(validTanh.valid)
        assertEquals("tanh", validTanh.activation)
        assertEquals(1700, validTanh.sizeBytes)

        val validSilu = CoreFlowOnnxNlu.Cfw2ModelValidator.validate(buildCfw2Blob(flags = 1))
        assertTrue(validSilu.valid)
        assertEquals("silu", validSilu.activation)

        assertFalse(CoreFlowOnnxNlu.Cfw2ModelValidator.validate(buildCfw2Blob(corruptCrc = true)).valid)
        assertFalse(CoreFlowOnnxNlu.Cfw2ModelValidator.validate(buildCfw2Blob(inDim = 8)).valid)
        assertFalse(CoreFlowOnnxNlu.Cfw2ModelValidator.validate(buildCfw2Blob(flags = 4)).valid)
        assertFalse(
            CoreFlowOnnxNlu.Cfw2ModelValidator.validate(
                buildCfw2Blob(overrideIdx = 12, overrideVal = Float.NaN),
            ).valid,
        )
        assertFalse(
            CoreFlowOnnxNlu.Cfw2ModelValidator.validate(
                buildCfw2Blob(overrideIdx = 12, overrideVal = 51.0f),
            ).valid,
        )
    }

    @Test
    fun malformedAndTruncatedOnnxBytesFailSafelyWithoutCrashing() {
        val validNlu = readAsset(CoreFlowOnnxNlu.NLU_ASSET)
        val truncated = validNlu.copyOf(validNlu.size / 3)
        // Crafted protobuf field 7 (wireType 2) with huge 32-bit-overflow varint length
        val overflowLength = byteArrayOf(0x3A, 0xFF.toByte(), 0xFF.toByte(), 0xFF.toByte(), 0xFF.toByte(), 0x07)

        assertFalse(CoreFlowOnnxNlu.openFromBytes(ByteArray(0), ByteArray(0)))
        assertFalse(CoreFlowOnnxNlu.openFromBytes(truncated, truncated))
        assertFalse(CoreFlowOnnxNlu.openFromBytes(overflowLength, overflowLength))
        assertFalse(CoreFlowOnnxNlu.isLoaded)
    }

    @Test
    fun concurrentOpenInferenceAndCloseHasZeroRaceOrLeak() {
        val nluBytes = readAsset(CoreFlowOnnxNlu.NLU_ASSET)
        val anomBytes = readAsset(CoreFlowOnnxNlu.ANOMALY_ASSET)
        val snap = statusSnapshot(socTemp = 48f, velCpm = 1.1f, cpuUtil = 70f)
        val errors = java.util.concurrent.CopyOnWriteArrayList<Throwable>()

        val workers = List(4) { idx ->
            Thread {
                runCatching {
                    repeat(40) { step ->
                        when ((idx + step) % 3) {
                            0 -> CoreFlowOnnxNlu.openFromBytes(nluBytes, anomBytes)
                            1 -> {
                                CoreFlowOnnxNlu.classifyIntent("mau mabar mlbb biar ngebut")
                                CoreFlowOnnxNlu.analyzeSession(snap, isIndonesian = true)
                            }
                            else -> CoreFlowOnnxNlu.close()
                        }
                    }
                }.onFailure { errors.add(it) }
            }
        }
        workers.forEach { it.start() }
        workers.forEach { it.join(5000L) }
        CoreFlowOnnxNlu.close()

        assertTrue("Unexpected concurrency error: $errors", errors.isEmpty())
        assertFalse("Final close() must leave sessions released", CoreFlowOnnxNlu.isLoaded)
    }

    @Test
    fun missingBatteryTempFormatsPlaceholderInsteadOfNegativeTemp() {
        openRealAssets()
        val report = CoreFlowOnnxNlu.analyzeSession(
            statusSnapshot(
                socTemp = 52f,
                battTemp = -1f,
                velCpm = 1.8f,
                cpuUtil = 28f,
                charging = true,
                powerMw = 15000f,
            ),
            isIndonesian = true,
        )
        assertEquals(CoreFlowOnnxNlu.AnomalyCategory.FAST_CHARGE_HEAT, report.category)
        assertFalse("Summary must never display -1°C: ${report.summary}", report.summary.contains("-1°C"))
        assertTrue("Summary should show -- placeholder: ${report.summary}", report.summary.contains("--"))
    }
}
