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
import top.yukonga.miuix.kmp.theme.MiuixTheme
import top.yukonga.miuix.kmp.utils.PressFeedbackType
import top.yukonga.miuix.kmp.utils.overScrollVertical
import top.yukonga.miuix.kmp.utils.scrollEndHaptic

@Composable
fun AppsTab(
    strings: AppStrings,
    apps: List<InstalledAppEntry>,
    loading: Boolean,
    globalProfile: CoreFlowManager.Profile,
    bypassOnGaming: Boolean,
    rules: CoreFlowManager.AppRules,
    isRefreshing: Boolean,
    contentPadding: PaddingValues,
    scrollBehavior: ScrollBehavior,
    onReloadApps: () -> Unit,
    onAssignBucket: (String, CoreFlowManager.AppRuleBucket) -> Unit,
) {
    var query by rememberSaveable { mutableStateOf("") }
    var filterMode by rememberSaveable { mutableIntStateOf(0) } // 0 = All, 1 = Games, 2 = Rules
    var expandedPackage by rememberSaveable { mutableStateOf<String?>(null) }

    val gamesCount = remember(apps, rules) {
        apps.count { it.isGame || rules.bucketFor(it.packageName) == CoreFlowManager.AppRuleBucket.GAMING }
    }

    val filtered = remember(apps, query, filterMode, rules) {
        val q = query.trim().lowercase()
        apps.filter { entry ->
            val matchesFilter = when (filterMode) {
                1 -> entry.isGame || rules.bucketFor(entry.packageName) == CoreFlowManager.AppRuleBucket.GAMING
                2 -> rules.bucketFor(entry.packageName) != CoreFlowManager.AppRuleBucket.AUTO
                else -> true
            }
            val matchesQuery = q.isEmpty() ||
                entry.label.lowercase().contains(q) ||
                entry.packageName.lowercase().contains(q)
            matchesFilter && matchesQuery
        }
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

            // HyperOS-style Uniform Segmented Filter Bar (strictly single-line, identical heights)
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
                        "${strings.filterRules} (${rules.totalConfigured})",
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
                    val currentBucket = rules.bucketFor(app.packageName)
                    val isExpanded = expandedPackage == app.packageName
                    val isOverride = currentBucket != CoreFlowManager.AppRuleBucket.AUTO
                    val stateColor = bucketAccentColor(currentBucket)

                    Card(
                        modifier = Modifier.fillMaxWidth(),
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
                                            text = "GAME",
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
                                .background(stateColor.copy(alpha = 0.15f))
                                .padding(horizontal = 8.dp, vertical = 4.dp),
                        ) {
                            Text(
                                text = currentBucket.label,
                                fontSize = 10.sp,
                                fontWeight = FontWeight.Bold,
                                color = stateColor,
                                maxLines = 1,
                            )
                        }
                    }

                    // Deep Per-App Detail Panel (Crisp Uniform 3x2 Chip Grid)
                    AnimatedVisibility(visible = isExpanded) {
                        Column(modifier = Modifier.padding(top = 12.dp)) {
                            val effectiveLabel = if (isOverride) {
                                "${strings.effectiveOverride}: ${currentBucket.label}"
                            } else {
                                "${strings.effectiveGlobal}: ${globalProfile.label}"
                            }
                            Text(
                                text = effectiveLabel,
                                fontSize = 11.sp,
                                fontWeight = FontWeight.Medium,
                                color = MiuixTheme.colorScheme.primary,
                            )
                            if (bypassOnGaming && (currentBucket == CoreFlowManager.AppRuleBucket.GAMING || app.isGame)) {
                                Spacer(Modifier.height(2.dp))
                                Text(
                                    text = strings.autoBypassHint,
                                    fontSize = 10.sp,
                                    color = Color(0xFF10B981),
                                )
                            }
                            Spacer(Modifier.height(10.dp))
                            val firstRow = listOf(
                                CoreFlowManager.AppRuleBucket.AUTO,
                                CoreFlowManager.AppRuleBucket.GAMING,
                                CoreFlowManager.AppRuleBucket.SMOOTHNESS,
                            )
                            val secondRow = listOf(
                                CoreFlowManager.AppRuleBucket.BATTERY,
                                CoreFlowManager.AppRuleBucket.EXCLUDED,
                            )
                            Row(
                                modifier = Modifier.fillMaxWidth(),
                                horizontalArrangement = Arrangement.spacedBy(6.dp),
                            ) {
                                for (b in firstRow) {
                                    BucketChip(
                                        label = b.label,
                                        selected = currentBucket == b,
                                        accent = bucketAccentColor(b),
                                        modifier = Modifier.weight(1f),
                                        onClick = { onAssignBucket(app.packageName, b) },
                                    )
                                }
                            }
                            Spacer(Modifier.height(6.dp))
                            Row(
                                modifier = Modifier.fillMaxWidth(),
                                horizontalArrangement = Arrangement.spacedBy(6.dp),
                            ) {
                                for (b in secondRow) {
                                    BucketChip(
                                        label = b.label,
                                        selected = currentBucket == b,
                                        accent = bucketAccentColor(b),
                                        modifier = Modifier.weight(1f),
                                        onClick = { onAssignBucket(app.packageName, b) },
                                    )
                                }
                                if (isOverride) {
                                    BucketChip(
                                        label = strings.resetShort,
                                        selected = false,
                                        accent = Color(0xFF94A3B8),
                                        outlined = true,
                                        modifier = Modifier.weight(1f),
                                        onClick = { onAssignBucket(app.packageName, CoreFlowManager.AppRuleBucket.AUTO) },
                                    )
                                } else {
                                    Spacer(Modifier.weight(1f))
                                }
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

@Composable
private fun bucketAccentColor(bucket: CoreFlowManager.AppRuleBucket): Color = when (bucket) {
    CoreFlowManager.AppRuleBucket.GAMING -> Color(0xFFEF4444)
    CoreFlowManager.AppRuleBucket.SMOOTHNESS -> Color(0xFF38BDF8)
    CoreFlowManager.AppRuleBucket.BATTERY -> Color(0xFF22C55E)
    CoreFlowManager.AppRuleBucket.EXCLUDED -> Color(0xFFF59E0B)
    CoreFlowManager.AppRuleBucket.AUTO -> MiuixTheme.colorScheme.primary
}

@Composable
private fun BucketChip(
    label: String,
    selected: Boolean,
    accent: Color,
    modifier: Modifier = Modifier,
    outlined: Boolean = false,
    onClick: () -> Unit,
) {
    val bgColor = when {
        selected -> accent
        outlined -> Color.Transparent
        else -> MiuixTheme.colorScheme.secondaryContainer.copy(alpha = 0.42f)
    }
    val textColor = when {
        selected -> Color.White
        outlined -> MiuixTheme.colorScheme.onSurfaceVariantSummary
        else -> MiuixTheme.colorScheme.onSurface
    }
    val borderMod = if (outlined) {
        Modifier.border(
            BorderStroke(1.dp, MiuixTheme.colorScheme.outline.copy(alpha = 0.45f)),
            RoundedCornerShape(10.dp),
        )
    } else {
        Modifier
    }

    Box(
        modifier = modifier
            .height(36.dp)
            .clip(RoundedCornerShape(10.dp))
            .background(bgColor)
            .then(borderMod)
            .clickable(onClick = onClick)
            .padding(horizontal = 8.dp),
        contentAlignment = Alignment.Center,
    ) {
        Text(
            text = label,
            fontSize = 12.sp,
            fontWeight = if (selected) FontWeight.Bold else FontWeight.Medium,
            color = textColor,
            maxLines = 1,
            softWrap = false,
            overflow = TextOverflow.Ellipsis,
        )
    }
}
