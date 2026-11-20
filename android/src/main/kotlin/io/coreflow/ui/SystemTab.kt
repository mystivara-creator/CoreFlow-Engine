package io.coreflow.ui

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.nestedscroll.nestedScroll
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import io.coreflow.BuildConfig
import io.coreflow.CoreFlowManager
import top.yukonga.miuix.kmp.basic.ButtonDefaults
import top.yukonga.miuix.kmp.basic.Card
import top.yukonga.miuix.kmp.basic.ScrollBehavior
import top.yukonga.miuix.kmp.basic.SmallTitle
import top.yukonga.miuix.kmp.basic.Text
import top.yukonga.miuix.kmp.basic.TextButton
import top.yukonga.miuix.kmp.preference.ArrowPreference
import top.yukonga.miuix.kmp.theme.MiuixTheme

@Composable
fun SystemTab(
    status: CoreFlowManager.DaemonStatus?,
    journalText: String?,
    busy: Boolean,
    scrollBehavior: ScrollBehavior,
    onLoadJournal: () -> Unit,
    onShareDiag: () -> Unit,
    onRollback: () -> Unit,
) {
    var confirmRollback by remember { mutableStateOf(false) }

    LazyColumn(
        modifier = Modifier
            .fillMaxSize()
            .nestedScroll(scrollBehavior.nestedScrollConnection),
        contentPadding = PaddingValues(horizontal = 12.dp, vertical = 8.dp),
        verticalArrangement = Arrangement.spacedBy(10.dp),
    ) {
        item {
            SmallTitle(text = "OEM COEXISTENCE & SAFETY")
        }

        item {
            val contended = status?.contendedPaths.orEmpty()
            Card(
                modifier = Modifier.fillMaxWidth(),
                insideMargin = PaddingValues(16.dp),
            ) {
                Text(
                    text = if (contended.isEmpty()) {
                        "Zero OEM Contention · Full Ownership"
                    } else {
                        "${contended.size} Node(s) in Auto-Cooldown Backoff"
                    },
                    style = MiuixTheme.textStyles.headline2,
                    color = if (contended.isEmpty()) Color(0xFF22C55E) else Color(0xFFF59E0B),
                )
                Spacer(Modifier.height(4.dp))
                Text(
                    text = if (contended.isEmpty()) {
                        "CoreFlow v5.1.0 is cleanly cooperating with OEM thermal & storage policies."
                    } else {
                        contended.joinToString("\n")
                    },
                    style = MiuixTheme.textStyles.footnote1,
                    color = MiuixTheme.colorScheme.onSurfaceVariantSummary,
                )
            }
        }

        item {
            SmallTitle(text = "DIAGNOSTICS & CFJ4 JOURNAL")
        }

        item {
            Card(modifier = Modifier.fillMaxWidth()) {
                ArrowPreference(
                    title = "Export & Share Diagnostics (--diag)",
                    summary = "Generate coreflow-diag-v5.1.0.json and open Android Share Sheet",
                    enabled = !busy,
                    onClick = onShareDiag,
                )
                ArrowPreference(
                    title = "Inspect CFJ4 Baseline Journal",
                    summary = "${status?.baselineNodesSnapshotted ?: 0} OEM sysfs nodes safely snapshotted",
                    enabled = !busy,
                    onClick = onLoadJournal,
                )
            }
        }

        if (!journalText.isNullOrBlank()) {
            item {
                Card(
                    modifier = Modifier.fillMaxWidth(),
                    insideMargin = PaddingValues(14.dp),
                ) {
                    Text(
                        text = journalText.take(2400),
                        fontFamily = FontFamily.Monospace,
                        fontSize = 11.sp,
                        color = MiuixTheme.colorScheme.onSurfaceVariantSummary,
                    )
                }
            }
        }

        item {
            SmallTitle(text = "RECOVERY")
        }

        item {
            Card(
                modifier = Modifier.fillMaxWidth(),
                insideMargin = PaddingValues(16.dp),
            ) {
                Text(
                    text = "CFJ4 Zero-Trace OEM Rollback",
                    style = MiuixTheme.textStyles.headline2,
                )
                Spacer(Modifier.height(4.dp))
                Text(
                    text = "Stops CoreFlow actuation and restores every modified sysfs node to its exact factory OEM value.",
                    style = MiuixTheme.textStyles.body2,
                    color = MiuixTheme.colorScheme.onSurfaceVariantSummary,
                )
                Spacer(Modifier.height(12.dp))
                if (!confirmRollback) {
                    TextButton(
                        text = "Restore OEM Baseline",
                        onClick = { confirmRollback = true },
                        enabled = !busy,
                        modifier = Modifier.fillMaxWidth(),
                    )
                } else {
                    TextButton(
                        text = "Confirm Rollback Now",
                        onClick = {
                            confirmRollback = false
                            onRollback()
                        },
                        enabled = !busy,
                        colors = ButtonDefaults.textButtonColorsPrimary(),
                        modifier = Modifier.fillMaxWidth(),
                    )
                }
            }
        }

        item {
            SmallTitle(text = "ABOUT")
        }

        item {
            Card(
                modifier = Modifier.fillMaxWidth(),
                insideMargin = PaddingValues(16.dp),
            ) {
                Text(
                    text = "CoreFlow Companion v${BuildConfig.VERSION_NAME}",
                    style = MiuixTheme.textStyles.headline2,
                )
                Spacer(Modifier.height(4.dp))
                Text(
                    text = "Built with Miuix Compose & libsu for CFAE System Framework (Magisk / KernelSU / APatch). Apache-2.0 License.",
                    style = MiuixTheme.textStyles.footnote1,
                    color = MiuixTheme.colorScheme.onSurfaceVariantSummary,
                )
            }
        }

        item { Spacer(Modifier.height(12.dp)) }
    }
}
