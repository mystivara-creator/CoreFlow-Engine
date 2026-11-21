package io.coreflow.ui

import android.content.Intent
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.animation.AnimatedContent
import androidx.compose.animation.animateColorAsState
import androidx.compose.animation.core.tween
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.animation.slideInHorizontally
import androidx.compose.animation.slideOutHorizontally
import androidx.compose.animation.togetherWith
import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.defaultMinSize
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.navigationBarsPadding
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.shadow
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.core.content.FileProvider
import io.coreflow.AppCatalog
import io.coreflow.AppPreferences
import io.coreflow.CoreFlowManager
import io.coreflow.InstalledAppEntry
import kotlinx.coroutines.delay
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch
import top.yukonga.miuix.kmp.basic.Icon
import top.yukonga.miuix.kmp.basic.IconButton
import top.yukonga.miuix.kmp.basic.MiuixScrollBehavior
import top.yukonga.miuix.kmp.basic.NavigationBar
import top.yukonga.miuix.kmp.basic.NavigationBarItem
import top.yukonga.miuix.kmp.basic.Scaffold
import top.yukonga.miuix.kmp.basic.SnackbarHost
import top.yukonga.miuix.kmp.basic.SnackbarHostState
import top.yukonga.miuix.kmp.basic.Text
import top.yukonga.miuix.kmp.basic.TopAppBar
import top.yukonga.miuix.kmp.icon.MiuixIcons
import top.yukonga.miuix.kmp.icon.extended.Home
import top.yukonga.miuix.kmp.icon.extended.ListView
import top.yukonga.miuix.kmp.icon.extended.MindMap
import top.yukonga.miuix.kmp.icon.extended.Refresh
import top.yukonga.miuix.kmp.icon.extended.Settings
import top.yukonga.miuix.kmp.icon.extended.Tune
import top.yukonga.miuix.kmp.theme.MiuixTheme
import java.io.File

private data class NavTabSpec(
    val index: Int,
    val icon: ImageVector,
    val label: String,
    val badgeCount: Int = 0,
)

@Composable
fun CoreFlowApp(
    themeModeIndex: Int,
    onThemeModeChange: (Int) -> Unit,
) {
    val context = LocalContext.current
    val scope = rememberCoroutineScope()
    val snackbarHostState = remember { SnackbarHostState() }

    var selectedTab by rememberSaveable { mutableIntStateOf(0) }
    var showAboutScreen by rememberSaveable { mutableStateOf(false) }
    var pollIntervalIndex by remember { mutableIntStateOf(AppPreferences.getPollIntervalIndex(context)) }
    var languageIndex by remember { mutableIntStateOf(AppPreferences.getLanguageIndex(context)) }
    var floatingNavBar by remember { mutableStateOf(AppPreferences.getFloatingNavBar(context)) }
    var navBadgesEnabled by remember { mutableStateOf(AppPreferences.getNavBadgesEnabled(context)) }
    val strings = rememberAppStrings(languageIndex)

    var hasRoot by remember { mutableStateOf<Boolean?>(null) }
    var status by remember { mutableStateOf<CoreFlowManager.DaemonStatus?>(null) }
    var lastError by remember { mutableStateOf<String?>(null) }
    var busyAction by remember { mutableStateOf(false) }
    var isManualRefreshing by remember { mutableStateOf(false) }

    var installedApps by remember { mutableStateOf<List<InstalledAppEntry>>(emptyList()) }
    var appsLoading by remember { mutableStateOf(false) }
    var journalText by remember { mutableStateOf<String?>(null) }

    suspend fun refreshStatus() {
        val rootOk = hasRoot ?: CoreFlowManager.checkRootAccess().also { hasRoot = it }
        if (!rootOk) {
            lastError = "Root access not granted. Allow superuser in Magisk, KernelSU, or APatch."
            return
        }
        CoreFlowManager.getStatus()
            .onSuccess {
                status = it
                lastError = null
            }
            .onFailure { err ->
                lastError = err.message ?: "Unable to reach coreflowd"
            }
    }

    fun triggerPullRefresh() {
        if (isManualRefreshing) return
        scope.launch {
            isManualRefreshing = true
            hasRoot = CoreFlowManager.checkRootAccess()
            refreshStatus()
            delay(360L)
            isManualRefreshing = false
        }
    }

    // SAF Profile Export Launcher
    val exportProfileLauncher = rememberLauncherForActivityResult(
        contract = ActivityResultContracts.CreateDocument("application/json"),
    ) { uri ->
        val currentStatus = status ?: return@rememberLauncherForActivityResult
        if (uri == null) return@rememberLauncherForActivityResult
        scope.launch {
            runCatching {
                val payload = CoreFlowManager.buildShareableProfileJson(currentStatus)
                context.contentResolver.openOutputStream(uri)?.use { out ->
                    out.write(payload.toByteArray(Charsets.UTF_8))
                } ?: error("Cannot open output stream")
            }.onSuccess {
                snackbarHostState.showSnackbar("Profile exported (${currentStatus.hardware.socModel})")
            }.onFailure { err ->
                snackbarHostState.showSnackbar(err.message ?: "Export failed")
            }
        }
    }

    // SAF Profile Import Launcher
    val importProfileLauncher = rememberLauncherForActivityResult(
        contract = ActivityResultContracts.OpenDocument(),
    ) { uri ->
        if (uri == null) return@rememberLauncherForActivityResult
        scope.launch {
            busyAction = true
            runCatching {
                val raw = context.contentResolver.openInputStream(uri)?.bufferedReader()?.use { it.readText() }
                    ?: error("Cannot read profile file")
                CoreFlowManager.importShareableProfileJson(raw, status?.hardware?.socModel.orEmpty()).getOrThrow()
            }.onSuccess { res ->
                refreshStatus()
                val note = if (res.socMismatchWarning) {
                    "Imported ${res.appliedCount} settings (note: source SoC ${res.sourceSoc} differs)"
                } else {
                    "Imported ${res.appliedCount} settings cleanly"
                }
                snackbarHostState.showSnackbar(note)
            }.onFailure { err ->
                snackbarHostState.showSnackbar(err.message ?: "Import failed")
            }
            busyAction = false
        }
    }

    LaunchedEffect(pollIntervalIndex) {
        hasRoot = CoreFlowManager.checkRootAccess()
        val intervalMs = AppPreferences.pollIntervalValuesMs[pollIntervalIndex]
        while (isActive) {
            refreshStatus()
            delay(intervalMs)
        }
    }

    LaunchedEffect(selectedTab) {
        if (selectedTab == 3 && installedApps.isEmpty() && !appsLoading) {
            appsLoading = true
            installedApps = AppCatalog.loadLaunchableApps(context)
            appsLoading = false
        }
    }

    AnimatedContent(
        targetState = showAboutScreen,
        transitionSpec = {
            if (targetState) {
                (slideInHorizontally(tween(260)) { it / 3 } + fadeIn(tween(220))) togetherWith
                    (slideOutHorizontally(tween(240)) { -it / 6 } + fadeOut(tween(180)))
            } else {
                (slideInHorizontally(tween(240)) { -it / 6 } + fadeIn(tween(200))) togetherWith
                    (slideOutHorizontally(tween(240)) { it / 3 } + fadeOut(tween(180)))
            }
        },
        label = "CoreFlowRouteTransition",
    ) { isAbout ->
        if (isAbout) {
            AboutScreen(
                strings = strings,
                daemonVersion = status?.version,
                onBack = { showAboutScreen = false },
            )
            return@AnimatedContent
        }

        val scrollBehavior = MiuixScrollBehavior()
        val tabTitles = listOf(
            strings.titleHome,
            strings.titleControl,
            strings.titleLive,
            strings.titleApps,
            strings.titleSettings,
        )
        val tabSubtitles = listOf(
            status?.let {
                val hz = it.hardware.refreshHz
                "v${it.version} · ${if (it.isRunning) "PID ${it.pid} · ${hz}Hz" else strings.statusStopped}"
            } ?: "Autonomous Co-Pilot",
            strings.subControl,
            strings.subLive,
            "${status?.apps?.totalConfigured ?: 0} ${strings.activeOverridesSuffix}",
            strings.subSettings,
        )

        val navTabs = listOf(
            NavTabSpec(0, MiuixIcons.Home, strings.tabHome),
            NavTabSpec(
                1,
                MiuixIcons.Tune,
                strings.tabControl,
                badgeCount = if (navBadgesEnabled) (status?.semanticKnobsActive ?: 0) else 0,
            ),
            NavTabSpec(2, MiuixIcons.MindMap, strings.tabLive),
            NavTabSpec(
                3,
                MiuixIcons.ListView,
                strings.tabApps,
                badgeCount = if (navBadgesEnabled) (status?.apps?.totalConfigured ?: 0) else 0,
            ),
            NavTabSpec(4, MiuixIcons.Settings, strings.tabSettings),
        )

        Scaffold(
            topBar = {
                TopAppBar(
                    title = tabTitles[selectedTab],
                    subtitle = tabSubtitles[selectedTab],
                    scrollBehavior = scrollBehavior,
                    actions = {
                        IconButton(
                            onClick = { triggerPullRefresh() },
                            modifier = Modifier.padding(end = 6.dp),
                        ) {
                            Icon(
                                imageVector = MiuixIcons.Refresh,
                                contentDescription = "Refresh",
                                tint = MiuixTheme.colorScheme.onSurface,
                            )
                        }
                    },
                )
            },
            bottomBar = {
                if (floatingNavBar) {
                    HyperOsFloatingPillNavBar(
                        tabs = navTabs,
                        selectedIndex = selectedTab,
                        onSelect = { selectedTab = it },
                    )
                } else {
                    NavigationBar {
                        for (tab in navTabs) {
                            NavigationBarItem(
                                selected = selectedTab == tab.index,
                                onClick = { selectedTab = tab.index },
                                icon = tab.icon,
                                label = tab.label,
                            )
                        }
                    }
                }
            },
            snackbarHost = { SnackbarHost(state = snackbarHostState) },
        ) { innerPadding ->
            Box(modifier = Modifier.fillMaxSize()) {
                when (selectedTab) {
                    0 -> HomeTab(
                        strings = strings,
                        hasRoot = hasRoot,
                        status = status,
                        lastError = lastError,
                        busy = busyAction,
                        isRefreshing = isManualRefreshing,
                        contentPadding = innerPadding,
                        scrollBehavior = scrollBehavior,
                        onRefresh = { triggerPullRefresh() },
                        onSelectProfile = { profile ->
                            scope.launch {
                                busyAction = true
                                CoreFlowManager.applyProfile(profile)
                                    .onSuccess {
                                        refreshStatus()
                                        snackbarHostState.showSnackbar("Profile switched to ${profile.label}")
                                    }
                                    .onFailure { err ->
                                        snackbarHostState.showSnackbar(err.message ?: "Failed to apply profile")
                                    }
                                busyAction = false
                            }
                        },
                        onTogglePause = { paused ->
                            scope.launch {
                                busyAction = true
                                CoreFlowManager.setPaused(paused)
                                    .onSuccess {
                                        refreshStatus()
                                        snackbarHostState.showSnackbar(
                                            if (paused) "CoreFlow paused (OEM A/B mode)" else "CoreFlow resumed",
                                        )
                                    }
                                    .onFailure { err ->
                                        snackbarHostState.showSnackbar(err.message ?: "Failed to toggle pause")
                                    }
                                busyAction = false
                            }
                        },
                        onToggleBypass = { enabled ->
                            scope.launch {
                                busyAction = true
                                CoreFlowManager.setPowerControl("bypass_charging", if (enabled) "true" else "false")
                                    .onSuccess {
                                        refreshStatus()
                                        snackbarHostState.showSnackbar(
                                            if (enabled) "Bypass Charging enabled (CFJ4)" else "Bypass Charging disabled",
                                        )
                                    }
                                    .onFailure { err ->
                                        snackbarHostState.showSnackbar(err.message ?: "Failed to toggle bypass")
                                    }
                                busyAction = false
                            }
                        },
                        onStartDaemon = {
                            scope.launch {
                                busyAction = true
                                CoreFlowManager.startDaemon()
                                    .onSuccess {
                                        delay(900L)
                                        refreshStatus()
                                        snackbarHostState.showSnackbar("Starting CoreFlow daemon...")
                                    }
                                    .onFailure { err ->
                                        snackbarHostState.showSnackbar(err.message ?: "Failed to start daemon")
                                    }
                                busyAction = false
                            }
                        },
                    )

                    1 -> ControlHubTab(
                        strings = strings,
                        status = status,
                        busy = busyAction,
                        isRefreshing = isManualRefreshing,
                        contentPadding = innerPadding,
                        scrollBehavior = scrollBehavior,
                        onRefresh = { triggerPullRefresh() },
                        onToggleFeature = { key, label, enabled ->
                            scope.launch {
                                busyAction = true
                                CoreFlowManager.setFeatureToggle(key, enabled)
                                    .onSuccess {
                                        refreshStatus()
                                        snackbarHostState.showSnackbar("$label ${if (enabled) "enabled" else "disabled"}")
                                    }
                                    .onFailure { err ->
                                        snackbarHostState.showSnackbar(err.message ?: "Failed to update $label")
                                    }
                                busyAction = false
                            }
                        },
                        onSetPowerControl = { key, value, msg ->
                            scope.launch {
                                busyAction = true
                                CoreFlowManager.setPowerControl(key, value)
                                    .onSuccess {
                                        refreshStatus()
                                        snackbarHostState.showSnackbar(msg)
                                    }
                                    .onFailure { err ->
                                        snackbarHostState.showSnackbar(err.message ?: "Failed to update power control")
                                    }
                                busyAction = false
                            }
                        },
                        onMessage = { msg ->
                            scope.launch { snackbarHostState.showSnackbar(msg) }
                        },
                    )

                    2 -> LiveFeedTab(
                        strings = strings,
                        status = status,
                        isRefreshing = isManualRefreshing,
                        contentPadding = innerPadding,
                        scrollBehavior = scrollBehavior,
                        onRefresh = { triggerPullRefresh() },
                    )

                    3 -> AppsTab(
                        strings = strings,
                        apps = installedApps,
                        loading = appsLoading,
                        globalProfile = status?.profile ?: CoreFlowManager.Profile.BALANCED,
                        bypassOnGaming = status?.powerConfig?.bypassOnGaming == true,
                        rules = status?.apps ?: CoreFlowManager.AppRules(),
                        isRefreshing = isManualRefreshing,
                        contentPadding = innerPadding,
                        scrollBehavior = scrollBehavior,
                        onReloadApps = {
                            scope.launch {
                                appsLoading = true
                                installedApps = AppCatalog.loadLaunchableApps(context)
                                refreshStatus()
                                appsLoading = false
                            }
                        },
                        onAssignBucket = { pkg, bucket ->
                            val currentRules = status?.apps ?: CoreFlowManager.AppRules()
                            scope.launch {
                                CoreFlowManager.setAppRule(pkg, bucket, currentRules)
                                    .onSuccess {
                                        refreshStatus()
                                        snackbarHostState.showSnackbar("$pkg -> ${bucket.label}")
                                    }
                                    .onFailure { err ->
                                        snackbarHostState.showSnackbar(err.message ?: "Failed to save app rule")
                                    }
                            }
                        },
                    )

                    4 -> SystemTab(
                        strings = strings,
                        status = status,
                        journalText = journalText,
                        busy = busyAction,
                        isRefreshing = isManualRefreshing,
                        themeModeIndex = themeModeIndex,
                        pollIntervalIndex = pollIntervalIndex,
                        languageIndex = languageIndex,
                        floatingNavBar = floatingNavBar,
                        navBadgesEnabled = navBadgesEnabled,
                        contentPadding = innerPadding,
                        scrollBehavior = scrollBehavior,
                        onRefresh = { triggerPullRefresh() },
                        onThemeModeChange = { newIdx ->
                            onThemeModeChange(newIdx)
                            scope.launch {
                                snackbarHostState.showSnackbar("Theme: ${AppPreferences.themeModeLabels[newIdx]}")
                            }
                        },
                        onPollIntervalChange = { newIdx ->
                            pollIntervalIndex = newIdx
                            AppPreferences.setPollIntervalIndex(context, newIdx)
                            scope.launch {
                                snackbarHostState.showSnackbar("Refresh rate: ${AppPreferences.pollIntervalLabels[newIdx]}")
                            }
                        },
                        onLanguageChange = { newIdx ->
                            languageIndex = newIdx
                            AppPreferences.setLanguageIndex(context, newIdx)
                            scope.launch {
                                snackbarHostState.showSnackbar("Language: ${AppPreferences.languageLabels[newIdx]}")
                            }
                        },
                        onFloatingNavBarChange = { enabled ->
                            floatingNavBar = enabled
                            AppPreferences.setFloatingNavBar(context, enabled)
                        },
                        onNavBadgesChange = { enabled ->
                            navBadgesEnabled = enabled
                            AppPreferences.setNavBadgesEnabled(context, enabled)
                        },
                        onExportProfile = {
                            exportProfileLauncher.launch("coreflow-profile-v6.0.0.json")
                        },
                        onImportProfile = {
                            importProfileLauncher.launch(arrayOf("application/json", "text/plain", "*/*"))
                        },
                        onLoadJournal = {
                            scope.launch {
                                busyAction = true
                                CoreFlowManager.getJournalJson()
                                    .onSuccess { journalText = it }
                                    .onFailure { err ->
                                        snackbarHostState.showSnackbar(err.message ?: "Failed to read CFJ4 journal")
                                    }
                                busyAction = false
                            }
                        },
                        onShareDiag = {
                            scope.launch {
                                busyAction = true
                                CoreFlowManager.getDiagJson()
                                    .onSuccess { diagJson ->
                                        runCatching {
                                            val dir = File(context.cacheDir, "diag").apply { mkdirs() }
                                            val outFile = File(dir, "coreflow-diag-v6.0.0.json")
                                            outFile.writeText(diagJson)
                                            val uri = FileProvider.getUriForFile(
                                                context,
                                                "${context.packageName}.fileprovider",
                                                outFile,
                                            )
                                            val shareIntent = Intent(Intent.ACTION_SEND).apply {
                                                type = "application/json"
                                                putExtra(Intent.EXTRA_STREAM, uri)
                                                putExtra(Intent.EXTRA_SUBJECT, "CoreFlow Engine v6.0.0 Diagnostics")
                                                addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
                                            }
                                            context.startActivity(
                                                Intent.createChooser(shareIntent, "Share CoreFlow Diagnostics"),
                                            )
                                        }.onFailure { err ->
                                            snackbarHostState.showSnackbar(err.message ?: "Failed to share diag bundle")
                                        }
                                    }
                                    .onFailure { err ->
                                        snackbarHostState.showSnackbar(err.message ?: "Failed to export --diag")
                                    }
                                busyAction = false
                            }
                        },
                        onResetOemMap = {
                            scope.launch {
                                busyAction = true
                                CoreFlowManager.resetOemCollaborationMap()
                                    .onSuccess {
                                        refreshStatus()
                                        snackbarHostState.showSnackbar(
                                            if (strings.isIndonesian) {
                                                "Peta Kolaborasi OEM direset — aktif penuh setelah restart daemon"
                                            } else {
                                                "OEM Collaboration Map cleared — takes full effect on next daemon cycle"
                                            },
                                        )
                                    }
                                    .onFailure { err ->
                                        snackbarHostState.showSnackbar(err.message ?: "Failed to reset OEM map")
                                    }
                                busyAction = false
                            }
                        },
                        onRollback = {
                            scope.launch {
                                busyAction = true
                                CoreFlowManager.rollbackBaseline()
                                    .onSuccess {
                                        refreshStatus()
                                        snackbarHostState.showSnackbar("OEM baseline restored via CFJ4")
                                    }
                                    .onFailure { err ->
                                        snackbarHostState.showSnackbar(err.message ?: "Rollback failed")
                                    }
                                busyAction = false
                            }
                        },
                        onOpenAbout = { showAboutScreen = true },
                    )
                }
            }
        }
    }
}

/**
 * Authentic HyperOS 4 / KernelSU floating frosted-glass capsule bottom navigation bar.
 */
@Composable
private fun HyperOsFloatingPillNavBar(
    tabs: List<NavTabSpec>,
    selectedIndex: Int,
    onSelect: (Int) -> Unit,
) {
    val dark = isSystemInDarkTheme()
    val pillShape = RoundedCornerShape(999.dp)
    val glassBg = if (dark) {
        Color(0xFF1A1B20).copy(alpha = 0.88f)
    } else {
        MiuixTheme.colorScheme.surfaceContainer.copy(alpha = 0.92f)
    }
    val rimColor = if (dark) {
        Color.White.copy(alpha = 0.14f)
    } else {
        Color.Black.copy(alpha = 0.08f)
    }

    Box(
        modifier = Modifier
            .fillMaxWidth()
            .navigationBarsPadding()
            .padding(horizontal = 16.dp, vertical = 10.dp),
        contentAlignment = Alignment.Center,
    ) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .shadow(
                    elevation = 14.dp,
                    shape = pillShape,
                    ambientColor = Color.Black.copy(alpha = 0.35f),
                    spotColor = Color.Black.copy(alpha = 0.35f),
                )
                .clip(pillShape)
                .background(glassBg)
                .border(BorderStroke(0.8.dp, rimColor), pillShape)
                .padding(horizontal = 6.dp, vertical = 5.dp),
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.spacedBy(2.dp),
        ) {
            for (tab in tabs) {
                val selected = tab.index == selectedIndex
                val activeIndicator by animateColorAsState(
                    targetValue = if (selected) {
                        if (dark) Color.White.copy(alpha = 0.12f) else MiuixTheme.colorScheme.primary.copy(alpha = 0.12f)
                    } else {
                        Color.Transparent
                    },
                    animationSpec = tween(180),
                    label = "pillBg",
                )
                val contentTint by animateColorAsState(
                    targetValue = if (selected) {
                        MiuixTheme.colorScheme.primary
                    } else {
                        MiuixTheme.colorScheme.onSurfaceVariantSummary
                    },
                    animationSpec = tween(180),
                    label = "pillTint",
                )

                Column(
                    modifier = Modifier
                        .weight(1f)
                        .clip(pillShape)
                        .background(activeIndicator)
                        .clickable(
                            interactionSource = remember { MutableInteractionSource() },
                            indication = null,
                            onClick = { onSelect(tab.index) },
                        )
                        .padding(vertical = 6.dp, horizontal = 2.dp),
                    horizontalAlignment = Alignment.CenterHorizontally,
                    verticalArrangement = Arrangement.Center,
                ) {
                    Box(contentAlignment = Alignment.TopEnd) {
                        Icon(
                            imageVector = tab.icon,
                            contentDescription = tab.label,
                            tint = contentTint,
                            modifier = Modifier.size(21.dp),
                        )
                        if (tab.badgeCount > 0) {
                            Box(
                                modifier = Modifier
                                    .offset(x = 7.dp, y = (-4).dp)
                                    .defaultMinSize(minWidth = 15.dp, minHeight = 15.dp)
                                    .clip(CircleShape)
                                    .background(MiuixTheme.colorScheme.primary)
                                    .padding(horizontal = 4.dp),
                                contentAlignment = Alignment.Center,
                            ) {
                                Text(
                                    text = if (tab.badgeCount > 99) "99" else "${tab.badgeCount}",
                                    fontSize = 8.sp,
                                    fontWeight = FontWeight.Bold,
                                    color = Color.White,
                                    maxLines = 1,
                                )
                            }
                        }
                    }
                    Spacer(Modifier.height(2.dp))
                    Text(
                        text = tab.label,
                        fontSize = 10.sp,
                        fontWeight = if (selected) FontWeight.SemiBold else FontWeight.Medium,
                        color = contentTint,
                        maxLines = 1,
                        softWrap = false,
                        overflow = TextOverflow.Ellipsis,
                    )
                }
            }
        }
    }
}
