package io.coreflow

import ai.onnxruntime.OnnxTensor
import ai.onnxruntime.OrtEnvironment
import ai.onnxruntime.OrtSession
import android.content.Context
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.nio.FloatBuffer
import java.util.Locale
import java.util.zip.CRC32
import kotlin.math.abs
import kotlin.math.exp
import kotlin.math.sqrt
import kotlin.math.tanh

/**
 * Phase B On-Demand ONNX Runtime NLU & Session Anomaly Engine for CoreFlow Companion.
 *
 * Lifecycle Discipline:
 * - Lazy-loaded only when the user opens CFAE TELEMETRY LINK or runs AI Session Analysis.
 * - Deterministically closed on Lifecycle.Event.ON_STOP / DisposableEffect.onDispose so
 *   background RAM overhead is strictly 0 MB.
 * - Executes genuine `ai.onnxruntime.OrtSession` (single-threaded, arena-free) on Android
 *   ARM64 devices and falls back transparently to a pure-Kotlin reader of the exact same
 *   `.onnx` opset-13 protobuf weights on JVM unit tests or unsupported ABIs.
 */
object CoreFlowOnnxNlu {

    const val NLU_ASSET = "cfae_nlu_intent.onnx"
    const val ANOMALY_ASSET = "cfae_session_anomaly.onnx"
    const val NLU_INPUT_DIM = 256
    const val ANOMALY_INPUT_DIM = 8

    private val INTENT_ORDER = listOf(
        CoreFlowDialogueEngine.DialogueIntent.CONTEXT_SHIFT_GAMING,
        CoreFlowDialogueEngine.DialogueIntent.CONTEXT_SHIFT_BATTERY,
        CoreFlowDialogueEngine.DialogueIntent.CONTEXT_SHIFT_SMOOTHNESS,
        CoreFlowDialogueEngine.DialogueIntent.CONTEXT_SHIFT_BALANCED,
        CoreFlowDialogueEngine.DialogueIntent.TAMPER_RESYNC,
        CoreFlowDialogueEngine.DialogueIntent.DIAGNOSE_STUTTER,
        CoreFlowDialogueEngine.DialogueIntent.DIAGNOSE_THERMAL_LOAD,
        CoreFlowDialogueEngine.DialogueIntent.DIAGNOSE_CONFLICT_TAMPER,
        CoreFlowDialogueEngine.DialogueIntent.DIAGNOSE_LEARNING_DNA,
        CoreFlowDialogueEngine.DialogueIntent.REJECT_MANUAL_TWEAK,
        CoreFlowDialogueEngine.DialogueIntent.GENERAL_SUMMARY,
    )

    enum class AnomalyCategory {
        HEALTHY,
        THERMAL_SATURATION,
        FAST_CHARGE_HEAT,
        STUTTER_IO_CONTENTION,
        EXTERNAL_TAMPER,
    }

    data class IntentPrediction(
        val intent: CoreFlowDialogueEngine.DialogueIntent,
        val confidence: Float,
        val backendName: String,
    )

    data class SessionAnomalyReport(
        val category: AnomalyCategory,
        val confidence: Float,
        val title: String,
        val summary: String,
        val recommendation: String,
        val backendName: String,
    )

    data class Cfw2ValidationResult(
        val valid: Boolean,
        val sizeBytes: Int = 0,
        val crc32Hex: String = "",
        val activation: String = "tanh",
        val error: String? = null,
    )

    /**
     * Pure-Kotlin parser & executor for standard ONNX opset-13 2-layer MLP graphs
     * (`Gemm(transB=1)` -> `Tanh` -> `Gemm(transB=1)`). Used as an automatic zero-crash
     * fallback when native Android `libonnxruntime.so` cannot load (e.g. Linux JVM tests).
     */
    internal class OnnxMlpWeights(
        val inDim: Int,
        val hDim: Int,
        val outDim: Int,
        val w1: FloatArray,
        val b1: FloatArray,
        val w2: FloatArray,
        val b2: FloatArray,
    ) {
        fun forward(features: FloatArray): FloatArray {
            require(features.size == inDim) { "Expected $inDim features, got ${features.size}" }
            val h = FloatArray(hDim)
            for (i in 0 until hDim) {
                var acc = b1[i].toDouble()
                val row = i * inDim
                for (k in 0 until inDim) {
                    acc += w1[row + k].toDouble() * features[k].toDouble()
                }
                h[i] = tanh(acc).toFloat()
            }
            val logits = FloatArray(outDim)
            for (o in 0 until outDim) {
                var acc = b2[o].toDouble()
                val row = o * hDim
                for (k in 0 until hDim) {
                    acc += w2[row + k].toDouble() * h[k].toDouble()
                }
                logits[o] = acc.toFloat()
            }
            return logits
        }

        companion object {
            fun parseFromOnnxBytes(bytes: ByteArray): OnnxMlpWeights {
                var graphBytes: ByteArray? = null
                readProtoFields(bytes, 0, bytes.size) { fieldNum, wireType, start, len ->
                    if (fieldNum == 7 && wireType == 2) {
                        graphBytes = bytes.copyOfRange(start, start + len)
                    }
                }
                val g = graphBytes ?: error("ONNX ModelProto missing GraphProto")

                val tensors = HashMap<String, Pair<List<Int>, FloatArray>>()
                readProtoFields(g, 0, g.size) { fieldNum, wireType, start, len ->
                    if (fieldNum == 5 && wireType == 2) {
                        val (name, dims, floats) = parseTensorProto(g, start, len)
                        if (name.isNotEmpty()) {
                            tensors[name] = dims to floats
                        }
                    }
                }

                val (w1Dims, w1) = tensors["w1"] ?: error("ONNX graph missing initializer w1")
                val (_, b1) = tensors["b1"] ?: error("ONNX graph missing initializer b1")
                val (w2Dims, w2) = tensors["w2"] ?: error("ONNX graph missing initializer w2")
                val (_, b2) = tensors["b2"] ?: error("ONNX graph missing initializer b2")
                require(w1Dims.size == 2 && w2Dims.size == 2) { "Invalid MLP tensor dimensions" }
                val hDim = w1Dims[0]
                val inDim = w1Dims[1]
                val outDim = w2Dims[0]
                require(
                    inDim > 0 && hDim > 0 && outDim > 0 &&
                        w2Dims[1] == hDim &&
                        hDim.toLong() * inDim.toLong() == w1.size.toLong() &&
                        b1.size == hDim &&
                        outDim.toLong() * hDim.toLong() == w2.size.toLong() &&
                        b2.size == outDim &&
                        w1.all { it.isFinite() } &&
                        b1.all { it.isFinite() } &&
                        w2.all { it.isFinite() } &&
                        b2.all { it.isFinite() },
                ) {
                    "Invalid or non-finite MLP weights in ONNX graph"
                }
                return OnnxMlpWeights(inDim, hDim, outDim, w1, b1, w2, b2)
            }

            private fun parseTensorProto(
                buf: ByteArray,
                offset: Int,
                length: Int,
            ): Triple<String, List<Int>, FloatArray> {
                val dims = ArrayList<Int>(2)
                var name = ""
                var rawFloats = FloatArray(0)
                readProtoFields(buf, offset, length) { fieldNum, wireType, start, len ->
                    when {
                        fieldNum == 1 && wireType == 0 -> dims.add(start) // unpacked varint value
                        fieldNum == 1 && wireType == 2 -> {
                            var p = start
                            val pEnd = start + len
                            while (p < pEnd) {
                                val (v, np) = readVarint(buf, p, pEnd)
                                dims.add(v.toInt())
                                p = np
                            }
                        }
                        fieldNum == 8 && wireType == 2 -> name = String(buf, start, len, Charsets.UTF_8)
                        fieldNum == 9 && wireType == 2 && len % 4 == 0 -> {
                            val bb = ByteBuffer.wrap(buf, start, len).order(ByteOrder.LITTLE_ENDIAN)
                            val arr = FloatArray(len / 4)
                            for (i in arr.indices) arr[i] = bb.float
                            rawFloats = arr
                        }
                    }
                }
                return Triple(name, dims, rawFloats)
            }

            private fun readProtoFields(
                buf: ByteArray,
                offset: Int,
                length: Int,
                onField: (fieldNum: Int, wireType: Int, valueOrStart: Int, len: Int) -> Unit,
            ) {
                var pos = offset
                val end = offset + length
                while (pos < end) {
                    val (tag, nextPos) = readVarint(buf, pos, end)
                    pos = nextPos
                    val fieldNum = (tag ushr 3).toInt()
                    val wireType = (tag and 0x7L).toInt()
                    when (wireType) {
                        0 -> {
                            val (v, np) = readVarint(buf, pos, end)
                            pos = np
                            onField(fieldNum, 0, v.toInt(), 0)
                        }
                        1 -> {
                            require(end - pos >= 8) { "Truncated protobuf fixed64 field" }
                            pos += 8
                        }
                        2 -> {
                            val (size, np) = readVarint(buf, pos, end)
                            pos = np
                            require(size in 0L..(end - pos).toLong()) { "Truncated protobuf field" }
                            val sizeInt = size.toInt()
                            onField(fieldNum, 2, pos, sizeInt)
                            pos += sizeInt
                        }
                        5 -> {
                            require(end - pos >= 4) { "Truncated protobuf fixed32 field" }
                            pos += 4
                        }
                        else -> error("Unsupported protobuf wire type $wireType")
                    }
                }
            }

            private fun readVarint(buf: ByteArray, start: Int, end: Int): Pair<Long, Int> {
                var result = 0L
                var shift = 0
                var pos = start
                while (pos < end && shift < 64) {
                    val b = buf[pos++].toInt() and 0xFF
                    result = result or ((b and 0x7F).toLong() shl shift)
                    if ((b and 0x80) == 0) return result to pos
                    shift += 7
                }
                error("Malformed protobuf varint")
            }
        }
    }

    private class ManagedModelSession(
        val ortSession: OrtSession?,
        val ortEnv: OrtEnvironment?,
        val liteWeights: OnnxMlpWeights,
    ) {
        val backendName: String
            get() = if (ortSession != null) "ONNX Runtime" else "ONNX-Lite"

        fun run(features: FloatArray): FloatArray {
            val session = ortSession
            val env = ortEnv
            if (session != null && env != null) {
                val nativeOut = runCatching {
                    val shape = longArrayOf(1L, features.size.toLong())
                    OnnxTensor.createTensor(env, FloatBuffer.wrap(features), shape).use { tensor ->
                        session.run(mapOf("features" to tensor)).use { results ->
                            val raw = results[0].value
                            val row = (raw as? Array<*>)?.firstOrNull() as? FloatArray
                            if (row != null && row.all { it.isFinite() }) row.copyOf() else null
                        }
                    }
                }.getOrNull()
                if (nativeOut != null) return nativeOut
            }
            return liteWeights.forward(features)
        }

        fun close() {
            runCatching { ortSession?.close() }
        }
    }

    private val NON_ALNUM_REGEX = Regex("[^a-z0-9]+")
    private val lock = Any()
    private var nluSession: ManagedModelSession? = null
    private var anomalySession: ManagedModelSession? = null
    private var lifecycleGeneration: Long = 0L

    val isLoaded: Boolean
        get() = synchronized(lock) { nluSession != null || anomalySession != null }

    val activeBackendLabel: String
        get() = synchronized(lock) {
            nluSession?.backendName ?: anomalySession?.backendName ?: "Standby (0 MB)"
        }

    fun open(context: Context): Boolean {
        val startGen = synchronized(lock) {
            if (nluSession != null && anomalySession != null) return true
            lifecycleGeneration
        }
        val appCtx = context.applicationContext ?: context
        val nluBytes = runCatching { appCtx.assets.open(NLU_ASSET).use { it.readBytes() } }.getOrNull()
        val anomBytes = runCatching { appCtx.assets.open(ANOMALY_ASSET).use { it.readBytes() } }.getOrNull()
        return openFromBytesInternal(nluBytes, anomBytes, expectedGeneration = startGen)
    }

    fun openFromBytes(nluBytes: ByteArray?, anomalyBytes: ByteArray?): Boolean {
        val startGen = synchronized(lock) { lifecycleGeneration }
        return openFromBytesInternal(nluBytes, anomBytes, expectedGeneration = startGen)
    }

    private fun openFromBytesInternal(
        nluBytes: ByteArray?,
        anomalyBytes: ByteArray?,
        expectedGeneration: Long,
    ): Boolean {
        val builtNlu = nluBytes?.let { buildManagedSession(it) }
        val builtAnom = anomalyBytes?.let { buildManagedSession(it) }
        synchronized(lock) {
            // If close() was called while async open() was loading native sessions, release immediately (0 MB leak!)
            if (lifecycleGeneration != expectedGeneration) {
                builtNlu?.close()
                builtAnom?.close()
                return false
            }
            if (nluSession == null && builtNlu != null) {
                nluSession = builtNlu
            } else {
                builtNlu?.close()
            }
            if (anomalySession == null && builtAnom != null) {
                anomalySession = builtAnom
            } else {
                builtAnom?.close()
            }
            return nluSession != null || anomalySession != null
        }
    }

    /**
     * Deterministically closes all native ONNX Runtime sessions so background RAM returns to 0 MB.
     * Safe to invoke repeatedly (idempotent) and cancels any in-flight async open().
     */
    fun close() = synchronized(lock) {
        lifecycleGeneration++
        nluSession?.close()
        anomalySession?.close()
        nluSession = null
        anomalySession = null
    }

    private fun buildManagedSession(modelBytes: ByteArray): ManagedModelSession? {
        val lite = runCatching { OnnxMlpWeights.parseFromOnnxBytes(modelBytes) }.getOrNull() ?: return null
        val ortPair = runCatching {
            val env = OrtEnvironment.getEnvironment()
            val opts = OrtSession.SessionOptions().apply {
                setIntraOpNumThreads(1)
                setInterOpNumThreads(1)
                setMemoryPatternOptimization(false)
                setCPUArenaAllocator(false)
            }
            val sess = opts.use { env.createSession(modelBytes, it) }
            sess to env
        }.getOrNull()
        return ManagedModelSession(
            ortSession = ortPair?.first,
            ortEnv = ortPair?.second,
            liteWeights = lite,
        )
    }

    /**
     * Exact deterministic FNV-1a 32-bit feature hashing matching `tools/train_companion_onnx.py`.
     */
    fun featurizeQuery(raw: String): FloatArray {
        val cleaned = raw.trim().lowercase(Locale.US)
            .replace(NON_ALNUM_REGEX, " ")
            .trim()
        val vec = FloatArray(NLU_INPUT_DIM)
        if (cleaned.isEmpty()) return vec

        val tokens = cleaned.split(' ').filter { it.isNotEmpty() }
        fun addFeat(key: String, weight: Float) {
            val h = fnv1a32(key)
            val bucket = h and (NLU_INPUT_DIM - 1)
            val sign = if (((h ushr 8) and 1) == 0) 1.0f else -1.0f
            vec[bucket] += sign * weight
        }

        for (tok in tokens) {
            if (tok.length >= 2) {
                addFeat("w:$tok", 1.5f)
            }
            val padded = "^$tok$"
            if (padded.length >= 3) {
                for (i in 0..(padded.length - 3)) {
                    addFeat("g:${padded.substring(i, i + 3)}", 0.6f)
                }
            }
        }
        for (i in 0 until (tokens.size - 1)) {
            addFeat("b:${tokens[i]}_${tokens[i + 1]}", 1.8f)
        }

        var sumSq = 0.0
        for (v in vec) sumSq += v.toDouble() * v.toDouble()
        val norm = sqrt(sumSq)
        if (norm > 1e-6) {
            val inv = (1.0 / norm).toFloat()
            for (i in vec.indices) vec[i] *= inv
        }
        return vec
    }

    private fun fnv1a32(text: String): Int {
        var h = 0x811C9DC5.toInt()
        for (b in text.toByteArray(Charsets.UTF_8)) {
            h = h xor (b.toInt() and 0xFF)
            h *= 0x01000193
        }
        return h
    }

    private fun softmax(logits: FloatArray): FloatArray {
        var maxVal = Float.NEGATIVE_INFINITY
        for (x in logits) if (x > maxVal) maxVal = x
        val exps = DoubleArray(logits.size)
        var sum = 0.0
        for (i in logits.indices) {
            val clamped = (logits[i] - maxVal).toDouble().coerceIn(-30.0, 30.0)
            val e = exp(clamped)
            exps[i] = e
            sum += e
        }
        val out = FloatArray(logits.size)
        val denom = if (sum > 0.0) sum else 1.0
        for (i in logits.indices) out[i] = (exps[i] / denom).toFloat()
        return out
    }

    fun classifyIntent(rawQuery: String): IntentPrediction? {
        val features = featurizeQuery(rawQuery)
        if (features.all { it == 0f }) return null
        // Hold lock across <0.15ms native inference to guarantee zero use-after-close against ON_STOP close()
        val (logits, backend) = synchronized(lock) {
            val sess = nluSession ?: return null
            val out = runCatching { sess.run(features) }.getOrNull() ?: return null
            out to sess.backendName
        }
        if (logits.size != INTENT_ORDER.size || logits.any { !it.isFinite() }) return null
        val probs = softmax(logits)
        var bestIdx = 0
        for (i in 1 until probs.size) {
            if (probs[i] > probs[bestIdx]) bestIdx = i
        }
        return IntentPrediction(
            intent = INTENT_ORDER[bestIdx],
            confidence = probs[bestIdx].coerceIn(0f, 1f),
            backendName = backend,
        )
    }

    fun extractSessionFeatures(status: CoreFlowManager.DaemonStatus?): FloatArray {
        if (status == null) return FloatArray(ANOMALY_INPUT_DIM)
        val peakTemp = status.session.peakSocTempCelsius.takeIf { it > 0f }
            ?: status.socTempCelsius.takeIf { it > 0f } ?: 35f
        val avgTemp = status.session.avgSocTempCelsius.takeIf { it > 0f }
            ?: status.socTempCelsius.takeIf { it > 0f } ?: 34f
        val peakNorm = ((peakTemp - 30f) / 30f).coerceIn(0f, 1.5f)
        val avgNorm = ((avgTemp - 30f) / 30f).coerceIn(0f, 1.5f)
        val slopeNorm = (status.thermalVelocityCpm / 3.0f).coerceIn(0f, 1.5f)
        val jankNorm = maxOf(
            status.hyperFluid.jankRatio ?: 0f,
            status.aiJankRisk,
        ).coerceIn(0f, 1.0f)
        val cpuNorm = (status.cpuUtilPct / 100f).coerceIn(0f, 1.0f)
        val guardNorm = if (status.thermalGuardActive || status.session.thermalGuardEvents > 0L) 1.0f else 0.0f
        val tamperCount = status.tamperGuard.totalEvents + status.conflictingModules.size.toLong()
        val tamperNorm = (tamperCount.toFloat() / 2.0f).coerceIn(0f, 1.0f)
        val chargeHeatNorm = if (status.batteryPower.charging) {
            val powerFactor = (status.batteryPower.powerMw / 12000f).coerceIn(0f, 1.2f)
            val battHeat = ((status.batteryTempCelsius.takeIf { it > 0f } ?: 32f) - 34f) / 10f
            maxOf(powerFactor, battHeat).coerceIn(0f, 1.2f)
        } else {
            0.0f
        }
        return floatArrayOf(
            peakNorm,
            avgNorm,
            slopeNorm,
            jankNorm,
            cpuNorm,
            guardNorm,
            tamperNorm,
            chargeHeatNorm,
        )
    }

    fun analyzeSession(
        status: CoreFlowManager.DaemonStatus?,
        isIndonesian: Boolean,
    ): SessionAnomalyReport {
        val features = extractSessionFeatures(status)
        val (logits, sessBackend) = synchronized(lock) {
            val sess = anomalySession
            if (sess != null) {
                runCatching { sess.run(features) }.getOrNull() to sess.backendName
            } else {
                null to "Telemetry"
            }
        }
        val (category, conf, backend) = if (
            logits != null &&
            logits.size == AnomalyCategory.entries.size &&
            logits.all { it.isFinite() }
        ) {
            val probs = softmax(logits)
            var bestIdx = 0
            for (i in 1 until probs.size) {
                if (probs[i] > probs[bestIdx]) bestIdx = i
            }
            Triple(AnomalyCategory.entries[bestIdx], probs[bestIdx].coerceIn(0f, 1f), sessBackend)
        } else {
            Triple(AnomalyCategory.HEALTHY, 0.5f, "Telemetry")
        }

        val tempStr = status?.socTempCelsius?.takeIf { it > 0f }
            ?.let { String.format(Locale.US, "%.1f°C", it) } ?: "--"
        val battStr = status?.batteryTempCelsius?.takeIf { it > 0f }
            ?.let { "${it.toInt()}°C" } ?: "--"
        val jankPct = ((maxOf(status?.hyperFluid?.jankRatio ?: 0f, status?.aiJankRisk ?: 0f)) * 100f)
            .toInt().coerceIn(0, 100)

        return when (category) {
            AnomalyCategory.HEALTHY -> SessionAnomalyReport(
                category = category,
                confidence = conf,
                title = if (isIndonesian) "Sesi Sehat & Seimbang" else "Healthy Equilibrium",
                summary = if (isIndonesian) {
                    "Suhu SoC ($tempStr), risiko jank ($jankPct%), dan penguncian 0444 berada di zona optimal tanpa anomali dominan."
                } else {
                    "SoC temperature ($tempStr), jank risk ($jankPct%), and 0444 locks are operating smoothly with no dominant anomaly."
                },
                recommendation = if (isIndonesian) {
                    "Pertahankan mode otomatis Balanced agar adaptasi RLS & App DNA terus menstabilkan sesi."
                } else {
                    "Keep Balanced adaptive mode active so online RLS and App DNA continue refining efficiency."
                },
                backendName = backend,
            )
            AnomalyCategory.THERMAL_SATURATION -> SessionAnomalyReport(
                category = category,
                confidence = conf,
                title = if (isIndonesian) "Akumulasi Panas Termal" else "Thermal Saturation Detected",
                summary = if (isIndonesian) {
                    "Terdeteksi saturasi termal sustained (SoC $tempStr, prediksi 10d ${status?.aiPredicted10s?.toInt() ?: 0}%). Thermal Guard aktif menjaga batas aman."
                } else {
                    "Sustained thermal saturation detected (SoC $tempStr, 10s forecast ${status?.aiPredicted10s?.toInt() ?: 0}%). Thermal Guard is actively capping headroom."
                },
                recommendation = if (isIndonesian) {
                    "Gunakan profil Battery sementara atau beri jeda pendinginan 2–3 menit agar headroom pulih penuh."
                } else {
                    "Switch briefly to Battery profile or allow 2–3 minutes of cooling so thermal headroom recovers."
                },
                backendName = backend,
            )
            AnomalyCategory.FAST_CHARGE_HEAT -> SessionAnomalyReport(
                category = category,
                confidence = conf,
                title = if (isIndonesian) "Panas Pengisian Cepat" else "Fast-Charge Heat Accumulation",
                summary = if (isIndonesian) {
                    "Kenaikan suhu berkorelasi kuat dengan daya pengisian (${(status?.batteryPower?.powerMw ?: 0f).toInt()} mW, baterai $battStr)."
                } else {
                    "Thermal rise correlates strongly with charging power (${(status?.batteryPower?.powerMw ?: 0f).toInt()} mW, battery $battStr)."
                },
                recommendation = if (isIndonesian) {
                    "Aktifkan Bypass Charging (jika didukung) atau Step-Charging di Control Hub saat dipakai sambil mengisi daya."
                } else {
                    "Enable Bypass Charging (if supported) or Step-Charging in Control Hub when using the phone while plugged in."
                },
                backendName = backend,
            )
            AnomalyCategory.STUTTER_IO_CONTENTION -> SessionAnomalyReport(
                category = category,
                confidence = conf,
                title = if (isIndonesian) "Risiko Stutter / Kontensi I/O" else "Stutter & I/O Contention Risk",
                summary = if (isIndonesian) {
                    "Risiko jank meningkat ($jankPct%) meski ruang termal masih aman ($tempStr). Kemungkinan lonjakan I/O atau render frame."
                } else {
                    "Elevated jank risk ($jankPct%) despite safe thermal headroom ($tempStr), indicating UI render or storage I/O contention."
                },
                recommendation = if (isIndonesian) {
                    "Coba profil Smoothness atau pastikan Storage Burst & Frame Smoothness aktif di Control Hub."
                } else {
                    "Try Smoothness profile or verify Storage Burst and Frame Smoothness are enabled in Control Hub."
                },
                backendName = backend,
            )
            AnomalyCategory.EXTERNAL_TAMPER -> SessionAnomalyReport(
                category = category,
                confidence = conf,
                title = if (isIndonesian) "Intervensi / Konflik Eksternal" else "External Tamper / Module Conflict",
                summary = if (isIndonesian) {
                    "Terdeteksi ${status?.tamperGuard?.totalEvents ?: 0} insiden tamper dan ${status?.conflictingModules?.size ?: 0} modul bentrok yang mencoba mengubah parameter sysfs."
                } else {
                    "Detected ${status?.tamperGuard?.totalEvents ?: 0} tamper event(s) and ${status?.conflictingModules?.size ?: 0} conflicting module(s) attempting sysfs overrides."
                },
                recommendation = if (isIndonesian) {
                    "Jalankan perintah RESYNC di konsol dialog untuk merebut balik & mengunci ulang seluruh node 0444, lalu nonaktifkan modul bentrok."
                } else {
                    "Run RESYNC in the dialogue console to reclaim & re-lock all 0444 nodes, and disable conflicting tuner modules."
                },
                backendName = backend,
            )
        }
    }

    /**
     * Exact client-side CFW2 binary validator matching `coreflowd` (`src/kernel/neural_predictor.cpp`).
     * Validates magic, version, flags, 10->16->12->3 dimensions, IEEE CRC32, finite & |w| <= 50.0.
     */
    object Cfw2ModelValidator {
        private const val HEADER_BYTES = 24
        private const val MAX_FILE_BYTES = 16384
        private const val EXPECTED_FLOATS = (16 * 10) + 16 + (12 * 16) + 12 + (3 * 12) + 3 // 419
        private const val EXPECTED_BYTES = HEADER_BYTES + EXPECTED_FLOATS * 4 // 1700
        private const val MAX_ABS_WEIGHT = 50.0f

        fun validate(bytes: ByteArray): Cfw2ValidationResult {
            if (bytes.size < HEADER_BYTES || bytes.size > MAX_FILE_BYTES) {
                return Cfw2ValidationResult(false, bytes.size, error = "Invalid size (${bytes.size} B)")
            }
            if (bytes[0] != 'C'.code.toByte() || bytes[1] != 'F'.code.toByte() ||
                bytes[2] != 'W'.code.toByte() || bytes[3] != '2'.code.toByte()
            ) {
                return Cfw2ValidationResult(false, bytes.size, error = "Invalid CFW2 magic header")
            }
            val bb = ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN)
            bb.position(4)
            val version = bb.short.toInt() and 0xFFFF
            val flags = bb.short.toInt() and 0xFFFF
            val inDim = bb.short.toInt() and 0xFFFF
            val h1Dim = bb.short.toInt() and 0xFFFF
            val h2Dim = bb.short.toInt() and 0xFFFF
            val outDim = bb.short.toInt() and 0xFFFF
            val expectedCrc = bb.int.toLong() and 0xFFFFFFFFL
            val reserved = bb.int

            if (version != 2) return Cfw2ValidationResult(false, bytes.size, error = "Unsupported version $version")
            if ((flags and 0x0001.inv()) != 0) {
                return Cfw2ValidationResult(false, bytes.size, error = "Unsupported flags 0x${flags.toString(16)}")
            }
            if (reserved != 0) return Cfw2ValidationResult(false, bytes.size, error = "Non-zero reserved field")
            if (inDim != 10 || h1Dim != 16 || h2Dim != 12 || outDim != 3) {
                return Cfw2ValidationResult(
                    false,
                    bytes.size,
                    error = "Unsupported topology ${inDim}x${h1Dim}x${h2Dim}x$outDim (expected 10x16x12x3)",
                )
            }
            if (bytes.size != EXPECTED_BYTES) {
                return Cfw2ValidationResult(false, bytes.size, error = "Payload length mismatch (${bytes.size} != $EXPECTED_BYTES)")
            }

            val crc = CRC32()
            crc.update(bytes, HEADER_BYTES, bytes.size - HEADER_BYTES)
            val actualCrc = crc.value and 0xFFFFFFFFL
            if (actualCrc != expectedCrc) {
                return Cfw2ValidationResult(false, bytes.size, error = "CRC32 checksum mismatch")
            }

            for (i in 0 until EXPECTED_FLOATS) {
                val w = bb.float
                if (!w.isFinite() || abs(w) > MAX_ABS_WEIGHT) {
                    return Cfw2ValidationResult(false, bytes.size, error = "Non-finite or out-of-range weight at #$i")
                }
            }

            return Cfw2ValidationResult(
                valid = true,
                sizeBytes = bytes.size,
                crc32Hex = String.format(Locale.US, "0x%08x", actualCrc),
                activation = if ((flags and 1) != 0) "silu" else "tanh",
            )
        }
    }
}
