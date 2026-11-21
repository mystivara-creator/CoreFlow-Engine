package io.coreflow.ui

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.input.nestedscroll.nestedScroll
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import io.coreflow.AppPreferences
import io.coreflow.BuildConfig
import io.coreflow.CoreFlowManager
import top.yukonga.miuix.kmp.basic.ButtonDefaults
import top.yukonga.miuix.kmp.basic.Card
import top.yukonga.miuix.kmp.basic.Icon
import top.yukonga.miuix.kmp.basic.PullToRefresh
import top.yukonga.miuix.kmp.basic.ScrollBehavior
import top.yukonga.miuix.kmp.basic.SmallTitle
import top.yukonga.miuix.kmp.basic.Text
import top.yukonga.miuix.kmp.basic.TextButton
import top.yukonga.miuix.kmp.basic.rememberPullToRefreshState
import top.yukonga.miuix.kmp.icon.MiuixIcons
import top.yukonga.miuix.kmp.icon.extended.Backup
import top.yukonga.miuix.kmp.icon.extended.File
import top.yukonga.miuix.kmp.icon.extended.HorizontalSplit
import top.yukonga.miuix.kmp.icon.extended.Import
import top.yukonga.miuix.kmp.icon.extended.Info
import top.yukonga.miuix.kmp.icon.extended.Layers
import top.yukonga.miuix.kmp.icon.extended.Share
import top.yukonga.miuix.kmp.icon.extended.Theme
import top.yukonga.miuix.kmp.icon.extended.Timer
import top.yukonga.miuix.kmp.icon.extended.Translate
import top.yukonga.miuix.kmp.preference.ArrowPreference
import top.yukonga.miuix.kmp.preference.OverlayDropdownPreference
import top.yukonga.miuix.kmp.preference.SwitchPreference
import top.yukonga.miuix.kmp.theme.MiuixTheme
import top.yukonga.miuix.kmp.utils.PressFeedbackType
import top.yukonga.miuix.kmp.utils.overScrollVertical
import top.yukonga.miuix.kmp.utils.scrollEndHaptic

@Composable
fun SystemTab(
    strings: AppStrings,
    status: CoreFlowManager.DaemonStatus?,
    journalText: String?,
    busy: Boolean,
    isRefreshing: Boolean,
    themeModeIndex: Int,
    pollIntervalIndex: Int,
    languageIndex: Int,
    floatingNavBar: Boolean,
    navBadgesEnabled: Boolean,
    contentPadding: PaddingValues,
    scrollBehavior: ScrollBehavior,
    onRefresh: () -> Unit,
    onThemeModeChange: (Int) -> Unit,
    onPollIntervalChange: (Int) -> Unit,
    onLanguageChange: (Int) -> Unit,
    onFloatingNavBarChange: (Boolean) -> Unit,
    onNavBadgesChange: (Boolean) -> Unit,
    onExportProfile: () -> Unit,
    onImportProfile: () -> Unit,
    onLoadJournal: () -> Unit,
    onShareDiag: () -> Unit,
    onResetOemMap: () -> Unit,
    onRollback: () -> Unit,
    onOpenAbout: () -> Unit,
) {
    var confirmRollback by remember { mutableStateOf(false) }
    val pullState = rememberPullToRefreshState()

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
            // App Preferences & HyperOS 4 Visual Style
            item { SmallTitle(text = strings.sectionAppSettings) }
            item {
                Card(modifier = Modifier.fillMaxWidth()) {
                    OverlayDropdownPreference(
                        items = AppPreferences.languageLabels,
                        selectedIndex = languageIndex,
                        title = "Language / Bahasa",
                        summary = "Instant switch between English and Bahasa Indonesia",
                        startAction = { SettingLeadingIcon(MiuixIcons.Translate) },
                        onSelectedIndexChange = onLanguageChange,
                    )
                    OverlayDropdownPreference(
                        items = AppPreferences.themeModeLabels,
                        selectedIndex = themeModeIndex,
                        title = if (strings.isIndonesian) "Tema & Monet" else "Appearance & Monet Theme",
                        summary = if (strings.isIndonesian) {
                            "Sesuaikan tema Miuix terang, gelap, atau warna dinamis Monet"
                        } else {
                            "Customize Miuix light, dark, or dynamic wallpaper palette"
                        },
                        startAction = { SettingLeadingIcon(MiuixIcons.Theme) },
                        onSelectedIndexChange = onThemeModeChange,
                    )
                    OverlayDropdownPreference(
                        items = AppPreferences.pollIntervalLabels,
                        selectedIndex = pollIntervalIndex,
                        title = if (strings.isIndonesian) "Interval Telemetri" else "Telemetry Refresh Rate",
                        summary = if (strings.isIndonesian) {
                            "Seberapa sering statistik daemon diperbarui saat aplikasi dibuka"
                        } else {
                            "How often live daemon stats update while app is open"
                        },
                        startAction = { SettingLeadingIcon(MiuixIcons.Timer) },
                        onSelectedIndexChange = onPollIntervalChange,
                    )
                    SwitchPreference(
                        checked = floatingNavBar,
                        onCheckedChange = onFloatingNavBarChange,
                        title = strings.floatingBarTitle,
                        summary = strings.floatingBarSummary,
                        startAction = { SettingLeadingIcon(MiuixIcons.HorizontalSplit) },
                    )
                    SwitchPreference(
                        checked = navBadgesEnabled,
                        onCheckedChange = onNavBadgesChange,
                        title = strings.navBadgesTitle,
                        summary = strings.navBadgesSummary,
                        startAction = { SettingLeadingIcon(MiuixIcons.Layers) },
                    )
                }
            }

            // Profile Backup & Share (SAF with SoC Provenance)
            item { SmallTitle(text = strings.sectionProfileTransfer) }
            item {
                Card(modifier = Modifier.fillMaxWidth()) {
                    ArrowPreference(
                        title = strings.exportProfileTitle,
                        summary = strings.exportProfileSummary,
                        startAction = { SettingLeadingIcon(MiuixIcons.Backup) },
                        enabled = !busy && status != null,
                        onClick = onExportProfile,
                    )
                    ArrowPreference(
                        title = strings.importProfileTitle,
                        summary = strings.importProfileSummary,
                        startAction = { SettingLeadingIcon(MiuixIcons.Import) },
                        enabled = !busy,
                        onClick = onImportProfile,
                    )
                }
            }

            // Persistent OEM-Collaboration Map & Diagnostics
            item { SmallTitle(text = strings.sectionCoexistence) }
            item {
                val contended = status?.contendedPaths.orEmpty()
                val yielded = status?.oemYieldedPaths.orEmpty()
                val headerText = when {
                    contended.isNotEmpty() -> "${contended.size} Node(s) in Auto-Cooldown Backoff"
                    yielded.isNotEmpty() -> "${strings.oemCooperatingTitle} · ${yielded.size} Yielded"
                    else -> "Zero OEM Contention · Full Ownership"
                }
                val headerColor = when {
                    contended.isNotEmpty() -> Color(0xFFF59E0B)
                    yielded.isNotEmpty() -> Color(0xFF38BDF8)
                    else -> Color(0xFF22C55E)
                }

                Card(
                    modifier = Modifier.fillMaxWidth(),
                    insideMargin = PaddingValues(16.dp),
                    pressFeedbackType = PressFeedbackType.Tilt,
                    onClick = {},
                ) {
                    Text(
                        text = headerText,
                        style = MiuixTheme.textStyles.headline2,
                        color = headerColor,
                    )
                    Spacer(Modifier.height(4.dp))
                    if (contended.isEmpty() && yielded.isEmpty()) {
                        Text(
                            text = if (strings.isIndonesian) {
                                "CoreFlow Autonomous Co-Pilot berjalan harmonis bersama OEM tanpa konflik."
                            } else {
                                "CoreFlow Autonomous Co-Pilot is cleanly cooperating with OEM thermal & storage policies."
                            },
                            style = MiuixTheme.textStyles.footnote1,
                            color = MiuixTheme.colorScheme.onSurfaceVariantSummary,
                        )
                    } else {
                        if (contended.isNotEmpty()) {
                            Text(
                                text = contended.joinToString("\n"),
                                fontFamily = FontFamily.Monospace,
                                fontSize = 11.sp,
                                color = Color(0xFFF59E0B),
                            )
                        }
                        if (yielded.isNotEmpty()) {
                            if (contended.isNotEmpty()) Spacer(Modifier.height(8.dp))
                            Text(
                                text = strings.oemCooperatingSummary,
                                style = MiuixTheme.textStyles.footnote1,
                                color = MiuixTheme.colorScheme.onSurfaceVariantSummary,
                            )
                            Spacer(Modifier.height(4.dp))
                            Text(
                                text = yielded.joinToString("\n"),
                                fontFamily = FontFamily.Monospace,
                                fontSize = 11.sp,
                                color = MiuixTheme.colorScheme.onSurface,
                            )
                            Spacer(Modifier.height(10.dp))
                            TextButton(
                                text = strings.resetOemMap,
                                onClick = onResetOemMap,
                                enabled = !busy,
                                modifier = Modifier.fillMaxWidth(),
                            )
                        }
                    }
                }
            }

            item {
                Card(modifier = Modifier.fillMaxWidth()) {
                    ArrowPreference(
                        title = "Export & Share Diagnostics (--diag)",
                        summary = "Generate coreflow-diag-v${BuildConfig.VERSION_NAME}.json (Atlas + Status + CFJ4 Journal)",
                        startAction = { SettingLeadingIcon(MiuixIcons.Share) },
                        enabled = !busy,
                        onClick = onShareDiag,
                    )
                    ArrowPreference(
                        title = "Inspect CFJ4 Baseline Journal",
                        summary = "${status?.baselineNodesSnapshotted ?: 0} OEM sysfs nodes currently held",
                        startAction = { SettingLeadingIcon(MiuixIcons.File) },
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

            // Emergency Recovery
            item { SmallTitle(text = strings.sectionRecovery) }
            item {
                Card(
                    modifier = Modifier.fillMaxWidth(),
                    insideMargin = PaddingValues(16.dp),
                    pressFeedbackType = PressFeedbackType.Tilt,
                    onClick = { if (!busy) confirmRollback = !confirmRollback },
                ) {
                    Text(
                        text = "CFJ4 Zero-Trace OEM Rollback",
                        style = MiuixTheme.textStyles.headline2,
                    )
                    Spacer(Modifier.height(4.dp))
                    Text(
                        text = "Stops CoreFlow actuation and restores every modified sysfs & charging node to its exact factory OEM value.",
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

            // Dedicated About Entry (KernelSU / HyperOS 4 Style)
            item {
                Card(modifier = Modifier.fillMaxWidth()) {
                    ArrowPreference(
                        title = strings.aboutTitle,
                        summary = "${strings.aboutSummary} · v${BuildConfig.VERSION_NAME}",
                        startAction = { SettingLeadingIcon(MiuixIcons.Info) },
                        onClick = onOpenAbout,
                    )
                }
            }

            item { Spacer(Modifier.height(12.dp)) }
        }
    }
}

@Composable
private fun SettingLeadingIcon(icon: ImageVector) {
    Icon(
        imageVector = icon,
        contentDescription = null,
        tint = MiuixTheme.colorScheme.onSurfaceVariantSummary,
        modifier = Modifier
            .padding(end = 14.dp)
            .size(22.dp),
    )
}
