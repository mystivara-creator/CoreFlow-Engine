package io.coreflow

import android.content.Intent
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.graphics.Color
import io.coreflow.ui.CoreFlowApp
import top.yukonga.miuix.kmp.theme.MiuixTheme
import top.yukonga.miuix.kmp.theme.ThemeController

class MainActivity : ComponentActivity() {
    private val deepLinkTab = mutableIntStateOf(-1)
    private val deepLinkPrompt = mutableStateOf<String?>(null)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        consumeIntent(intent)
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
                    pendingTab = deepLinkTab.intValue.takeIf { it >= 0 },
                    pendingDialoguePrompt = deepLinkPrompt.value,
                    onConsumeDeepLink = {
                        deepLinkTab.intValue = -1
                        deepLinkPrompt.value = null
                    },
                )
            }
        }
    }

    override fun onNewIntent(intent: Intent) {
        super.onNewIntent(intent)
        setIntent(intent)
        consumeIntent(intent)
    }

    private fun consumeIntent(intent: Intent?) {
        if (intent == null) return
        val tab = intent.getIntExtra(EXTRA_OPEN_TAB, -1)
        if (tab in 0..4) {
            deepLinkTab.intValue = tab
        }
        val prompt = intent.getStringExtra(EXTRA_FORENSIC_PROMPT)?.trim()
        if (!prompt.isNullOrEmpty()) {
            deepLinkPrompt.value = prompt
        }
    }

    companion object {
        const val EXTRA_OPEN_TAB = "io.coreflow.EXTRA_OPEN_TAB"
        const val EXTRA_FORENSIC_PROMPT = "io.coreflow.EXTRA_FORENSIC_PROMPT"
    }
}
