package io.coreflow

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.graphics.Color
import io.coreflow.ui.CoreFlowApp
import top.yukonga.miuix.kmp.theme.MiuixTheme
import top.yukonga.miuix.kmp.theme.ThemeController

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            var themeModeIndex by remember {
                mutableIntStateOf(AppPreferences.getThemeModeIndex(this))
            }
            val selectedMode = AppPreferences.themeModes[themeModeIndex]
            val themeController = remember(selectedMode) {
                ThemeController(
                    colorSchemeMode = selectedMode,
                    keyColor = Color(0xFF3482FF),
                )
            }
            MiuixTheme(controller = themeController) {
                CoreFlowApp(
                    themeModeIndex = themeModeIndex,
                    onThemeModeChange = { newIdx ->
                        themeModeIndex = newIdx
                        AppPreferences.setThemeModeIndex(this, newIdx)
                    },
                )
            }
        }
    }
}
