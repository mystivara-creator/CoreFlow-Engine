package io.coreflow.ui

import androidx.compose.foundation.Image
import androidx.compose.foundation.background
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
import androidx.compose.ui.input.nestedscroll.nestedScroll
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
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
import top.yukonga.miuix.kmp.preference.OverlayDropdownPreference
import top.yukonga.miuix.kmp.theme.MiuixTheme

@Composable
fun AppsTab(
    apps: List<InstalledAppEntry>,
    loading: Boolean,
    rules: CoreFlowManager.AppRules,
    scrollBehavior: ScrollBehavior,
    onReloadApps: () -> Unit,
    onAssignBucket: (String, CoreFlowManager.AppRuleBucket) -> Unit,
) {
    var query by rememberSaveable { mutableStateOf("") }
    var filterMode by rememberSaveable { mutableIntStateOf(0) } // 0 = All, 1 = Games, 2 = Configured

    val bucketItems = remember { CoreFlowManager.AppRuleBucket.entries.map { it.label } }

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
                label = "Search apps or package name...",
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
                    "All (${apps.size})",
                    "Games",
                    "Rules (${rules.totalConfigured})",
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
                    Text(
                        text = "No matching apps found.",
                        style = MiuixTheme.textStyles.body1,
                    )
                    Spacer(Modifier.height(8.dp))
                    TextButton(
                        text = "Reload App List",
                        onClick = onReloadApps,
                    )
                }
            }
        } else {
            item {
                SmallTitle(text = "INSTALLED APPS (${filtered.size})")
            }

            items(items = filtered, key = { it.packageName }) { app ->
                val currentBucket = rules.bucketFor(app.packageName)
                val selectedIdx = CoreFlowManager.AppRuleBucket.entries.indexOf(currentBucket).coerceAtLeast(0)
                Card(modifier = Modifier.fillMaxWidth()) {
                    OverlayDropdownPreference(
                        items = bucketItems,
                        selectedIndex = selectedIdx,
                        title = if (app.isGame) "${app.label} · Game" else app.label,
                        summary = app.packageName,
                        startAction = {
                            if (app.icon != null) {
                                Image(
                                    bitmap = app.icon,
                                    contentDescription = app.label,
                                    modifier = Modifier
                                        .padding(end = 12.dp)
                                        .size(36.dp)
                                        .clip(RoundedCornerShape(10.dp)),
                                )
                            } else {
                                Box(
                                    modifier = Modifier
                                        .padding(end = 12.dp)
                                        .size(36.dp)
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
                        },
                        onSelectedIndexChange = { newIdx ->
                            val target = CoreFlowManager.AppRuleBucket.entries.getOrElse(newIdx) {
                                CoreFlowManager.AppRuleBucket.AUTO
                            }
                            if (target != currentBucket) {
                                onAssignBucket(app.packageName, target)
                            }
                        },
                    )
                }
            }
        }

        item { Spacer(Modifier.height(12.dp)) }
    }
}
