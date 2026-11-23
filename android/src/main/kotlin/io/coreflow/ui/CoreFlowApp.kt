package io.coreflow.ui

import android.content.Intent
import android.net.Uri
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
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.luminance
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.hapticfeedback.HapticFeedbackType
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalHapticFeedback
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.core.content.FileProvider
import io.coreflow.AppCatalog
import io.coreflow.AppPreferences
import io.coreflow.BuildConfig
import io.coreflow.CoreFlowManager
import io.coreflow.CoreFlowUpdateChecker
import io.coreflow.InstalledAppEntry
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
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
import top.yukonga.miuix.kmp.icon.extended.Refresh
import top.yukonga.miuix.kmp.icon.extended.Settings
import top.yukonga.miuix.kmp.icon.extended.Stopwatch
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
    pendingTab: Int? = null,
    pendingDialoguePrompt: String? = null,
    onConsumeDeepLink: () -> Unit = {},
) {
    val context = LocalContext.current
    val scope = rememberCoroutineScope()
    val snackbarHostState = remember { SnackbarHostState() }

    var selectedTab by rememberSaveable { mutableIntStateOf(0) }
    var activeDialoguePrompt by remember { mutableStateOf<String?>(null) }
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
    var selfAuditReport by remember { mutableStateOf<CoreFlowManager.SelfAuditReport?>(null) }
    var auditBusy by remember { mutableStateOf(false) }
    var updateReport by remember { mutableStateOf<CoreFlowUpdateChecker.UpdateCheckReport?>(null) }
    var updateBusy by remember { mutableStateOf(false) }
    var neuralModelInfo by remember { mutableStateOf(CoreFlowManager.InstalledNeuralModelInfo()) }

    LaunchedEffect(pendingTab, pendingDialoguePrompt) {
        if (pendingTab != null || pendingDialoguePrompt != null) {
            pendingTab?.let { if (it in 0..4) selectedTab = it }
            if (!pendingDialoguePrompt.isNullOrBlank()) {
                activeDialoguePrompt = pendingDialoguePrompt
            }
            showAboutScreen = false
            onConsumeDeepLink()
        }
    }

    suspend fun runUpdateCheck(forceRefresh: Boolean, notifyUser: Boolean) {
        if (updateBusy) return
        updateBusy = true
        runCatching {
            val modMeta = CoreFlowManager.getInstalledModuleMeta(status?.version)
            val installed = CoreFlowUpdateChecker.InstalledSnapshot(
                appVersion = BuildConfig.VERSION_NAME,
                appVersionCode = BuildConfig.VERSION_CODE,
                moduleVersion = modMeta.version,
                moduleVersionCode = modMeta.versionCode,
                moduleInstalled = modMeta.installed,
            )
            CoreFlowUpdateChecker.checkForUpdates(context, installed, forceRefresh = forceRefresh)
        }.onSuccess { report ->
            updateReport = report
            if (notifyUser) {
                val msg = when (report.state) {
                    CoreFlowUpdateChecker.UpdateState.UP_TO_DATE -> strings.updateUpToDate
                    CoreFlowUpdateChecker.UpdateState.UPDATE_AVAILABLE ->
                        "${strings.updateAvailable}: v${report.latestVersion}"
                    CoreFlowUpdateChecker.UpdateState.PENDING_PUBLIC_RELEASE ->
                        "${strings.updatePendingRelease} (v${report.latestVersion})"
                    CoreFlowUpdateChecker.UpdateState.ERROR ->
                        report.errorMessage ?: strings.updateError
                }
                snackbarHostState.showSnackbar(msg)
            }
        }.onFailure { err ->
            if (notifyUser) {
                snackbarHostState.showSnackbar(err.message ?: strings.updateError)
            }
        }
        updateBusy = false
    }

    fun openTrustedUrl(rawUrl: String) {
        if (!CoreFlowUpdateChecker.isTrustedUrl(rawUrl)) {
            scope.launch { snackbarHostState.showSnackbar("Blocked untrusted URL") }
            return
        }
        runCatching {
            val intent = Intent(Intent.ACTION_VIEW, Uri.parse(rawUrl)).apply {
                addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
            }
            context.startActivity(intent)
        }.onFailure { err ->
            scope.launch { snackbarHostState.showSnackbar(err.message ?: "Unable to open browser") }
        }
    }

    LaunchedEffect(Unit) {
        delay(1200L)
        runUpdateCheck(forceRefresh = false, notifyUser = false)
    }

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
            if (selectedTab == 4) {
                neuralModelInfo = CoreFlowManager.getInstalledNeuralModelInfo()
            }
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

    // SAF CFW2 Micro-Model Import Launcher (bounded <= 16 KB read + CRC32 validation before root install)
    val importCfw2Launcher = rememberLauncherForActivityResult(
        contract = ActivityResultContracts.OpenDocument(),
    ) { uri ->
        if (uri == null) return@rememberLauncherForActivityResult
        scope.launch {
            busyAction = true
            runCatching {
                val raw = withContext(Dispatchers.IO) {
                    context.contentResolver.openInputStream(uri)?.use { stream ->
                        val out = java.io.ByteArrayOutputStream()
                        val chunk = ByteArray(4096)
                        var total = 0
                        while (total <= 16384) {
                            val n = stream.read(chunk)
                            if (n <= 0) break
                            out.write(chunk, 0, n)
                            total += n
                        }
                        if (total > 16384) error("File exceeds 16 KB CFW2 safety cap")
                        out.toByteArray()
                    } ?: error("Cannot read CFW2 file")
                }
                CoreFlowManager.installCfw2Model(raw).getOrThrow()
            }.onSuccess { info ->
                delay(600L)
                neuralModelInfo = CoreFlowManager.getInstalledNeuralModelInfo()
                refreshStatus()
                snackbarHostState.showSnackbar(
                    "CFW2 installed (${info.sizeBytes} B, ${info.activation}, ${info.crc32Hex})",
                )
            }.onFailure { err ->
                snackbarHostState.showSnackbar(err.message ?: "CFW2 import failed")
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
        if (selectedTab == 4) {
            neuralModelInfo = CoreFlowManager.getInstalledNeuralModelInfo()
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
            } ?: "Zero-Profile Autonomous",
            strings.subControl,
            strings.subLive,
            "${status?.apps?.excluded?.size ?: 0} ${strings.activeOverridesSuffix}",
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
            NavTabSpec(2, MiuixIcons.Stopwatch, strings.tabLive),
            NavTabSpec(
                3,
                MiuixIcons.ListView,
                strings.tabApps,
                badgeCount = if (navBadgesEnabled) (status?.apps?.excluded?.size ?: 0) else 0,
            ),
            NavTabSpec(
                4,
                MiuixIcons.Settings,
                strings.tabSettings,
                badgeCount = if (navBadgesEnabled && (updateReport?.state == CoreFlowUpdateChecker.UpdateState.UPDATE_AVAILABLE ||
                        updateReport?.state == CoreFlowUpdateChecker.UpdateState.PENDING_PUBLIC_RELEASE)
                ) {
                    1
                } else {
                    0
                },
            ),
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
                        onTogglePause = { paused ->
                            scope.launch {
                                busyAction = true
                                CoreFlowManager.setPaused(paused)
                                    .onSuccess {
                                        refreshStatus()
                                        snackbarHostState.showSnackbar(
                                            if (paused) "CoreFlow paused (OEM A/B mode)" else "CoreFlow autonomous resumed",
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
                        onOpenDialogue = { prompt ->
                            if (!prompt.isNullOrBlank()) {
                                activeDialoguePrompt = prompt
                            }
                            selectedTab = 2
                        },
                        updateReport = updateReport,
                        onOpenUpdateCenter = { selectedTab = 4 },
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
                        pendingPrompt = activeDialoguePrompt,
                        onConsumePrompt = { activeDialoguePrompt = null },
                        onTogglePause = { paused ->
                            scope.launch {
                                busyAction = true
                                CoreFlowManager.setPaused(paused)
                                    .onSuccess {
                                        refreshStatus()
                                        snackbarHostState.showSnackbar(
                                            if (paused) "CoreFlow paused (OEM A/B mode)" else "CoreFlow autonomous resumed",
                                        )
                                    }
                                    .onFailure { err ->
                                        snackbarHostState.showSnackbar(err.message ?: "Failed to toggle pause")
                                    }
                                busyAction = false
                            }
                        },
                        onResolveTamper = { nodePath ->
                            scope.launch {
                                busyAction = true
                                CoreFlowManager.resolveTamper(nodePath, "RESYNC")
                                    .onSuccess {
                                        refreshStatus()
                                        snackbarHostState.showSnackbar("CFAE Sovereign RESYNC verified")
                                    }
                                    .onFailure { err ->
                                        snackbarHostState.showSnackbar(err.message ?: "Failed to resync node")
                                    }
                                busyAction = false
                            }
                        },
                    )

                    3 -> AppsTab(
                        strings = strings,
                        apps = installedApps,
                        loading = appsLoading,
                        bypassOnGaming = status?.powerConfig?.bypassOnGaming == true,
                        excludedApps = status?.apps?.excluded.orEmpty(),
                        appDnaEntries = status?.appDnaEntries.orEmpty(),
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
                        onToggleExcluded = { pkg, excluded ->
                            val currentExcluded = status?.apps?.excluded.orEmpty()
                            scope.launch {
                                CoreFlowManager.setAppExcluded(pkg, excluded, currentExcluded)
                                    .onSuccess {
                                        refreshStatus()
                                        snackbarHostState.showSnackbar(
                                            if (excluded) "$pkg -> Stock OEM (Excluded)" else "$pkg -> Autonomous",
                                        )
                                    }
                                    .onFailure { err ->
                                        snackbarHostState.showSnackbar(err.message ?: "Failed to update exclusion")
                                    }
                            }
                        },
                    )

                    4 -> SystemTab(
                        strings = strings,
                        status = status,
                        updateReport = updateReport,
                        updateBusy = updateBusy,
                        journalText = journalText,
                        selfAuditReport = selfAuditReport,
                        auditBusy = auditBusy,
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
                        onCheckUpdates = {
                            scope.launch { runUpdateCheck(forceRefresh = true, notifyUser = true) }
                        },
                        onOpenTrustedUrl = { url -> openTrustedUrl(url) },
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
                            exportProfileLauncher.launch("coreflow-profile-v${BuildConfig.VERSION_NAME}.json")
                        },
                        onImportProfile = {
                            importProfileLauncher.launch(arrayOf("application/json", "text/plain", "*/*"))
                        },
                        neuralModelInfo = neuralModelInfo,
                        onImportCfw2 = {
                            importCfw2Launcher.launch(arrayOf("application/octet-stream", "*/*"))
                        },
                        onRemoveCfw2 = {
                            scope.launch {
                                busyAction = true
                                CoreFlowManager.removeCustomCfw2Model()
                                    .onSuccess {
                                        delay(600L)
                                        neuralModelInfo = CoreFlowManager.getInstalledNeuralModelInfo()
                                        refreshStatus()
                                        snackbarHostState.showSnackbar("Reverted to built-in Embedded RLS-MLP")
                                    }
                                    .onFailure { err ->
                                        snackbarHostState.showSnackbar(err.message ?: "Failed to remove CFW2")
                                    }
                                busyAction = false
                            }
                        },
                        onRunSelfAudit = {
                            scope.launch {
                                auditBusy = true
                                CoreFlowManager.runSelfAudit(status, strings.isIndonesian)
                                    .onSuccess { report ->
                                        selfAuditReport = report
                                        snackbarHostState.showSnackbar(
                                            if (report.passedAll) strings.auditPassAll else strings.auditAttention,
                                        )
                                    }
                                    .onFailure { err ->
                                        snackbarHostState.showSnackbar(err.message ?: "Self-audit failed")
                                    }
                                auditBusy = false
                            }
                        },
                        onShareSelfAudit = { summary ->
                            runCatching {
                                val intent = Intent(Intent.ACTION_SEND).apply {
                                    type = "text/plain"
                                    putExtra(Intent.EXTRA_SUBJECT, "CoreFlow Engine v${BuildConfig.VERSION_NAME} Health Audit")
                                    putExtra(Intent.EXTRA_TEXT, summary)
                                }
                                context.startActivity(Intent.createChooser(intent, strings.shareAudit))
                            }.onFailure { err ->
                                scope.launch {
                                    snackbarHostState.showSnackbar(err.message ?: "Failed to share audit")
                                }
                            }
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
                                            val outFile = File(dir, "coreflow-diag-v${BuildConfig.VERSION_NAME}.json")
                                            outFile.writeText(diagJson)
                                            val uri = FileProvider.getUriForFile(
                                                context,
                                                "${context.packageName}.fileprovider",
                                                outFile,
                                            )
                                            val shareIntent = Intent(Intent.ACTION_SEND).apply {
                                                type = "application/json"
                                                putExtra(Intent.EXTRA_STREAM, uri)
                                                putExtra(Intent.EXTRA_SUBJECT, "CoreFlow Engine v${BuildConfig.VERSION_NAME} Diagnostics")
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
    val haptic = LocalHapticFeedback.current
    val dark = MiuixTheme.colorScheme.surface.luminance() < 0.5f
    val surface = MiuixTheme.colorScheme.surface
    val pillShape = RoundedCornerShape(999.dp)
    val glassBg = if (dark) {
        Color(0xFF1A1C22).copy(alpha = 0.94f)
    } else {
        MiuixTheme.colorScheme.surfaceContainer.copy(alpha = 0.96f)
    }
    val rimColor = if (dark) {
        Color.White.copy(alpha = 0.14f)
    } else {
        Color.Black.copy(alpha = 0.08f)
    }

    Box(
        modifier = Modifier
            .fillMaxWidth()
            .background(
                Brush.verticalGradient(
                    colors = listOf(
                        Color.Transparent,
                        surface.copy(alpha = 0.72f),
                        surface.copy(alpha = 0.94f),
                    ),
                ),
            )
            .navigationBarsPadding()
            .padding(horizontal = 16.dp, vertical = 10.dp),
        contentAlignment = Alignment.Center,
    ) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .shadow(
                    elevation = 16.dp,
                    shape = pillShape,
                    ambientColor = Color.Black.copy(alpha = 0.40f),
                    spotColor = Color.Black.copy(alpha = 0.40f),
                )
                .clip(pillShape)
                .background(glassBg)
                .border(BorderStroke(0.85.dp, rimColor), pillShape)
                .padding(horizontal = 6.dp, vertical = 5.dp),
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.spacedBy(2.dp),
        ) {
            for (tab in tabs) {
                val selected = tab.index == selectedIndex
                val activeIndicator by animateColorAsState(
                    targetValue = if (selected) {
                        if (dark) Color.White.copy(alpha = 0.13f) else MiuixTheme.colorScheme.primary.copy(alpha = 0.13f)
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
                            onClick = {
                                if (!selected) {
                                    haptic.performHapticFeedback(HapticFeedbackType.TextHandleMove)
                                    onSelect(tab.index)
                                }
                            },
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
