package io.coreflow

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.runtime.remember
import androidx.compose.ui.graphics.Color
import io.coreflow.ui.CoreFlowApp
import top.yukonga.miuix.kmp.theme.ColorSchemeMode
import top.yukonga.miuix.kmp.theme.MiuixTheme
import top.yukonga.miuix.kmp.theme.ThemeController

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            val themeController = remember {
                ThemeController(
                    colorSchemeMode = ColorSchemeMode.MonetSystem,
                    keyColor = Color(0xFF3482FF),
                )
            }
            MiuixTheme(controller = themeController) {
                CoreFlowApp()
            }
        }
    }
}
