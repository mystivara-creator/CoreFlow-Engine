package io.coreflow

import android.content.Context
import top.yukonga.miuix.kmp.theme.ColorSchemeMode

object AppPreferences {
    private const val PREFS_NAME = "coreflow_companion_prefs"
    private const val KEY_THEME_MODE = "theme_mode_index"
    private const val KEY_POLL_INTERVAL_IDX = "poll_interval_index"

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
}
