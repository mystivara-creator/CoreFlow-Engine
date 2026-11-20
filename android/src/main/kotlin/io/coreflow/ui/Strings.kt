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
) {
    fun humanizeMode(mode: String): String = when (mode.uppercase(Locale.US)) {
        "LAUNCH_BURST" -> if (isIndonesian) "Lonjakan Buka" else "Launch Burst"
        "INTERACTIVE" -> if (isIndonesian) "Interaktif" else "Interactive"
        "GAMING" -> "Gaming"
        "EFFICIENCY" -> if (isIndonesian) "Efisiensi" else "Efficiency"
        "SCREEN_OFF" -> if (isIndonesian) "Layar Mati" else "Screen Off"
        "EXCLUDED" -> if (isIndonesian) "OEM Standar" else "OEM Stock"
        "THERMAL_GUARD" -> if (isIndonesian) "Jaga Suhu" else "Thermal Guard"
        "THERMAL_EMERGENCY" -> if (isIndonesian) "Darurat Suhu" else "Thermal Cutoff"
        else -> mode.lowercase(Locale.US).replaceFirstChar { it.uppercaseChar() }
    }

    fun humanizeOutcome(outcome: String): String = when (outcome) {
        "oem_yield" -> "OEM Yield"
        "rolled_back" -> if (isIndonesian) "Dipulihkan" else "Rolled Back"
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
        velCpm >= 0.6f -> String.format(Locale.US, "%s +%.1f°/m", trendHeating, velCpm)
        velCpm <= -0.6f -> String.format(Locale.US, "%s %.1f°/m", trendCooling, velCpm)
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
                titleHome = "CoreFlow v6",
                titleControl = "Control Hub",
                titleLive = "Live Engine Feed",
                titleApps = "Aturan Aplikasi",
                titleSettings = "Setelan",
                subControl = "Kontrol Adaptif Berbasis Kernel",
                subLive = "Keputusan Terverifikasi Real-Time",
                subSettings = "Tema, Bahasa, Profil & Kredit",
                statusStopped = "Berhenti",
                activeOverridesSuffix = "aturan aktif",
                sectionProfile = "PROFIL OTONOM",
                sectionSparkline = "TREN SUHU & HEADROOM LIVE",
                sectionSession = "DAMPAK SESI",
                pauseCompare = "Jeda (Bandingkan A/B)",
                resumeEngine = "Lanjutkan CoreFlow",
                startEngine = "Jalankan CoreFlow Engine",
                bypassOn = "Bypass Aktif",
                bypassOff = "Bypass Mati",
                sectionCpu = "CPU & PENJADWAL EAS",
                sectionGpuThermal = "GPU & ZONA TERMAL",
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
                filterGames = "Game",
                filterRules = "Diatur",
                installedAppsHeader = "APLIKASI TERPASANG",
                loadingApps = "Memuat daftar aplikasi & ikon...",
                noMatchingApps = "Tidak ada aplikasi yang cocok.",
                reloadApps = "Muat Ulang",
                effectiveGlobal = "Ikut Profil Global",
                effectiveOverride = "Override Aktif",
                autoBypassHint = "Auto Bypass Charging aktif otomatis saat game ini dibuka.",
                resetShort = "Reset",
                sectionAppSettings = "SETELAN APLIKASI",
                sectionProfileTransfer = "BACKUP & BAGIKAN PROFIL",
                sectionCoexistence = "KOEKSISTENSI OEM & DIAGNOSTIK",
                sectionRecovery = "PEMULIHAN CFJ4",
                sectionCreator = "PEMBUAT & PENGEMBANG",
                sectionSponsors = "DIDUKUNG OLEH · PARTNER AI & PLATFORM",
                sectionLicense = "LISENSI KOMUNITAS",
                exportProfileTitle = "Ekspor Profil & Aturan Aplikasi",
                exportProfileSummary = "Simpan coreflow-profile-v6.json lengkap dengan identitas SoC",
                importProfileTitle = "Impor Profil CoreFlow",
                importProfileSummary = "Muat preset & verifikasi kecocokan chipset secara otomatis",
            )
        } else {
            AppStrings(
                isIndonesian = false,
                tabHome = "Home",
                tabControl = "Control",
                tabLive = "Live AI",
                tabApps = "Apps",
                tabSettings = "Settings",
                titleHome = "CoreFlow v6",
                titleControl = "Control Hub",
                titleLive = "Live Engine Feed",
                titleApps = "Per-App Rules",
                titleSettings = "Settings",
                subControl = "Capability-Driven Kernel Domains",
                subLive = "Verified Real-Time Decisions",
                subSettings = "Theme, Language, Profiles & Credits",
                statusStopped = "Stopped",
                activeOverridesSuffix = "active rules",
                sectionProfile = "AUTONOMOUS PROFILE",
                sectionSparkline = "LIVE TEMPERATURE & HEADROOM",
                sectionSession = "SESSION IMPACT",
                pauseCompare = "Pause (A/B Compare)",
                resumeEngine = "Resume CoreFlow",
                startEngine = "Start CoreFlow Engine",
                bypassOn = "Bypass Active",
                bypassOff = "Bypass Off",
                sectionCpu = "CPU & EAS SCHEDULER",
                sectionGpuThermal = "GPU & THERMAL ZONES",
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
                filterGames = "Games",
                filterRules = "Rules",
                installedAppsHeader = "INSTALLED APPS",
                loadingApps = "Loading installed apps and icons...",
                noMatchingApps = "No matching apps found.",
                reloadApps = "Reload Apps",
                effectiveGlobal = "Using Global",
                effectiveOverride = "Override Active",
                autoBypassHint = "Auto Bypass Charging engages automatically when launched.",
                resetShort = "Reset",
                sectionAppSettings = "APP SETTINGS",
                sectionProfileTransfer = "BACKUP & SHARE PROFILES",
                sectionCoexistence = "OEM COEXISTENCE & DIAGNOSTICS",
                sectionRecovery = "CFJ4 RECOVERY",
                sectionCreator = "CREATOR & MAINTAINER",
                sectionSponsors = "POWERED BY · AI & PLATFORM SPONSORS",
                sectionLicense = "COMMUNITY LICENSE",
                exportProfileTitle = "Export Profile & App Rules",
                exportProfileSummary = "Save coreflow-profile-v6.json with SoC provenance metadata",
                importProfileTitle = "Import CoreFlow Profile",
                importProfileSummary = "Load preset and automatically verify chipset provenance",
            )
        }
    }
}
