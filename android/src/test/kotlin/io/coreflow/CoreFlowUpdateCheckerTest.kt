package io.coreflow

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

class CoreFlowUpdateCheckerTest {

    private val sampleInstalledV60R = CoreFlowUpdateChecker.InstalledSnapshot(
        appVersion = "6.0.0-R",
        appVersionCode = 63085, // CI run_number offset
        moduleVersion = "6.0.0-R",
        moduleVersionCode = 63000,
        moduleInstalled = true,
    )

    @Test
    fun trustedUrlAllowlistBlocksUntrustedHostsAndHttp() {
        assertTrue(
            CoreFlowUpdateChecker.isTrustedUrl(
                "https://github.com/mystivara-creator/CoreFlow-Engine/releases/download/v6.0.0-R/CFAE-System-Framework-v6.0.0-R-arm64.zip",
            ),
        )
        assertTrue(
            CoreFlowUpdateChecker.isTrustedUrl(
                "https://raw.githubusercontent.com/mystivara-creator/CoreFlow-Engine/main/module/update.json",
            ),
        )
        assertFalse(CoreFlowUpdateChecker.isTrustedUrl("http://github.com/mystivara-creator/CoreFlow-Engine/releases"))
        assertFalse(CoreFlowUpdateChecker.isTrustedUrl("https://evil.example.com/CoreFlow-Companion.apk"))
        assertFalse(CoreFlowUpdateChecker.isTrustedUrl("https://github.com/attacker/CoreFlow-Engine/releases"))
        assertFalse(CoreFlowUpdateChecker.isTrustedUrl("javascript:alert(1)"))
    }

    @Test
    fun sovereignRebuildEpochComparesHigherThanLegacyV620() {
        // Legacy v6.2.0 user (62000) MUST see v6.0.0-R (63000) as an update
        assertTrue(
            CoreFlowUpdateChecker.isRemoteNewer(
                remoteVersion = "v6.0.0-R",
                remoteCode = 63000,
                localVersion = "6.2.0",
                localCode = 62000,
                isCompanionApp = false,
            ),
        )
        // Current v6.0.0-R user (63000/63085) MUST NOT be downgraded to older published v6.2.0 (62000)
        assertFalse(
            CoreFlowUpdateChecker.isRemoteNewer(
                remoteVersion = "v6.2.0",
                remoteCode = 0,
                localVersion = "6.0.0-R",
                localCode = 63000,
                isCompanionApp = false,
            ),
        )
        assertFalse(
            CoreFlowUpdateChecker.isRemoteNewer(
                remoteVersion = "v6.0.0-R",
                remoteCode = 63000,
                localVersion = "6.0.0-R-debug",
                localCode = 63085,
                isCompanionApp = true,
            ),
        )
        // Future v6.0.1-R (63100) or v6.4.0 (64000) MUST trigger update even with CI run_number 63085
        assertTrue(
            CoreFlowUpdateChecker.isRemoteNewer(
                remoteVersion = "v6.0.1-R",
                remoteCode = 63100,
                localVersion = "6.0.0-R",
                localCode = 63085,
                isCompanionApp = true,
            ),
        )
        assertTrue(
            CoreFlowUpdateChecker.isRemoteNewer(
                remoteVersion = "v6.4.0",
                remoteCode = 64000,
                localVersion = "6.0.0-R",
                localCode = 63085,
                isCompanionApp = true,
            ),
        )
    }

    @Test
    fun parsesGitHubReleaseAssetsAndReportsUpdateAvailable() {
        val releaseJson = """
            {
              "tag_name": "v6.4.0",
              "html_url": "https://github.com/mystivara-creator/CoreFlow-Engine/releases/tag/v6.4.0",
              "published_at": "2026-12-01T10:00:00Z",
              "body": "## Highlights\n- Faster UClamp convergence\n- Smarter thermal equilibrium",
              "assets": [
                {
                  "name": "CFAE-System-Framework-v6.4.0-arm64.zip",
                  "size": 712345,
                  "browser_download_url": "https://github.com/mystivara-creator/CoreFlow-Engine/releases/download/v6.4.0/CFAE-System-Framework-v6.4.0-arm64.zip"
                },
                {
                  "name": "CoreFlow-Companion-v6.4.0-release.apk",
                  "size": 8456789,
                  "browser_download_url": "https://github.com/mystivara-creator/CoreFlow-Engine/releases/download/v6.4.0/CoreFlow-Companion-v6.4.0-release.apk"
                }
              ]
            }
        """.trimIndent()

        val published = CoreFlowUpdateChecker.parsePublishedReleaseJson(releaseJson)
        assertNotNull(published)
        val report = CoreFlowUpdateChecker.evaluateUpdateReport(
            published = published,
            manifest = null,
            installed = sampleInstalledV60R,
            nowMs = 1795400000000L,
        )
        assertEquals(CoreFlowUpdateChecker.UpdateState.UPDATE_AVAILABLE, report.state)
        assertTrue(report.moduleUpdateAvailable)
        assertTrue(report.appUpdateAvailable)
        assertFalse(report.pendingPublicRelease)
        assertEquals("6.4.0", report.latestVersion)
        assertNotNull(report.zipDownloadUrl)
        assertNotNull(report.apkDownloadUrl)
        assertEquals(712345L, report.zipSizeBytes)
        assertEquals(8456789L, report.apkSizeBytes)
    }

    @Test
    fun detectsPendingPublicReleaseWhenMainUpdateJsonAheadOfGitHubRelease() {
        val oldReleaseJson = """
            {
              "tag_name": "v6.2.0",
              "html_url": "https://github.com/mystivara-creator/CoreFlow-Engine/releases/tag/v6.2.0",
              "body": "Legacy release",
              "assets": []
            }
        """.trimIndent()
        val newerManifestJson = """
            {
              "version": "v6.4.0",
              "versionCode": 64000,
              "zipUrl": "https://github.com/mystivara-creator/CoreFlow-Engine/releases/download/v6.4.0/CFAE-System-Framework-v6.4.0-arm64.zip",
              "changelog": "https://raw.githubusercontent.com/mystivara-creator/CoreFlow-Engine/v6.4.0/CHANGELOG.md",
              "companion": {
                "version": "6.4.0",
                "versionCode": 64000,
                "apkUrl": "https://github.com/mystivara-creator/CoreFlow-Engine/releases/download/v6.4.0/CoreFlow-Companion-v6.4.0-release.apk",
                "minModuleVersionCode": 64000
              },
              "releasePage": "https://github.com/mystivara-creator/CoreFlow-Engine/releases/latest"
            }
        """.trimIndent()

        val report = CoreFlowUpdateChecker.evaluateUpdateReport(
            published = CoreFlowUpdateChecker.parsePublishedReleaseJson(oldReleaseJson),
            manifest = CoreFlowUpdateChecker.parseUpdateManifestJson(newerManifestJson),
            installed = sampleInstalledV60R,
            nowMs = 1795400000000L,
        )
        assertEquals(CoreFlowUpdateChecker.UpdateState.PENDING_PUBLIC_RELEASE, report.state)
        assertTrue(report.pendingPublicRelease)
        assertTrue(report.moduleUpdateAvailable)
        assertTrue(report.appUpdateAvailable)
        // Must NOT hand out direct download links that would 404 before Release publication
        assertNull(report.zipDownloadUrl)
        assertNull(report.apkDownloadUrl)
    }

    @Test
    fun fallsBackToUpdateManifestWhenGitHubApiUnavailableAndDetectsVersionSkew() {
        val manifestJson = """
            {
              "version": "v6.0.0-R",
              "versionCode": 63000,
              "zipUrl": "https://github.com/mystivara-creator/CoreFlow-Engine/releases/download/v6.0.0-R/CFAE-System-Framework-v6.0.0-R-arm64.zip",
              "companion": {
                "version": "6.0.0-R",
                "versionCode": 63000,
                "apkUrl": "https://github.com/mystivara-creator/CoreFlow-Engine/releases/download/v6.0.0-R/CoreFlow-Companion-v6.0.0-R-release.apk"
              }
            }
        """.trimIndent()
        val skewedInstalled = CoreFlowUpdateChecker.InstalledSnapshot(
            appVersion = "6.0.0-R",
            appVersionCode = 63085,
            moduleVersion = "6.2.0",
            moduleVersionCode = 62000,
            moduleInstalled = true,
        )
        val report = CoreFlowUpdateChecker.evaluateUpdateReport(
            published = null,
            manifest = CoreFlowUpdateChecker.parseUpdateManifestJson(manifestJson),
            installed = skewedInstalled,
            nowMs = 1795400000000L,
        )
        assertEquals(CoreFlowUpdateChecker.UpdateState.UPDATE_AVAILABLE, report.state)
        assertEquals("update_json_fallback", report.source)
        assertTrue(report.versionSkewDetected)
        assertTrue(report.moduleUpdateAvailable)
        assertFalse(report.appUpdateAvailable)
    }
}
