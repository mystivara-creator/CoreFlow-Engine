package io.coreflow.ui

import androidx.compose.animation.AnimatedVisibility
import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.nestedscroll.nestedScroll
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import io.coreflow.CoreFlowManager
import io.coreflow.InstalledAppEntry
import top.yukonga.miuix.kmp.basic.Card
import top.yukonga.miuix.kmp.basic.Icon
import top.yukonga.miuix.kmp.basic.PullToRefresh
import top.yukonga.miuix.kmp.basic.ScrollBehavior
import top.yukonga.miuix.kmp.basic.SmallTitle
import top.yukonga.miuix.kmp.basic.Text
import top.yukonga.miuix.kmp.basic.TextButton
import top.yukonga.miuix.kmp.basic.TextField
import top.yukonga.miuix.kmp.basic.rememberPullToRefreshState
import top.yukonga.miuix.kmp.icon.MiuixIcons
import top.yukonga.miuix.kmp.icon.extended.Search
import top.yukonga.miuix.kmp.preference.SwitchPreference
import top.yukonga.miuix.kmp.theme.MiuixTheme
import top.yukonga.miuix.kmp.utils.PressFeedbackType
import top.yukonga.miuix.kmp.utils.overScrollVertical
import top.yukonga.miuix.kmp.utils.scrollEndHaptic
import java.util.Locale

@Composable
fun AppsTab(
    strings: AppStrings,
    apps: List<InstalledAppEntry>,
    loading: Boolean,
    bypassOnGaming: Boolean,
    excludedApps: List<String>,
    appDnaEntries: Map<String, CoreFlowManager.AppDnaSummary>,
    isRefreshing: Boolean,
    contentPadding: PaddingValues,
    scrollBehavior: ScrollBehavior,
    onReloadApps: () -> Unit,
    onToggleExcluded: (String, Boolean) -> Unit,
) {
    var query by rememberSaveable { mutableStateOf("") }
    var filterMode by rememberSaveable { mutableIntStateOf(0) } // 0 = All, 1 = Games (Auto), 2 = Stock OEM (Excluded)
    var expandedPackage by rememberSaveable { mutableStateOf<String?>(null) }

    val excludedSet = remember(excludedApps) { excludedApps.toSet() }
    val gamesCount = remember(apps) { apps.count { it.isGame } }

    val filtered = remember(apps, query, filterMode, excludedSet, appDnaEntries) {
        val q = query.trim().lowercase(Locale.US)
        apps.filter { entry ->
            val matchesFilter = when (filterMode) {
                1 -> entry.isGame
                2 -> entry.packageName in excludedSet
                else -> true
            }
            val matchesQuery = q.isEmpty() ||
                entry.label.lowercase(Locale.US).contains(q) ||
                entry.packageName.lowercase(Locale.US).contains(q)
            matchesFilter && matchesQuery
        }.sortedWith(
            compareByDescending<InstalledAppEntry> { it.packageName in excludedSet }
                .thenByDescending { appDnaEntries.containsKey(it.packageName) }
                .thenBy { it.label.lowercase(Locale.US) },
        )
    }

    val pullState = rememberPullToRefreshState()

    PullToRefresh(
        isRefreshing = isRefreshing || loading,
        onRefresh = onReloadApps,
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
            verticalArrangement = Arrangement.spacedBy(8.dp),
        ) {
            // Concise Zero-Profile App DNA Explainer Banner
            item {
                Card(
                    modifier = Modifier.fillMaxWidth(),
                    insideMargin = PaddingValues(13.dp),
                ) {
                    Text(
                        text = if (strings.isIndonesian) {
                            "Otonom 100% · App DNA (${appDnaEntries.size} dipelajari)"
                        } else {
                            "100% Autonomous · App DNA (${appDnaEntries.size} learned)"
                        },
                        style = MiuixTheme.textStyles.body2,
                        fontWeight = FontWeight.SemiBold,
                        color = MiuixTheme.colorScheme.primary,
                    )
                    Spacer(Modifier.height(3.dp))
                    Text(
                        text = if (strings.isIndonesian) {
                            "Tidak perlu memilih profil per-aplikasi. CoreFlow otomatis mengenali game 3D & mempelajari kebutuhan tiap aplikasi. Aktifkan sakelar Stock OEM hanya jika ingin aplikasi berjalan tanpa intervensi CoreFlow."
                        } else {
                            "No manual per-app profiles needed. CoreFlow automatically detects 3D games and learns each app's optimal DNA. Toggle Stock OEM only if you want an app to bypass CoreFlow completely."
                        },
                        style = MiuixTheme.textStyles.footnote1,
                        color = MiuixTheme.colorScheme.onSurfaceVariantSummary,
                    )
                }
            }

            item {
                TextField(
                    value = query,
                    onValueChange = { query = it },
                    label = strings.searchAppsHint,
                    useLabelAsPlaceholder = true,
                    leadingIcon = {
                        Icon(
                            imageVector = MiuixIcons.Search,
                            contentDescription = "Search",
                            tint = MiuixTheme.colorScheme.onSurfaceVariantSummary,
                            modifier = Modifier.padding(start = 12.dp),
                        )
                    },
                    modifier = Modifier.fillMaxWidth(),
                )
            }

            // HyperOS-style Uniform Segmented Filter Bar
            item {
                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .clip(RoundedCornerShape(14.dp))
                        .background(MiuixTheme.colorScheme.surfaceContainer)
                        .padding(4.dp),
                    horizontalArrangement = Arrangement.spacedBy(4.dp),
                ) {
                    val tabs = listOf(
                        "${strings.filterAll} (${apps.size})",
                        "${strings.filterGames} ($gamesCount)",
                        "${strings.filterRules} (${excludedSet.size})",
                    )
                    tabs.forEachIndexed { idx, label ->
                        val selected = filterMode == idx
                        Box(
                            modifier = Modifier
                                .weight(1f)
                                .height(36.dp)
                                .clip(RoundedCornerShape(10.dp))
                                .background(
                                    if (selected) MiuixTheme.colorScheme.primary else Color.Transparent,
                                )
                                .clickable { filterMode = idx }
                                .padding(horizontal = 6.dp),
                            contentAlignment = Alignment.Center,
                        ) {
                            Text(
                                text = label,
                                fontSize = 12.sp,
                                fontWeight = if (selected) FontWeight.SemiBold else FontWeight.Medium,
                                color = if (selected) {
                                    MiuixTheme.colorScheme.onPrimary
                                } else {
                                    MiuixTheme.colorScheme.onSurfaceVariantSummary
                                },
                                maxLines = 1,
                                softWrap = false,
                                overflow = TextOverflow.Ellipsis,
                            )
                        }
                    }
                }
            }

            if (loading) {
                item {
                    Card(modifier = Modifier.fillMaxWidth(), insideMargin = PaddingValues(18.dp)) {
                        Text(
                            text = strings.loadingApps,
                            style = MiuixTheme.textStyles.body2,
                            color = MiuixTheme.colorScheme.onSurfaceVariantSummary,
                        )
                    }
                }
            } else if (filtered.isEmpty()) {
                item {
                    Card(modifier = Modifier.fillMaxWidth(), insideMargin = PaddingValues(18.dp)) {
                        Text(text = strings.noMatchingApps, style = MiuixTheme.textStyles.body1)
                        Spacer(Modifier.height(10.dp))
                        TextButton(text = strings.reloadApps, onClick = onReloadApps)
                    }
                }
            } else {
                item {
                    SmallTitle(text = "${strings.installedAppsHeader} (${filtered.size})")
                }

                items(items = filtered, key = { it.packageName }) { app ->
                    val isExcluded = app.packageName in excludedSet
                    val dna = appDnaEntries[app.packageName]
                    val isExpanded = expandedPackage == app.packageName
                    val badgeColor = when {
                        isExcluded -> Color(0xFFF59E0B)
                        dna != null -> Color(0xFFA78BFA)
                        else -> MiuixTheme.colorScheme.primary
                    }
                    val badgeText = when {
                        isExcluded -> "Stock OEM"
                        dna != null -> String.format(Locale.US, "DNA %.2fx", dna.uclampBias)
                        else -> if (strings.isIndonesian) "Otonom" else "Auto"
                    }

                    Card(
                        modifier = Modifier
                            .fillMaxWidth()
                            .then(
                                if (isExcluded) {
                                    Modifier.border(
                                        BorderStroke(1.dp, Color(0xFFF59E0B).copy(alpha = 0.45f)),
                                        RoundedCornerShape(16.dp),
                                    )
                                } else {
                                    Modifier
                                },
                            ),
                        insideMargin = PaddingValues(13.dp),
                        pressFeedbackType = PressFeedbackType.Tilt,
                        onClick = {
                            expandedPackage = if (isExpanded) null else app.packageName
                        },
                    ) {
                        Row(
                            modifier = Modifier.fillMaxWidth(),
                            verticalAlignment = Alignment.CenterVertically,
                            horizontalArrangement = Arrangement.spacedBy(11.dp),
                        ) {
                            if (app.icon != null) {
                                Image(
                                    bitmap = app.icon,
                                    contentDescription = app.label,
                                    modifier = Modifier
                                        .size(38.dp)
                                        .clip(RoundedCornerShape(10.dp)),
                                )
                            } else {
                                Box(
                                    modifier = Modifier
                                        .size(38.dp)
                                        .clip(RoundedCornerShape(10.dp))
                                        .background(MiuixTheme.colorScheme.primary.copy(alpha = 0.16f)),
                                    contentAlignment = Alignment.Center,
                                ) {
                                    Text(
                                        text = app.label.take(1).uppercase(),
                                        fontWeight = FontWeight.Bold,
                                        color = MiuixTheme.colorScheme.primary,
                                    )
                                }
                            }

                            Column(modifier = Modifier.weight(1f)) {
                                Row(
                                    verticalAlignment = Alignment.CenterVertically,
                                    horizontalArrangement = Arrangement.spacedBy(6.dp),
                                ) {
                                    Text(
                                        text = app.label,
                                        style = MiuixTheme.textStyles.body1,
                                        fontWeight = FontWeight.SemiBold,
                                        maxLines = 1,
                                        overflow = TextOverflow.Ellipsis,
                                        modifier = Modifier.weight(1f, fill = false),
                                    )
                                    if (app.isGame) {
                                        Box(
                                            modifier = Modifier
                                                .clip(RoundedCornerShape(6.dp))
                                                .background(Color(0xFFEF4444).copy(alpha = 0.16f))
                                                .padding(horizontal = 5.dp, vertical = 2.dp),
                                        ) {
                                            Text(
                                                text = "3D AUTO",
                                                fontSize = 9.sp,
                                                fontWeight = FontWeight.Bold,
                                                color = Color(0xFFEF4444),
                                                maxLines = 1,
                                            )
                                        }
                                    }
                                }
                                Text(
                                    text = app.packageName,
                                    fontSize = 11.sp,
                                    color = MiuixTheme.colorScheme.onSurfaceVariantSummary,
                                    maxLines = 1,
                                    overflow = TextOverflow.Ellipsis,
                                )
                            }

                            Box(
                                modifier = Modifier
                                    .clip(RoundedCornerShape(8.dp))
                                    .background(badgeColor.copy(alpha = 0.15f))
                                    .padding(horizontal = 8.dp, vertical = 4.dp),
                            ) {
                                Text(
                                    text = badgeText,
                                    fontSize = 10.sp,
                                    fontWeight = FontWeight.Bold,
                                    color = badgeColor,
                                    maxLines = 1,
                                )
                            }
                        }

                        AnimatedVisibility(visible = isExpanded) {
                            Column(modifier = Modifier.padding(top = 12.dp)) {
                                if (dna != null) {
                                    Text(
                                        text = if (strings.isIndonesian) {
                                            String.format(
                                                Locale.US,
                                                "App DNA Terpelajari: Bias UClamp %.2fx · Efisiensi %.2fx · %d sampel · %d launch",
                                                dna.uclampBias,
                                                dna.efficiencyWeight,
                                                dna.samples,
                                                dna.launches,
                                            )
                                        } else {
                                            String.format(
                                                Locale.US,
                                                "Learned App DNA: UClamp Bias %.2fx · Efficiency %.2fx · %d samples · %d launches",
                                                dna.uclampBias,
                                                dna.efficiencyWeight,
                                                dna.samples,
                                                dna.launches,
                                            )
                                        },
                                        fontSize = 11.sp,
                                        color = Color(0xFFA78BFA),
                                    )
                                    Spacer(Modifier.height(6.dp))
                                } else {
                                    Text(
                                        text = if (strings.isIndonesian) {
                                            "Belum ada DNA tersimpan — CoreFlow akan otomatis mempelajari pola beban saat aplikasi dibuka."
                                        } else {
                                            "No DNA learned yet — CoreFlow will automatically learn workload behavior on launch."
                                        },
                                        fontSize = 11.sp,
                                        color = MiuixTheme.colorScheme.onSurfaceVariantSummary,
                                    )
                                    Spacer(Modifier.height(6.dp))
                                }

                                if (bypassOnGaming && app.isGame && !isExcluded) {
                                    Text(
                                        text = strings.autoBypassHint,
                                        fontSize = 10.sp,
                                        color = Color(0xFF10B981),
                                    )
                                    Spacer(Modifier.height(6.dp))
                                }

                                Box(
                                    modifier = Modifier
                                        .fillMaxWidth()
                                        .clip(RoundedCornerShape(12.dp))
                                        .background(MiuixTheme.colorScheme.secondaryContainer.copy(alpha = 0.35f)),
                                ) {
                                    SwitchPreference(
                                        checked = isExcluded,
                                        onCheckedChange = { checked ->
                                            onToggleExcluded(app.packageName, checked)
                                        },
                                        title = if (strings.isIndonesian) {
                                            "Kecualikan (Stock OEM)"
                                        } else {
                                            "Exclude (Stock OEM)"
                                        },
                                        summary = if (strings.isIndonesian) {
                                            "Lepas seluruh kendali CoreFlow saat aplikasi ini di latar depan"
                                        } else {
                                            "Bypass CoreFlow completely while this app is in foreground"
                                        },
                                    )
                                }
                            }
                        }
                    }
                }
            }

            item { Spacer(Modifier.height(12.dp)) }
        }
    }
}
