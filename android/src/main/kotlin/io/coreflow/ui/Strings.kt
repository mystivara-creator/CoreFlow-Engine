package io.coreflow.ui

import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import java.util.Locale

data class AppStrings(
    val isIndonesian: Boolean,
    val tabHome: String,
    val tabControl: String,
    val tabLive: String,
    val tabApps: String,
    val tabSettings: String,
    val titleHome: String,
    val titleControl: String,
    val titleLive: String,
    val titleApps: String,
    val titleSettings: String,
    val subControl: String,
    val subLive: String,
    val subSettings: String,
    val statusStopped: String,
    val activeOverridesSuffix: String,
    val sectionProfile: String,
    val sectionSparkline: String,
    val sectionSession: String,
    val pauseCompare: String,
    val resumeEngine: String,
    val startEngine: String,
    val bypassOn: String,
    val bypassOff: String,
    val sectionCpu: String,
    val sectionGpuThermal: String,
    val sectionPower: String,
    val sectionMemStorage: String,
    val sectionHudTiles: String,
    val sectionAiBrain: String,
    val sectionEpisodes: String,
    val emptyEpisodes: String,
    val labelForecast10s: String,
    val labelConfidence: String,
    val labelCpuLoad: String,
    val labelTrend: String,
    val trendHeating: String,
    val trendCooling: String,
    val trendStable: String,
    val statusAdaptive: String,
    val statusOemYield: String,
    val statusFallback: String,
    val searchAppsHint: String,
    val filterAll: String,
    val filterGames: String,
    val filterRules: String,
    val installedAppsHeader: String,
    val loadingApps: String,
    val noMatchingApps: String,
    val reloadApps: String,
    val effectiveGlobal: String,
    val effectiveOverride: String,
    val autoBypassHint: String,
    val resetShort: String,
    val sectionAppSettings: String,
    val sectionProfileTransfer: String,
    val sectionCoexistence: String,
    val sectionRecovery: String,
    val sectionCreator: String,
    val sectionSponsors: String,
    val sectionLicense: String,
    val exportProfileTitle: String,
    val exportProfileSummary: String,
    val importProfileTitle: String,
    val importProfileSummary: String,
    val pullToRefreshTexts: List<String>,
    val sectionCopilot: String,
    val copilotStandby: String,
    val copilotActive: String,
    val oemCooperatingTitle: String,
    val oemCooperatingSummary: String,
    val resetOemMap: String,
    val floatingBarTitle: String,
    val floatingBarSummary: String,
    val navBadgesTitle: String,
    val navBadgesSummary: String,
    val aboutTitle: String,
    val aboutSummary: String,
    val aboutViewGithub: String,
    val aboutCreatorProfile: String,
    val sectionSelfAudit: String,
    val runSelfAudit: String,
    val runningAudit: String,
    val shareAudit: String,
    val auditPassAll: String,
    val auditAttention: String,
    val labelFramePacing: String,
    val labelAppDna: String,
    val labelThermalEq: String,
    val sectionDialogueConsole: String,
    val dialogueInputHint: String,
    val dialogueSend: String,
    val dialogueQuickCardTitle: String,
    val dialogueQuickCardSummary: String,
    val sectionUpdateCenter: String,
    val updateCheckNow: String,
    val updateChecking: String,
    val updateUpToDate: String,
    val updateAvailable: String,
    val updatePendingRelease: String,
    val updateError: String,
    val updateDownloadApk: String,
    val updateDownloadModule: String,
    val updateOpenReleases: String,
    val updateModuleLabel: String,
    val updateAppLabel: String,
    val updateSkewWarning: String,
    val updatePendingHint: String,
    val labelJankRisk: String,
    val labelWorkload: String,
    val runSessionAnalysis: String,
    val sectionNeuralModel: String,
    val importCfw2Title: String,
    val importCfw2Summary: String,
    val removeCfw2Title: String,
    val removeCfw2Summary: String,
) {
    fun humanizeAiBackend(raw: String): String = when (raw.lowercase(Locale.US)) {
        "onnx-runtime-live" -> "ONNX Live"
        "cfw2-micro-mlp" -> "CFW2 MLP"
        "custom-cfw-rls-mlp" -> "CFW1 MLP"
        else -> "Embedded RLS"
    }

    fun humanizeAiBackendCompact(raw: String): String = when (raw.lowercase(Locale.US)) {
        "onnx-runtime-live" -> "ONNX"
        "cfw2-micro-mlp" -> "CFW2"
        "custom-cfw-rls-mlp" -> "CFW1"
        else -> "RLS"
    }

    fun humanizeWorkload(raw: String): String = when (raw.lowercase(Locale.US)) {
        "thermal_surge" -> if (isIndonesian) "Lonjakan Suhu" else "Thermal Surge"
        "gpu_heavy" -> "GPU Heavy"
        "io_bound" -> "I/O Bound"
        "interactive" -> if (isIndonesian) "Interaktif" else "Interactive"
        "idle" -> if (isIndonesian) "Tenang" else "Idle"
        else -> if (isIndonesian) "Seimbang" else "Balanced"
    }

    fun humanizeMode(mode: String): String = when (mode.uppercase(Locale.US)) {
        "LAUNCH_BURST" -> if (isIndonesian) "Lonjakan Buka" else "Launch Burst"
        "INTERACTIVE" -> if (isIndonesian) "Interaktif" else "Interactive"
        "GAMING" -> "Gaming"
        "EFFICIENCY" -> if (isIndonesian) "Efisiensi" else "Efficiency"
        "BOOT_SETTLE" -> if (isIndonesian) "Tenang Pasca-Boot" else "Boot Settle"
        "SCREEN_OFF" -> if (isIndonesian) "Layar Mati" else "Screen Off"
        "EXCLUDED" -> if (isIndonesian) "OEM Standar" else "OEM Stock"
        "THERMAL_GUARD" -> if (isIndonesian) "Jaga Suhu" else "Thermal Guard"
        "THERMAL_EMERGENCY" -> if (isIndonesian) "Darurat Suhu" else "Thermal Cutoff"
        else -> mode.lowercase(Locale.US).replaceFirstChar { it.uppercaseChar() }
    }

    fun humanizeOutcome(outcome: String): String = when (outcome) {
        "oem_yield" -> "OEM Yield"
        "rolled_back" -> if (isIndonesian) "Dipulihkan" else "Rolled Back"
        "reclaimed" -> if (isIndonesian) "Direbut Balik" else "Reclaimed"
        "quarantined" -> if (isIndonesian) "Karantina 30d" else "Quarantined"
        "skipped" -> if (isIndonesian) "Dilewati" else "Skipped"
        else -> if (isIndonesian) "Terverifikasi" else "Verified"
    }

    fun humanizeThermalSource(raw: String, contendedCount: Int): String {
        if (contendedCount > 0) return "$statusOemYield ($contendedCount)"
        val lower = raw.lowercase(Locale.US)
        return when {
            lower.contains("fallback") || lower == "unavailable" -> statusFallback
            lower.contains("degraded") -> statusAdaptive
            else -> statusAdaptive
        }
    }

    fun formatTrend(velCpm: Float): String = when {
        velCpm >= 0.15f -> String.format(Locale.US, "+%.1f°/m", velCpm)
        velCpm <= -0.15f -> String.format(Locale.US, "%.1f°/m", velCpm)
        else -> trendStable
    }
}

@Composable
fun rememberAppStrings(languageIndex: Int): AppStrings {
    val isId = when (languageIndex) {
        1 -> false
        2 -> true
        else -> {
            val lang = Locale.getDefault().language.lowercase(Locale.US)
            lang == "in" || lang == "id"
        }
    }
    return remember(isId) {
        if (isId) {
            AppStrings(
                isIndonesian = true,
                tabHome = "Beranda",
                tabControl = "Kontrol",
                tabLive = "Live AI",
                tabApps = "Aplikasi",
                tabSettings = "Setelan",
                titleHome = "CoreFlow v6.1",
                titleControl = "Control Hub",
                titleLive = "Live Engine Feed",
                titleApps = "App DNA & OEM",
                titleSettings = "Setelan",
                subControl = "Kontrol Adaptif Berbasis Kernel",
                subLive = "Keputusan Terverifikasi Real-Time",
                subSettings = "Tema, Bahasa, Peta OEM & Tentang",
                statusStopped = "Berhenti",
                activeOverridesSuffix = "dikecualikan OEM",
                sectionProfile = "MESIN OTONOM REAL-TIME",
                sectionSparkline = "TREN SUHU & HEADROOM LIVE",
                sectionSession = "DAMPAK SESI",
                pauseCompare = "Jeda (Bandingkan A/B)",
                resumeEngine = "Lanjutkan CoreFlow",
                startEngine = "Jalankan CoreFlow Engine",
                bypassOn = "Bypass Aktif",
                bypassOff = "Bypass Mati",
                sectionCpu = "CPU & PENJADWAL EAS",
                sectionGpuThermal = "GPU, LAYAR & ZONA TERMAL",
                sectionPower = "DAYA & BYPASS CHARGING",
                sectionMemStorage = "MEMORI & PENYIMPANAN",
                sectionHudTiles = "GAME HUD & QUICK TILES",
                sectionAiBrain = "PREDIKTOR AI & KONTEKS LIVE",
                sectionEpisodes = "LOG KEPUTUSAN TERVERIFIKASI",
                emptyEpisodes = "Mesin sedang memantau secara tenang — belum ada mutasi baru yang diperlukan.",
                labelForecast10s = "Prediksi 10d",
                labelConfidence = "Akurasi AI",
                labelCpuLoad = "Beban CPU",
                labelTrend = "Tren Suhu",
                trendHeating = "Naik",
                trendCooling = "Turun",
                trendStable = "Stabil",
                statusAdaptive = "Adaptif",
                statusOemYield = "OEM Yield",
                statusFallback = "Fallback",
                searchAppsHint = "Cari nama aplikasi atau paket...",
                filterAll = "Semua",
                filterGames = "Game (Auto)",
                filterRules = "Stock OEM",
                installedAppsHeader = "APLIKASI TERPASANG",
                loadingApps = "Memuat daftar aplikasi & ikon...",
                noMatchingApps = "Tidak ada aplikasi yang cocok.",
                reloadApps = "Muat Ulang",
                effectiveGlobal = "Dikelola Otonom",
                effectiveOverride = "Stock OEM (Dikecualikan)",
                autoBypassHint = "Auto Bypass Charging aktif otomatis saat game 3D ini berjalan.",
                resetShort = "Otonom",
                sectionAppSettings = "SETELAN & GAYA HYPEROS",
                sectionProfileTransfer = "BACKUP & BAGIKAN KONFIGURASI",
                sectionCoexistence = "KOLABORASI OEM & DIAGNOSTIK",
                sectionRecovery = "PEMULIHAN CFJ4",
                sectionCreator = "PEMBUAT & PENGEMBANG",
                sectionSponsors = "DIDUKUNG OLEH · PARTNER AI & PLATFORM",
                sectionLicense = "LISENSI KOMUNITAS",
                exportProfileTitle = "Ekspor Konfigurasi & Pengecualian",
                exportProfileSummary = "Simpan konfigurasi otonom + daftar Stock OEM lengkap dengan identitas SoC",
                importProfileTitle = "Impor Konfigurasi CoreFlow",
                importProfileSummary = "Muat konfigurasi & verifikasi kecocokan chipset secara otomatis",
                pullToRefreshTexts = listOf(
                    "Tarik ke bawah untuk menyegarkan",
                    "Lepas untuk menyegarkan",
                    "Menyegarkan...",
                    "Berhasil disegarkan",
                ),
                sectionCopilot = "AUTONOMOUS CO-PILOT · NODE DINAMIS",
                copilotStandby = "Siaga · OEM",
                copilotActive = "Aktif",
                oemCooperatingTitle = "Bekerja Sama dengan OEM",
                oemCooperatingSummary = "CoreFlow secara permanen menyerahkan node berikut ke OEM (otomatis reset saat update ROM/kernel):",
                resetOemMap = "Reset Peta Kolaborasi OEM",
                floatingBarTitle = "Bilah bawah mengambang",
                floatingBarSummary = "Gunakan bilah navigasi kapsul mengambang gaya HyperOS",
                navBadgesTitle = "Lencana navigasi",
                navBadgesSummary = "Tampilkan jumlah aplikasi Stock OEM & node aktif pada bilah bawah",
                aboutTitle = "Tentang CoreFlow",
                aboutSummary = "Profil kreator, kode sumber GitHub, partner AI & lisensi",
                aboutViewGithub = "Lihat kode sumber di GitHub",
                aboutCreatorProfile = "Profil GitHub Mystivara Creator",
                sectionSelfAudit = "AUDIT KESEHATAN PERANGKAT",
                runSelfAudit = "Jalankan Audit 1-Ketuk",
                runningAudit = "Memeriksa kernel, UClamp & SELinux...",
                shareAudit = "Bagikan Ringkasan Audit",
                auditPassAll = "Semua Cek Sehat & Optimal",
                auditAttention = "Ada Catatan Perhatian",
                labelFramePacing = "Frame Pacing",
                labelAppDna = "App DNA",
                labelThermalEq = "Termal Dual",
                sectionDialogueConsole = "KONSOL DIALOG TELEMETRI CFAE",
                dialogueInputHint = "Tanya telemetri, App DNA, atau ketik Pause/Resume...",
                dialogueSend = "Kirim",
                dialogueQuickCardTitle = "Konsol Dialog CFAE Sovereign",
                dialogueQuickCardSummary = "Tanya alasan stutter, Kendali Termal EAS, audit konflik, atau jeda perbandingan OEM.",
                sectionUpdateCenter = "PUSAT PEMBARUAN CFAE",
                updateCheckNow = "Cek Pembaruan",
                updateChecking = "Memeriksa rilis GitHub...",
                updateUpToDate = "Modul & Aplikasi Mutakhir",
                updateAvailable = "Pembaruan Tersedia",
                updatePendingRelease = "Versi Baru di Main · Menunggu Rilis Publik",
                updateError = "Gagal Memeriksa Pembaruan",
                updateDownloadApk = "Unduh APK",
                updateDownloadModule = "Unduh Modul ZIP",
                updateOpenReleases = "Buka Halaman Rilis",
                updateModuleLabel = "Modul CFAE Engine",
                updateAppLabel = "Companion App",
                updateSkewWarning = "Versi Modul & Aplikasi berbeda — samakan keduanya agar protokol telemetri tetap sinkron.",
                updatePendingHint = "Versi baru sudah tercatat di update.json cabang main, tetapi GitHub Release publik belum diterbitkan. Cek halaman Releases atau Actions.",
                labelJankRisk = "Risiko Jank",
                labelWorkload = "Kelas Beban",
                runSessionAnalysis = "Analisis Sesi (AI ONNX)",
                sectionNeuralModel = "MODEL NEURAL DAEMON (CFW2 / ONNX)",
                importCfw2Title = "Impor Model CFW2 (.cfw2)",
                importCfw2Summary = "Pasang model mikro 1,7 KB hasil kalibrasi --diag (verifikasi CRC32 otomatis)",
                removeCfw2Title = "Kembalikan ke Model Bawaan",
                removeCfw2Summary = "Hapus equilibrium.cfw2 kustom dan gunakan Embedded RLS-MLP bawaan",
            )
        } else {
            AppStrings(
                isIndonesian = false,
                tabHome = "Home",
                tabControl = "Control",
                tabLive = "Live AI",
                tabApps = "Apps",
                tabSettings = "Settings",
                titleHome = "CoreFlow v6.1",
                titleControl = "Control Hub",
                titleLive = "Live Engine Feed",
                titleApps = "App DNA & OEM",
                titleSettings = "Settings",
                subControl = "Capability-Driven Kernel Domains",
                subLive = "Verified Real-Time Decisions",
                subSettings = "Theme, Language, OEM Map & About",
                statusStopped = "Stopped",
                activeOverridesSuffix = "OEM excluded",
                sectionProfile = "REAL-TIME AUTONOMOUS ENGINE",
                sectionSparkline = "LIVE TEMPERATURE & HEADROOM",
                sectionSession = "SESSION IMPACT",
                pauseCompare = "Pause (A/B Compare)",
                resumeEngine = "Resume CoreFlow",
                startEngine = "Start CoreFlow Engine",
                bypassOn = "Bypass Active",
                bypassOff = "Bypass Off",
                sectionCpu = "CPU & EAS SCHEDULER",
                sectionGpuThermal = "GPU, DISPLAY & THERMAL",
                sectionPower = "POWER & BYPASS CHARGING",
                sectionMemStorage = "MEMORY & STORAGE",
                sectionHudTiles = "GAME HUD & QUICK TILES",
                sectionAiBrain = "LIVE AI PREDICTOR & CONTEXT",
                sectionEpisodes = "VERIFIED DECISION EPISODES",
                emptyEpisodes = "Engine observing calmly — no new mutations needed yet.",
                labelForecast10s = "10s Forecast",
                labelConfidence = "AI Confidence",
                labelCpuLoad = "CPU Load",
                labelTrend = "Thermal Trend",
                trendHeating = "Rising",
                trendCooling = "Cooling",
                trendStable = "Stable",
                statusAdaptive = "Adaptive",
                statusOemYield = "OEM Yield",
                statusFallback = "Fallback",
                searchAppsHint = "Search app name or package...",
                filterAll = "All",
                filterGames = "Games (Auto)",
                filterRules = "Stock OEM",
                installedAppsHeader = "INSTALLED APPS",
                loadingApps = "Loading installed apps and icons...",
                noMatchingApps = "No matching apps found.",
                reloadApps = "Reload Apps",
                effectiveGlobal = "Autonomous Managed",
                effectiveOverride = "Stock OEM (Excluded)",
                autoBypassHint = "Auto Bypass Charging engages automatically when 3D gaming starts.",
                resetShort = "Autonomous",
                sectionAppSettings = "APP SETTINGS & HYPEROS STYLE",
                sectionProfileTransfer = "BACKUP & SHARE CONFIG",
                sectionCoexistence = "OEM COLLABORATION & DIAGNOSTICS",
                sectionRecovery = "CFJ4 RECOVERY",
                sectionCreator = "CREATOR & MAINTAINER",
                sectionSponsors = "POWERED BY · AI & PLATFORM SPONSORS",
                sectionLicense = "COMMUNITY LICENSE",
                exportProfileTitle = "Export Config & Exclusions",
                exportProfileSummary = "Save autonomous config + Stock OEM exclusions with SoC provenance",
                importProfileTitle = "Import CoreFlow Config",
                importProfileSummary = "Load config and automatically verify chipset provenance",
                pullToRefreshTexts = listOf(
                    "Pull down to refresh",
                    "Release to refresh",
                    "Refreshing...",
                    "Refreshed successfully",
                ),
                sectionCopilot = "AUTONOMOUS CO-PILOT · DYNAMIC KNOBS",
                copilotStandby = "Standby · OEM",
                copilotActive = "Active",
                oemCooperatingTitle = "Cooperating with OEM",
                oemCooperatingSummary = "CoreFlow cleanly yielded these nodes to OEM (auto-resets on ROM/kernel update):",
                resetOemMap = "Reset OEM Collaboration Map",
                floatingBarTitle = "Floating bottom bar",
                floatingBarSummary = "Use HyperOS floating pill navigation bar",
                navBadgesTitle = "Navigation badges",
                navBadgesSummary = "Show Stock OEM excluded apps & active co-pilot knob count on bottom bar",
                aboutTitle = "About CoreFlow",
                aboutSummary = "Creator profile, GitHub source, AI partners & license",
                aboutViewGithub = "View source code on GitHub",
                aboutCreatorProfile = "Mystivara Creator GitHub Profile",
                sectionSelfAudit = "DEVICE HEALTH SELF-AUDIT",
                runSelfAudit = "Run 1-Tap Health Audit",
                runningAudit = "Checking kernel, UClamp & SELinux...",
                shareAudit = "Share Audit Summary",
                auditPassAll = "All Health Checks Passed",
                auditAttention = "Attention Recommended",
                labelFramePacing = "Frame Pacing",
                labelAppDna = "App DNA",
                labelThermalEq = "Thermal Eq",
                sectionDialogueConsole = "CFAE TELEMETRY DIALOGUE CONSOLE",
                dialogueInputHint = "Ask live telemetry, App DNA, or type Pause/Resume...",
                dialogueSend = "Send",
                dialogueQuickCardTitle = "CFAE Sovereign Dialogue Console",
                dialogueQuickCardSummary = "Ask why frames dropped, inspect EAS Thermal Pacing & tamper forensics, or pause for OEM A/B.",
                sectionUpdateCenter = "CFAE UPDATE CENTER",
                updateCheckNow = "Check for Updates",
                updateChecking = "Checking GitHub releases...",
                updateUpToDate = "Module & App Up to Date",
                updateAvailable = "Update Available",
                updatePendingRelease = "Newer Version on Main · Pending Public Release",
                updateError = "Update Check Failed",
                updateDownloadApk = "Download APK",
                updateDownloadModule = "Download Module ZIP",
                updateOpenReleases = "Open Releases Page",
                updateModuleLabel = "CFAE Engine Module",
                updateAppLabel = "Companion App",
                updateSkewWarning = "Module and App versions differ — keep both aligned for full telemetry parity.",
                updatePendingHint = "A newer version is staged in main's update.json, but the public GitHub Release has not been published yet.",
                labelJankRisk = "Jank Risk",
                labelWorkload = "Workload",
                runSessionAnalysis = "AI Session Analysis (ONNX)",
                sectionNeuralModel = "DAEMON NEURAL MODEL (CFW2 / ONNX)",
                importCfw2Title = "Import CFW2 Model (.cfw2)",
                importCfw2Summary = "Install 1.7 KB micro-model calibrated from --diag (CRC32 verified)",
                removeCfw2Title = "Revert to Built-in Model",
                removeCfw2Summary = "Remove custom equilibrium.cfw2 and return to Embedded RLS-MLP",
            )
        }
    }
}
