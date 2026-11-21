package io.coreflow.ui

import android.content.Context
import android.content.Intent
import android.net.Uri
import androidx.activity.compose.BackHandler
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.IntrinsicSize
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.navigationBarsPadding
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.shadow
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.luminance
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import io.coreflow.BuildConfig
import io.coreflow.R
import top.yukonga.miuix.kmp.basic.Card
import top.yukonga.miuix.kmp.basic.CardDefaults
import top.yukonga.miuix.kmp.basic.Icon
import top.yukonga.miuix.kmp.basic.IconButton
import top.yukonga.miuix.kmp.basic.Scaffold
import top.yukonga.miuix.kmp.basic.SmallTitle
import top.yukonga.miuix.kmp.basic.SmallTopAppBar
import top.yukonga.miuix.kmp.basic.Text
import top.yukonga.miuix.kmp.icon.MiuixIcons
import top.yukonga.miuix.kmp.icon.extended.Back
import top.yukonga.miuix.kmp.icon.extended.Community
import top.yukonga.miuix.kmp.icon.extended.Link
import top.yukonga.miuix.kmp.preference.ArrowPreference
import top.yukonga.miuix.kmp.theme.MiuixTheme
import top.yukonga.miuix.kmp.utils.PressFeedbackType
import top.yukonga.miuix.kmp.utils.overScrollVertical
import top.yukonga.miuix.kmp.utils.scrollEndHaptic

private data class AboutSponsorEntry(
    val name: String,
    val role: String,
    val badge: String,
    val accent: Color,
    val url: String,
)

private val ABOUT_SPONSORS = listOf(
    AboutSponsorEntry(
        name = "Claude",
        role = "Anthropic · Systems & Architecture",
        badge = "CL",
        accent = Color(0xFFD97757),
        url = "https://claude.ai",
    ),
    AboutSponsorEntry(
        name = "ChatGPT",
        role = "OpenAI · Prototyping & Ideation",
        badge = "AI",
        accent = Color(0xFF10A37F),
        url = "https://chatgpt.com",
    ),
    AboutSponsorEntry(
        name = "Gemini",
        role = "Google DeepMind · Engine & Miuix UI",
        badge = "GM",
        accent = Color(0xFF4285F4),
        url = "https://gemini.google.com",
    ),
    AboutSponsorEntry(
        name = "GitHub",
        role = "Actions CI/CD & Hosting",
        badge = "GH",
        accent = Color(0xFF8B5CF6),
        url = "https://github.com/mystivara-creator/CoreFlow-Engine",
    ),
)

@Composable
fun AboutScreen(
    strings: AppStrings,
    daemonVersion: String?,
    onBack: () -> Unit,
) {
    BackHandler(onBack = onBack)

    val context = LocalContext.current
    val dark = MiuixTheme.colorScheme.surface.luminance() < 0.5f
    val primary = MiuixTheme.colorScheme.primary

    val backdropBrush = if (dark) {
        Brush.verticalGradient(
            colors = listOf(
                Color(0xFF1C0C14),
                Color(0xFF0E1324),
                Color(0xFF07080D),
            ),
        )
    } else {
        Brush.verticalGradient(
            colors = listOf(
                primary.copy(alpha = 0.16f),
                MiuixTheme.colorScheme.surface,
                MiuixTheme.colorScheme.surface,
            ),
        )
    }

    val translucentCardColors = CardDefaults.defaultColors(
        color = if (dark) {
            Color(0xFF1A1D2C).copy(alpha = 0.72f)
        } else {
            MiuixTheme.colorScheme.surfaceContainer.copy(alpha = 0.88f)
        },
    )

    Box(
        modifier = Modifier
            .fillMaxSize()
            .background(backdropBrush),
    ) {
        Scaffold(
            containerColor = Color.Transparent,
            topBar = {
                SmallTopAppBar(
                    title = "",
                    color = Color.Transparent,
                    navigationIcon = {
                        IconButton(
                            onClick = onBack,
                            modifier = Modifier.padding(start = 8.dp),
                        ) {
                            Icon(
                                imageVector = MiuixIcons.Back,
                                contentDescription = "Back",
                                tint = MiuixTheme.colorScheme.onSurface,
                            )
                        }
                    },
                )
            },
        ) { innerPadding ->
            LazyColumn(
                modifier = Modifier
                    .fillMaxSize()
                    .overScrollVertical()
                    .scrollEndHaptic(),
                contentPadding = PaddingValues(
                    start = 16.dp,
                    end = 16.dp,
                    top = innerPadding.calculateTopPadding() + 18.dp,
                    bottom = innerPadding.calculateBottomPadding() + 32.dp,
                ),
                horizontalAlignment = Alignment.CenterHorizontally,
                verticalArrangement = Arrangement.spacedBy(12.dp),
            ) {
                // Hero Brand Identity (KernelSU / HyperOS 4 About Layout)
                item {
                    Column(
                        modifier = Modifier
                            .fillMaxWidth()
                            .padding(vertical = 12.dp),
                        horizontalAlignment = Alignment.CenterHorizontally,
                    ) {
                        Card(
                            modifier = Modifier
                                .size(96.dp)
                                .shadow(
                                    elevation = 16.dp,
                                    shape = RoundedCornerShape(24.dp),
                                    ambientColor = Color(0xFFEF4444).copy(alpha = 0.45f),
                                    spotColor = Color(0xFFEF4444).copy(alpha = 0.45f),
                                )
                                .border(
                                    width = 1.dp,
                                    color = Color.White.copy(alpha = if (dark) 0.14f else 0.35f),
                                    shape = RoundedCornerShape(24.dp),
                                ),
                            cornerRadius = 24.dp,
                            insideMargin = PaddingValues(0.dp),
                            pressFeedbackType = PressFeedbackType.Tilt,
                            onClick = {
                                openAboutUrl(context, "https://github.com/mystivara-creator/CoreFlow-Engine")
                            },
                        ) {
                            Image(
                                painter = painterResource(id = R.drawable.ic_coreflow_brand),
                                contentDescription = "CoreFlow Engine Icon",
                                contentScale = ContentScale.Crop,
                                modifier = Modifier
                                    .fillMaxSize()
                                    .clip(RoundedCornerShape(24.dp)),
                            )
                        }

                        Spacer(Modifier.height(18.dp))

                        Text(
                            text = "CoreFlow Engine",
                            fontSize = 28.sp,
                            fontWeight = FontWeight.Bold,
                            color = if (dark) Color(0xFFF4F6FB) else MiuixTheme.colorScheme.onSurface,
                            textAlign = TextAlign.Center,
                        )

                        Spacer(Modifier.height(4.dp))

                        val ver = daemonVersion?.takeIf { it.isNotBlank() } ?: BuildConfig.VERSION_NAME
                        Text(
                            text = "v$ver · Autonomous Co-Pilot",
                            style = MiuixTheme.textStyles.body2,
                            color = MiuixTheme.colorScheme.onSurfaceVariantSummary,
                            textAlign = TextAlign.Center,
                        )
                    }
                }

                // Creator & GitHub Card (HyperOS Translucent Action Card)
                item {
                    Card(
                        modifier = Modifier.fillMaxWidth(),
                        colors = translucentCardColors,
                        insideMargin = PaddingValues(0.dp),
                    ) {
                        Row(
                            modifier = Modifier
                                .fillMaxWidth()
                                .padding(horizontal = 16.dp, vertical = 14.dp),
                            verticalAlignment = Alignment.CenterVertically,
                            horizontalArrangement = Arrangement.spacedBy(12.dp),
                        ) {
                            Box(
                                modifier = Modifier
                                    .size(44.dp)
                                    .clip(CircleShape)
                                    .background(primary.copy(alpha = 0.18f)),
                                contentAlignment = Alignment.Center,
                            ) {
                                Text(
                                    text = "MC",
                                    style = MiuixTheme.textStyles.headline2,
                                    fontWeight = FontWeight.Bold,
                                    color = primary,
                                )
                            }
                            Column(modifier = Modifier.weight(1f)) {
                                Text(
                                    text = "Mystivara Creator",
                                    style = MiuixTheme.textStyles.headline2,
                                    fontWeight = FontWeight.Bold,
                                )
                                Spacer(Modifier.height(2.dp))
                                Text(
                                    text = "@mystivara-creator · Creator & Maintainer",
                                    style = MiuixTheme.textStyles.footnote1,
                                    color = MiuixTheme.colorScheme.onSurfaceVariantSummary,
                                )
                            }
                        }

                        ArrowPreference(
                            title = strings.aboutViewGithub,
                            summary = "github.com/mystivara-creator/CoreFlow-Engine",
                            startAction = {
                                Icon(
                                    imageVector = MiuixIcons.Link,
                                    contentDescription = null,
                                    tint = MiuixTheme.colorScheme.onSurfaceVariantSummary,
                                    modifier = Modifier.padding(end = 12.dp).size(22.dp),
                                )
                            },
                            onClick = {
                                openAboutUrl(context, "https://github.com/mystivara-creator/CoreFlow-Engine")
                            },
                        )

                        ArrowPreference(
                            title = strings.aboutCreatorProfile,
                            summary = "github.com/mystivara-creator",
                            startAction = {
                                Icon(
                                    imageVector = MiuixIcons.Community,
                                    contentDescription = null,
                                    tint = MiuixTheme.colorScheme.onSurfaceVariantSummary,
                                    modifier = Modifier.padding(end = 12.dp).size(22.dp),
                                )
                            },
                            onClick = {
                                openAboutUrl(context, "https://github.com/mystivara-creator")
                            },
                        )
                    }
                }

                // AI Partners & Platform Sponsors (2x2 Tilt Cards)
                item {
                    Box(modifier = Modifier.fillMaxWidth()) {
                        SmallTitle(text = strings.sectionSponsors)
                    }
                }

                item {
                    Column(
                        modifier = Modifier.fillMaxWidth(),
                        verticalArrangement = Arrangement.spacedBy(8.dp),
                    ) {
                        for (pair in ABOUT_SPONSORS.chunked(2)) {
                            Row(
                                modifier = Modifier
                                    .fillMaxWidth()
                                    .height(IntrinsicSize.Min),
                                horizontalArrangement = Arrangement.spacedBy(8.dp),
                            ) {
                                for (sponsor in pair) {
                                    Card(
                                        modifier = Modifier
                                            .weight(1f)
                                            .fillMaxHeight(),
                                        colors = translucentCardColors,
                                        insideMargin = PaddingValues(14.dp),
                                        pressFeedbackType = PressFeedbackType.Tilt,
                                        onClick = { openAboutUrl(context, sponsor.url) },
                                    ) {
                                        Row(
                                            verticalAlignment = Alignment.CenterVertically,
                                            horizontalArrangement = Arrangement.spacedBy(10.dp),
                                        ) {
                                            Box(
                                                modifier = Modifier
                                                    .size(32.dp)
                                                    .clip(RoundedCornerShape(10.dp))
                                                    .background(sponsor.accent.copy(alpha = 0.18f)),
                                                contentAlignment = Alignment.Center,
                                            ) {
                                                Text(
                                                    text = sponsor.badge,
                                                    fontSize = 11.sp,
                                                    fontWeight = FontWeight.Bold,
                                                    color = sponsor.accent,
                                                )
                                            }
                                            Text(
                                                text = sponsor.name,
                                                style = MiuixTheme.textStyles.headline2,
                                                fontWeight = FontWeight.SemiBold,
                                                maxLines = 1,
                                                overflow = TextOverflow.Ellipsis,
                                            )
                                        }
                                        Spacer(Modifier.height(6.dp))
                                        Text(
                                            text = sponsor.role,
                                            style = MiuixTheme.textStyles.footnote1,
                                            color = MiuixTheme.colorScheme.onSurfaceVariantSummary,
                                            maxLines = 2,
                                            overflow = TextOverflow.Ellipsis,
                                        )
                                    }
                                }
                            }
                        }
                    }
                }

                // License Statement
                item {
                    Box(modifier = Modifier.fillMaxWidth()) {
                        SmallTitle(text = strings.sectionLicense)
                    }
                }

                item {
                    Card(
                        modifier = Modifier
                            .fillMaxWidth()
                            .navigationBarsPadding(),
                        colors = translucentCardColors,
                        insideMargin = PaddingValues(16.dp),
                        pressFeedbackType = PressFeedbackType.Tilt,
                        onClick = {
                            openAboutUrl(context, "https://github.com/mystivara-creator/CoreFlow-Engine")
                        },
                    ) {
                        Text(
                            text = "CoreFlow Companion v${BuildConfig.VERSION_NAME}",
                            style = MiuixTheme.textStyles.headline2,
                            fontWeight = FontWeight.SemiBold,
                        )
                        Spacer(Modifier.height(4.dp))
                        Text(
                            text = "CoreFlow Community Source-Available License — Free for personal use, learning, and security auditing. Authored by Mystivara Creator with transparent AI collaboration (Claude, ChatGPT, Gemini). Please do not rebrand, resell, or redistribute as your own work without permission.",
                            style = MiuixTheme.textStyles.footnote1,
                            color = MiuixTheme.colorScheme.onSurfaceVariantSummary,
                        )
                    }
                }
            }
        }
    }
}

private fun openAboutUrl(context: Context, url: String) {
    runCatching {
        val intent = Intent(Intent.ACTION_VIEW, Uri.parse(url))
            .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
        context.startActivity(intent)
    }
}
