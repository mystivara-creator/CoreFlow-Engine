package io.coreflow

import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.os.Build
import androidx.core.app.NotificationCompat
import androidx.core.app.NotificationManagerCompat
import androidx.core.app.RemoteInput
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.launch
import org.json.JSONObject
import java.util.Locale

internal object SovereignTamperNotification {
    const val CHANNEL_ID = "cfae_sovereign_guard"
    const val ACTION_NODE_TAMPER = "io.coreflow.ACTION_NODE_TAMPER"
    const val ACTION_TAMPER_DISMISS = "io.coreflow.ACTION_TAMPER_DISMISS"
    const val ACTION_TAMPER_REPLY = "io.coreflow.ACTION_TAMPER_REPLY"
    const val KEY_TAMPER_REPLY = "key_cfae_tamper_reply"
    const val EXTRA_NODE_PATH = "extra_tamper_node_path"
    const val EXTRA_NOTIFICATION_ID = "extra_tamper_notif_id"
    const val EXTRA_EXPECTED = "extra_tamper_expected"
    const val EXTRA_OBSERVED = "extra_tamper_observed"
    const val EXTRA_REASON = "extra_tamper_reason"

    fun ensureChannel(context: Context) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val nm = context.getSystemService(NotificationManager::class.java) ?: return
            if (nm.getNotificationChannel(CHANNEL_ID) == null) {
                val channel = NotificationChannel(
                    CHANNEL_ID,
                    "CFAE Sovereign Guard",
                    NotificationManager.IMPORTANCE_HIGH,
                ).apply {
                    description = "Notifikasi proteksi tegas CFAE System Framework terhadap intervensi node kernel eksternal."
                    enableVibration(true)
                }
                nm.createNotificationChannel(channel)
            }
        }
    }

    fun notificationIdForNode(nodePath: String): Int =
        60000 + (nodePath.hashCode() and 0x0FFF)

    fun showTamperAlert(
        context: Context,
        nodePath: String,
        expectedValue: String,
        observedValue: String,
        reason: String,
        reclaimed: Boolean,
        circuitBreakerOpen: Boolean,
    ) {
        ensureChannel(context)
        val notifId = notificationIdForNode(nodePath)
        val leaf = nodePath.substringAfterLast('/').ifBlank { nodePath }

        val sovereignHeadline = "CoreFlow Guard · Konflik Node Kernel"
        val sovereignBody = buildString {
            append("Layanan sistem/OEM atau modul lain berulang kali mengubah [$leaf] ")
            append("saat CoreFlow sedang mengaturnya.\n\n")
            append("• Node: $nodePath\n")
            append("• Nilai sistem/luar: $observedValue → Target CoreFlow: $expectedValue\n")
            append("• Konteks: ${reason.ifBlank { "Keseimbangan termal & penjadwalan CoreFlow" }}\n")
            append(
                if (circuitBreakerOpen) {
                    "• Status: Circuit breaker 3x/5d aktif — dikarantina 30d agar tidak terjadi write-loop."
                } else if (reclaimed) {
                    "• Status: Nilai dipulihkan sementara & dicatat di Live Feed."
                } else {
                    "• Status: Dipantau oleh CoreFlow Guard."
                },
            )
            append("\n\nBalas cepat (opsional): REPORT / WHY / RESYNC.")
        }

        // Tap notification -> open Companion App LiveFeedTab with forensic tamper context
        val contentIntent = Intent(context, MainActivity::class.java).apply {
            flags = Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_SINGLE_TOP or Intent.FLAG_ACTIVITY_CLEAR_TOP
            putExtra(MainActivity.EXTRA_OPEN_TAB, 2)
            putExtra(MainActivity.EXTRA_FORENSIC_PROMPT, "Laporan forensik konflik node $nodePath")
        }
        val contentPi = PendingIntent.getActivity(
            context,
            notifId,
            contentIntent,
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE,
        )

        // Action 1: "Tandai Dibaca"
        val dismissIntent = Intent(context, CoreFlowTamperActionReceiver::class.java).apply {
            action = ACTION_TAMPER_DISMISS
            putExtra(EXTRA_NOTIFICATION_ID, notifId)
            putExtra(EXTRA_NODE_PATH, nodePath)
        }
        val dismissPi = PendingIntent.getBroadcast(
            context,
            notifId + 10000,
            dismissIntent,
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE,
        )

        // Action 2: Inline Reply (RemoteInput) supporting RESYNC / ENFORCE / REPORT / WHY only
        val remoteInput = RemoteInput.Builder(KEY_TAMPER_REPLY)
            .setLabel("Kode CFAE: REPORT, WHY, atau RESYNC")
            .build()
        val replyIntent = Intent(context, CoreFlowTamperActionReceiver::class.java).apply {
            action = ACTION_TAMPER_REPLY
            putExtra(EXTRA_NOTIFICATION_ID, notifId)
            putExtra(EXTRA_NODE_PATH, nodePath)
            putExtra(EXTRA_EXPECTED, expectedValue)
            putExtra(EXTRA_OBSERVED, observedValue)
            putExtra(EXTRA_REASON, reason)
        }
        val replyFlags = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_MUTABLE
        } else {
            PendingIntent.FLAG_UPDATE_CURRENT
        }
        val replyPi = PendingIntent.getBroadcast(
            context,
            notifId + 20000,
            replyIntent,
            replyFlags,
        )
        val replyAction = NotificationCompat.Action.Builder(
            R.drawable.ic_coreflow_tile,
            "Detail / Balas",
            replyPi,
        ).addRemoteInput(remoteInput).build()

        val notification = NotificationCompat.Builder(context, CHANNEL_ID)
            .setSmallIcon(R.drawable.ic_coreflow_tile)
            .setContentTitle(sovereignHeadline)
            .setContentText("Konflik tulis berulang pada [$leaf]; CoreFlow mengaktifkan karantina anti write-loop.")
            .setStyle(NotificationCompat.BigTextStyle().bigText(sovereignBody))
            .setPriority(NotificationCompat.PRIORITY_DEFAULT)
            .setCategory(NotificationCompat.CATEGORY_STATUS)
            .setAutoCancel(true)
            .setContentIntent(contentPi)
            .addAction(0, "Tandai Dibaca", dismissPi)
            .addAction(replyAction)
            .build()

        runCatching {
            NotificationManagerCompat.from(context).notify(notifId, notification)
        }
    }

    fun showActionFeedback(context: Context, notifId: Int, title: String, message: String) {
        ensureChannel(context)
        val notification = NotificationCompat.Builder(context, CHANNEL_ID)
            .setSmallIcon(R.drawable.ic_coreflow_tile)
            .setContentTitle(title)
            .setContentText(message)
            .setStyle(NotificationCompat.BigTextStyle().bigText(message))
            .setPriority(NotificationCompat.PRIORITY_DEFAULT)
            .setAutoCancel(true)
            .build()
        runCatching {
            NotificationManagerCompat.from(context).notify(notifId, notification)
        }
    }
}

/**
 * Sovereign Tamper Guard IPC Receiver.
 * Protected by android.permission.DUMP in AndroidManifest.xml so only root coreflowd can invoke it.
 */
class CoreFlowTamperReceiver : BroadcastReceiver() {
    override fun onReceive(context: Context, intent: Intent) {
        if (intent.action != SovereignTamperNotification.ACTION_NODE_TAMPER) return
        val rawPayload = intent.getStringExtra("payload")?.trim().orEmpty()
        if (rawPayload.isEmpty()) return

        runCatching {
            val obj = JSONObject(rawPayload)
            val nodePath = obj.optString("nodePath", "").trim()
            if (nodePath.isEmpty()) return
            val expected = obj.optString("expectedValue", "?")
            val observed = obj.optString("observedValue", "?")
            val reason = obj.optString("reason", "Keseimbangan termal & penjadwalan CFAE")
            val reclaimed = obj.optBoolean("reclaimed", false)
            val breakerOpen = obj.optBoolean("circuitBreakerOpen", false)
            SovereignTamperNotification.showTamperAlert(
                context = context,
                nodePath = nodePath,
                expectedValue = expected,
                observedValue = observed,
                reason = reason,
                reclaimed = reclaimed,
                circuitBreakerOpen = breakerOpen,
            )
        }
    }
}

/**
 * Internal non-exported receiver handling "Tandai Dibaca" and Inline Reply (RESYNC / ENFORCE / REPORT / WHY).
 * Strictly rejects any compromise/YIELD command.
 */
class CoreFlowTamperActionReceiver : BroadcastReceiver() {
    private val scope = CoroutineScope(SupervisorJob() + Dispatchers.IO)

    override fun onReceive(context: Context, intent: Intent) {
        val notifId = intent.getIntExtra(SovereignTamperNotification.EXTRA_NOTIFICATION_ID, 60000)
        val nodePath = intent.getStringExtra(SovereignTamperNotification.EXTRA_NODE_PATH).orEmpty()

        when (intent.action) {
            SovereignTamperNotification.ACTION_TAMPER_DISMISS -> {
                NotificationManagerCompat.from(context).cancel(notifId)
            }

            SovereignTamperNotification.ACTION_TAMPER_REPLY -> {
                val input = RemoteInput.getResultsFromIntent(intent)
                    ?.getCharSequence(SovereignTamperNotification.KEY_TAMPER_REPLY)
                    ?.toString()
                    ?.trim()
                    ?.uppercase(Locale.US)
                    .orEmpty()

                when (input) {
                    "RESYNC", "ENFORCE" -> {
                        val pending = goAsync()
                        scope.launch {
                            try {
                                val res = CoreFlowManager.resolveTamper(
                                    nodePath = nodePath.ifBlank { "*" },
                                    actionCode = input,
                                )
                                val leaf = nodePath.substringAfterLast('/').ifBlank { "semua node aktif" }
                                if (res.isSuccess) {
                                    SovereignTamperNotification.showActionFeedback(
                                        context = context,
                                        notifId = notifId,
                                        title = "CFAE Sovereign Sync · $input Terverifikasi",
                                        message = "Otoritas CFAE pada [$leaf] telah disinkronkan ulang dan dikunci 0444.",
                                    )
                                } else {
                                    SovereignTamperNotification.showActionFeedback(
                                        context = context,
                                        notifId = notifId,
                                        title = "CFAE Sovereign Sync · Gagal",
                                        message = res.exceptionOrNull()?.message ?: "Gagal menghubungi coreflowd.",
                                    )
                                }
                            } finally {
                                pending.finish()
                            }
                        }
                    }

                    "REPORT", "WHY" -> {
                        NotificationManagerCompat.from(context).cancel(notifId)
                        val openIntent = Intent(context, MainActivity::class.java).apply {
                            flags = Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_SINGLE_TOP or Intent.FLAG_ACTIVITY_CLEAR_TOP
                            putExtra(MainActivity.EXTRA_OPEN_TAB, 2)
                            putExtra(
                                MainActivity.EXTRA_FORENSIC_PROMPT,
                                "Laporan forensik intervensi node ${nodePath.ifBlank { "aktif" }}",
                            )
                        }
                        context.startActivity(openIntent)
                    }

                    else -> {
                        SovereignTamperNotification.showActionFeedback(
                            context = context,
                            notifId = notifId,
                            title = "CFAE Sovereign Guard · Perintah Ditolak",
                            message = "CFAE tidak menerima kompromi atau perintah \"$input\". Gunakan RESYNC, ENFORCE, REPORT, atau WHY.",
                        )
                    }
                }
            }
        }
    }
}
