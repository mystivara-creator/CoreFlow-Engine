package io.coreflow

import android.content.Context
import top.yukonga.miuix.kmp.theme.ColorSchemeMode

object AppPreferences {
    private const val PREFS_NAME = "coreflow_companion_prefs"
    private const val KEY_THEME_MODE = "theme_mode_index"
    private const val KEY_POLL_INTERVAL_IDX = "poll_interval_index"
    private const val KEY_LANGUAGE_IDX = "language_index"

    val themeModeLabels: List<String> = listOf(
        "Monet System",
        "System",
        "Light",
        "Dark",
        "Monet Light",
        "Monet Dark",
    )

    val themeModes: List<ColorSchemeMode> = listOf(
        ColorSchemeMode.MonetSystem,
        ColorSchemeMode.System,
        ColorSchemeMode.Light,
        ColorSchemeMode.Dark,
        ColorSchemeMode.MonetLight,
        ColorSchemeMode.MonetDark,
    )

    val pollIntervalLabels: List<String> = listOf(
        "Fast (1.5s)",
        "Balanced (2.5s)",
        "Battery Saver (5.0s)",
    )

    val pollIntervalValuesMs: List<Long> = listOf(1500L, 2500L, 5000L)

    val languageLabels: List<String> = listOf(
        "System Default",
        "English",
        "Bahasa Indonesia",
    )

    fun getThemeModeIndex(context: Context): Int =
        context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
            .getInt(KEY_THEME_MODE, 0)
            .coerceIn(0, themeModes.lastIndex)

    fun setThemeModeIndex(context: Context, index: Int) {
        context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
            .edit()
            .putInt(KEY_THEME_MODE, index.coerceIn(0, themeModes.lastIndex))
            .apply()
    }

    fun getPollIntervalIndex(context: Context): Int =
        context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
            .getInt(KEY_POLL_INTERVAL_IDX, 1)
            .coerceIn(0, pollIntervalValuesMs.lastIndex)

    fun setPollIntervalIndex(context: Context, index: Int) {
        context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
            .edit()
            .putInt(KEY_POLL_INTERVAL_IDX, index.coerceIn(0, pollIntervalValuesMs.lastIndex))
            .apply()
    }

    private const val KEY_PRE_POWERSAVE_PROFILE = "pre_powersave_profile"
    private const val KEY_FLOATING_NAV_BAR = "floating_nav_bar"
    private const val KEY_NAV_BADGES = "nav_badges_enabled"

    fun getLanguageIndex(context: Context): Int =
        context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
            .getInt(KEY_LANGUAGE_IDX, 0)
            .coerceIn(0, languageLabels.lastIndex)

    fun setLanguageIndex(context: Context, index: Int) {
        context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
            .edit()
            .putInt(KEY_LANGUAGE_IDX, index.coerceIn(0, languageLabels.lastIndex))
            .apply()
    }

    fun getFloatingNavBar(context: Context): Boolean =
        context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
            .getBoolean(KEY_FLOATING_NAV_BAR, true)

    fun setFloatingNavBar(context: Context, enabled: Boolean) {
        context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
            .edit()
            .putBoolean(KEY_FLOATING_NAV_BAR, enabled)
            .apply()
    }

    fun getNavBadgesEnabled(context: Context): Boolean =
        context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
            .getBoolean(KEY_NAV_BADGES, true)

    fun setNavBadgesEnabled(context: Context, enabled: Boolean) {
        context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
            .edit()
            .putBoolean(KEY_NAV_BADGES, enabled)
            .apply()
    }

    fun getPrePowerSaveProfile(context: Context): String? =
        context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
            .getString(KEY_PRE_POWERSAVE_PROFILE, null)

    fun setPrePowerSaveProfile(context: Context, configValue: String?) {
        context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
            .edit()
            .apply {
                if (configValue.isNullOrBlank()) remove(KEY_PRE_POWERSAVE_PROFILE)
                else putString(KEY_PRE_POWERSAVE_PROFILE, configValue)
            }
            .apply()
    }
}
