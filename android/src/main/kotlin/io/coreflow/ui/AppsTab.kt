package io.coreflow.ui

import androidx.compose.animation.AnimatedVisibility
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.IntrinsicSize
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxHeight
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
import top.yukonga.miuix.kmp.basic.ButtonDefaults
import top.yukonga.miuix.kmp.basic.Card
import top.yukonga.miuix.kmp.basic.Icon
import top.yukonga.miuix.kmp.basic.ScrollBehavior
import top.yukonga.miuix.kmp.basic.SmallTitle
import top.yukonga.miuix.kmp.basic.Text
import top.yukonga.miuix.kmp.basic.TextButton
import top.yukonga.miuix.kmp.basic.TextField
import top.yukonga.miuix.kmp.icon.MiuixIcons
import top.yukonga.miuix.kmp.icon.extended.Search
import top.yukonga.miuix.kmp.theme.MiuixTheme

@Composable
fun AppsTab(
    strings: AppStrings,
    apps: List<InstalledAppEntry>,
    loading: Boolean,
    globalProfile: CoreFlowManager.Profile,
    bypassOnGaming: Boolean,
    rules: CoreFlowManager.AppRules,
    scrollBehavior: ScrollBehavior,
    onReloadApps: () -> Unit,
    onAssignBucket: (String, CoreFlowManager.AppRuleBucket) -> Unit,
) {
    var query by rememberSaveable { mutableStateOf("") }
    var filterMode by rememberSaveable { mutableIntStateOf(0) } // 0 = All, 1 = Games, 2 = Rules
    var expandedPackage by rememberSaveable { mutableStateOf<String?>(null) }

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

    LazyColumn(
        modifier = Modifier
            .fillMaxSize()
            .nestedScroll(scrollBehavior.nestedScrollConnection),
        contentPadding = PaddingValues(horizontal = 12.dp, vertical = 8.dp),
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

        item {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.spacedBy(8.dp),
            ) {
                val tabs = listOf(
                    "${strings.filterAll} (${apps.size})",
                    strings.filterGames,
                    "${strings.filterRules} (${rules.totalConfigured})",
                )
                tabs.forEachIndexed { idx, label ->
                    TextButton(
                        text = label,
                        onClick = { filterMode = idx },
                        colors = if (filterMode == idx) {
                            ButtonDefaults.textButtonColorsPrimary()
                        } else {
                            ButtonDefaults.textButtonColors()
                        },
                        modifier = Modifier.weight(1f),
                    )
                }
            }
        }

        if (loading) {
            item {
                Card(modifier = Modifier.fillMaxWidth(), insideMargin = PaddingValues(20.dp)) {
                    Text(
                        text = "Loading installed apps and native icons...",
                        style = MiuixTheme.textStyles.body2,
                        color = MiuixTheme.colorScheme.onSurfaceVariantSummary,
                    )
                }
            }
        } else if (filtered.isEmpty()) {
            item {
                Card(modifier = Modifier.fillMaxWidth(), insideMargin = PaddingValues(20.dp)) {
                    Text(text = "No matching apps found.", style = MiuixTheme.textStyles.body1)
                    Spacer(Modifier.height(8.dp))
                    TextButton(text = "Reload App List", onClick = onReloadApps)
                }
            }
        } else {
            item {
                SmallTitle(text = "INSTALLED APPS (${filtered.size})")
            }

            items(items = filtered, key = { it.packageName }) { app ->
                val currentBucket = rules.bucketFor(app.packageName)
                val isExpanded = expandedPackage == app.packageName
                val isOverride = currentBucket != CoreFlowManager.AppRuleBucket.AUTO
                val stateColor = when (currentBucket) {
                    CoreFlowManager.AppRuleBucket.GAMING -> Color(0xFFEF4444)
                    CoreFlowManager.AppRuleBucket.SMOOTHNESS -> Color(0xFF38BDF8)
                    CoreFlowManager.AppRuleBucket.BATTERY -> Color(0xFF22C55E)
                    CoreFlowManager.AppRuleBucket.EXCLUDED -> Color(0xFFF59E0B)
                    CoreFlowManager.AppRuleBucket.AUTO -> MiuixTheme.colorScheme.onSurfaceVariantSummary
                }

                Card(
                    modifier = Modifier.fillMaxWidth(),
                    insideMargin = PaddingValues(14.dp),
                    onClick = {
                        expandedPackage = if (isExpanded) null else app.packageName
                    },
                ) {
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        verticalAlignment = Alignment.CenterVertically,
                        horizontalArrangement = Arrangement.spacedBy(12.dp),
                    ) {
                        if (app.icon != null) {
                            Image(
                                bitmap = app.icon,
                                contentDescription = app.label,
                                modifier = Modifier
                                    .size(40.dp)
                                    .clip(RoundedCornerShape(10.dp)),
                            )
                        } else {
                            Box(
                                modifier = Modifier
                                    .size(40.dp)
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
                                            .padding(horizontal = 6.dp, vertical = 2.dp),
                                    ) {
                                        Text(
                                            text = "GAME",
                                            fontSize = 9.sp,
                                            fontWeight = FontWeight.Bold,
                                            color = Color(0xFFEF4444),
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

                    // Deep Per-App Detail Panel (Effective State + 5-Mode Selector + Auto-Bypass Status)
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
                                    text = "Auto Bypass Charging will engage automatically when launched.",
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
                                modifier = Modifier
                                    .fillMaxWidth()
                                    .height(IntrinsicSize.Min),
                                horizontalArrangement = Arrangement.spacedBy(6.dp),
                            ) {
                                for (b in firstRow) {
                                    TextButton(
                                        text = b.label,
                                        onClick = { onAssignBucket(app.packageName, b) },
                                        colors = if (currentBucket == b) {
                                            ButtonDefaults.textButtonColorsPrimary()
                                        } else {
                                            ButtonDefaults.textButtonColors()
                                        },
                                        modifier = Modifier.weight(1f).fillMaxHeight(),
                                    )
                                }
                            }
                            Spacer(Modifier.height(6.dp))
                            Row(
                                modifier = Modifier
                                    .fillMaxWidth()
                                    .height(IntrinsicSize.Min),
                                horizontalArrangement = Arrangement.spacedBy(6.dp),
                            ) {
                                for (b in secondRow) {
                                    TextButton(
                                        text = b.label,
                                        onClick = { onAssignBucket(app.packageName, b) },
                                        colors = if (currentBucket == b) {
                                            ButtonDefaults.textButtonColorsPrimary()
                                        } else {
                                            ButtonDefaults.textButtonColors()
                                        },
                                        modifier = Modifier.weight(1f).fillMaxHeight(),
                                    )
                                }
                                if (isOverride) {
                                    TextButton(
                                        text = strings.resetToGlobal,
                                        onClick = { onAssignBucket(app.packageName, CoreFlowManager.AppRuleBucket.AUTO) },
                                        modifier = Modifier.weight(1f).fillMaxHeight(),
                                    )
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
