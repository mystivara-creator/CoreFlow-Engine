package io.coreflow.ui

import android.content.Intent
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
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
import top.yukonga.miuix.kmp.basic.TopAppBar
import top.yukonga.miuix.kmp.icon.MiuixIcons
import top.yukonga.miuix.kmp.icon.extended.GridView
import top.yukonga.miuix.kmp.icon.extended.Info
import top.yukonga.miuix.kmp.icon.extended.ListView
import top.yukonga.miuix.kmp.icon.extended.Refresh
import top.yukonga.miuix.kmp.icon.extended.Settings
import top.yukonga.miuix.kmp.icon.extended.Tune
import top.yukonga.miuix.kmp.theme.MiuixTheme
import java.io.File

@Composable
fun CoreFlowApp(
    themeModeIndex: Int,
    onThemeModeChange: (Int) -> Unit,
) {
    val context = LocalContext.current
    val scope = rememberCoroutineScope()
    val snackbarHostState = remember { SnackbarHostState() }

    var selectedTab by rememberSaveable { mutableIntStateOf(0) }
    var pollIntervalIndex by remember { mutableIntStateOf(AppPreferences.getPollIntervalIndex(context)) }
    var languageIndex by remember { mutableIntStateOf(AppPreferences.getLanguageIndex(context)) }
    val strings = rememberAppStrings(languageIndex)

    var hasRoot by remember { mutableStateOf<Boolean?>(null) }
    var status by remember { mutableStateOf<CoreFlowManager.DaemonStatus?>(null) }
    var lastError by remember { mutableStateOf<String?>(null) }
    var busyAction by remember { mutableStateOf(false) }

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

    val scrollBehavior = MiuixScrollBehavior()
    val tabTitles = listOf(
        strings.titleHome,
        strings.titleControl,
        strings.titleLive,
        strings.titleApps,
        strings.titleSettings,
    )
    val tabSubtitles = listOf(
        status?.let { "v${it.version} · ${if (it.isRunning) "PID ${it.pid}" else strings.statusStopped}" } ?: "CFAE NextGen",
        strings.subControl,
        strings.subLive,
        "${status?.apps?.totalConfigured ?: 0} ${strings.activeOverridesSuffix}",
        strings.subSettings,
    )

    Scaffold(
        topBar = {
            TopAppBar(
                title = tabTitles[selectedTab],
                subtitle = tabSubtitles[selectedTab],
                scrollBehavior = scrollBehavior,
                actions = {
                    IconButton(
                        onClick = {
                            scope.launch {
                                hasRoot = CoreFlowManager.checkRootAccess()
                                refreshStatus()
                                snackbarHostState.showSnackbar("Telemetry refreshed")
                            }
                        },
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
            NavigationBar {
                NavigationBarItem(
                    selected = selectedTab == 0,
                    onClick = { selectedTab = 0 },
                    icon = MiuixIcons.GridView,
                    label = strings.tabHome,
                )
                NavigationBarItem(
                    selected = selectedTab == 1,
                    onClick = { selectedTab = 1 },
                    icon = MiuixIcons.Tune,
                    label = strings.tabControl,
                )
                NavigationBarItem(
                    selected = selectedTab == 2,
                    onClick = { selectedTab = 2 },
                    icon = MiuixIcons.Info,
                    label = strings.tabLive,
                )
                NavigationBarItem(
                    selected = selectedTab == 3,
                    onClick = { selectedTab = 3 },
                    icon = MiuixIcons.ListView,
                    label = strings.tabApps,
                )
                NavigationBarItem(
                    selected = selectedTab == 4,
                    onClick = { selectedTab = 4 },
                    icon = MiuixIcons.Settings,
                    label = strings.tabSettings,
                )
            }
        },
        snackbarHost = { SnackbarHost(state = snackbarHostState) },
    ) { innerPadding ->
        Box(
            modifier = Modifier
                .fillMaxSize()
                .padding(innerPadding),
        ) {
            when (selectedTab) {
                0 -> HomeTab(
                    strings = strings,
                    hasRoot = hasRoot,
                    status = status,
                    lastError = lastError,
                    busy = busyAction,
                    scrollBehavior = scrollBehavior,
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
                    scrollBehavior = scrollBehavior,
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
                    scrollBehavior = scrollBehavior,
                )

                3 -> AppsTab(
                    strings = strings,
                    apps = installedApps,
                    loading = appsLoading,
                    globalProfile = status?.profile ?: CoreFlowManager.Profile.BALANCED,
                    bypassOnGaming = status?.powerConfig?.bypassOnGaming == true,
                    rules = status?.apps ?: CoreFlowManager.AppRules(),
                    scrollBehavior = scrollBehavior,
                    onReloadApps = {
                        scope.launch {
                            appsLoading = true
                            installedApps = AppCatalog.loadLaunchableApps(context)
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
                    themeModeIndex = themeModeIndex,
                    pollIntervalIndex = pollIntervalIndex,
                    languageIndex = languageIndex,
                    scrollBehavior = scrollBehavior,
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
                )
            }
        }
    }
}
