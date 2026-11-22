package io.coreflow

import android.content.Context
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import java.io.ByteArrayOutputStream
import java.net.HttpURLConnection
import java.net.URL

/**
 * Dual-Source Sovereign Update Checker for CFAE System Framework (Module + Companion APK).
 *
 * Strategy:
 * 1. Primary: GitHub Releases API (`releases/latest`) using ETag (`If-None-Match`) so 304 checks
 *    consume zero GitHub API rate limit and verify that public downloadable assets actually exist.
 * 2. Secondary / Fallback: `raw.githubusercontent.com/.../main/module/update.json` (zero API rate
 *    limit) which also detects when `main` has a newer version whose public GitHub Release has not
 *    been published yet ("Pending Public Release" — never offers a broken 404 download link).
 * 3. Strict HTTPS host allowlist: only official `mystivara-creator/CoreFlow-Engine` GitHub URLs
 *    are accepted; untrusted URLs are discarded fail-closed.
 */
object CoreFlowUpdateChecker {

    const val RELEASES_API_URL =
        "https://api.github.com/repos/mystivara-creator/CoreFlow-Engine/releases/latest"
    const val UPDATE_MANIFEST_URL =
        "https://raw.githubusercontent.com/mystivara-creator/CoreFlow-Engine/main/module/update.json"
    const val DEFAULT_RELEASE_PAGE =
        "https://github.com/mystivara-creator/CoreFlow-Engine/releases"

    const val CACHE_TTL_MS: Long = 12L * 60L * 60L * 1000L // 12 hours
    private const val MAX_BODY_BYTES = 256 * 1024

    enum class UpdateState {
        UP_TO_DATE,
        UPDATE_AVAILABLE,
        PENDING_PUBLIC_RELEASE,
        ERROR,
    }

    data class InstalledSnapshot(
        val appVersion: String,
        val appVersionCode: Int,
        val moduleVersion: String,
        val moduleVersionCode: Int,
        val moduleInstalled: Boolean,
    )

    data class PublishedReleaseInfo(
        val tagName: String,
        val releasePageUrl: String,
        val zipDownloadUrl: String?,
        val zipSizeBytes: Long,
        val apkDownloadUrl: String?,
        val apkSizeBytes: Long,
        val changelogSummary: String,
        val publishedAt: String,
    )

    data class UpdateManifestInfo(
        val moduleVersion: String,
        val moduleVersionCode: Int,
        val zipUrl: String?,
        val changelogUrl: String?,
        val companionVersion: String,
        val companionVersionCode: Int,
        val apkUrl: String?,
        val minModuleVersionCode: Int,
        val releasePageUrl: String?,
    )

    data class UpdateCheckReport(
        val state: UpdateState,
        val source: String,
        val installedAppVersion: String,
        val installedAppCode: Int,
        val installedModuleVersion: String,
        val installedModuleCode: Int,
        val moduleInstalled: Boolean,
        val latestVersion: String,
        val latestModuleCode: Int,
        val latestAppVersion: String,
        val latestAppCode: Int,
        val moduleUpdateAvailable: Boolean,
        val appUpdateAvailable: Boolean,
        val pendingPublicRelease: Boolean,
        val versionSkewDetected: Boolean,
        val zipDownloadUrl: String?,
        val zipSizeBytes: Long,
        val apkDownloadUrl: String?,
        val apkSizeBytes: Long,
        val releasePageUrl: String,
        val changelogSummary: String,
        val checkedAtMs: Long,
        val errorMessage: String? = null,
    )

    fun isTrustedUrl(raw: String?): Boolean {
        val url = raw?.trim() ?: return false
        if (url.length > 512 || !url.startsWith("https://")) return false
        if (url.any { it <= ' ' || it == '\u007F' }) return false
        return url.startsWith("https://github.com/mystivara-creator/CoreFlow-Engine/") ||
            url == "https://github.com/mystivara-creator/CoreFlow-Engine" ||
            url.startsWith("https://raw.githubusercontent.com/mystivara-creator/CoreFlow-Engine/") ||
            url.startsWith("https://api.github.com/repos/mystivara-creator/CoreFlow-Engine/")
    }

    fun normalizeVersion(raw: String): String {
        var cleaned = raw.trim()
        if (cleaned.startsWith("v", ignoreCase = true)) {
            cleaned = cleaned.substring(1)
        }
        if (cleaned.endsWith("-debug", ignoreCase = true)) {
            cleaned = cleaned.dropLast(6)
        }
        return cleaned.trim()
    }

    /**
     * Maps semantic versions (including the v6.0.0-R Sovereign Rebuild epoch at 63000)
     * onto a monotonic comparable integer so v6.0.0-R (63000) is strictly newer than v6.2.0 (62000)
     * while future v6.0.1-R (63100), v6.4.0 (64000), or v7.0.0 (70000) compare strictly higher.
     */
    fun deriveCanonicalVersionCode(rawVersion: String, explicitCode: Int = 0): Int {
        val norm = normalizeVersion(rawVersion)
        if (norm.isNotEmpty()) {
            val match = Regex("""^(\d+)\.(\d+)\.(\d+)(?:-([A-Za-z0-9.]+))?$""").find(norm)
            if (match != null) {
                val major = match.groupValues[1].toIntOrNull() ?: 0
                val minor = match.groupValues[2].toIntOrNull() ?: 0
                val patch = match.groupValues[3].toIntOrNull() ?: 0
                val suffix = match.groupValues[4]
                val isSovereignRebuild = major == 6 && minor == 0 && suffix.equals("R", ignoreCase = true)
                val semverCode = if (isSovereignRebuild) {
                    63000 + patch * 100
                } else {
                    major * 10000 + minor * 1000 + patch * 100
                }
                return if (explicitCode > 0 && explicitCode >= semverCode && explicitCode < semverCode + 100) {
                    explicitCode
                } else if (explicitCode > semverCode && !isSovereignRebuild) {
                    explicitCode
                } else {
                    semverCode
                }
            }
        }
        return explicitCode.coerceAtLeast(0)
    }

    fun isRemoteNewer(
        remoteVersion: String,
        remoteCode: Int,
        localVersion: String,
        localCode: Int,
        isCompanionApp: Boolean,
    ): Boolean {
        val normRemote = normalizeVersion(remoteVersion)
        val normLocal = normalizeVersion(localVersion)
        if (normRemote.isEmpty()) return false
        if (normLocal.isEmpty()) return true

        val remoteCanonical = deriveCanonicalVersionCode(normRemote, remoteCode)
        // Companion App CI builds add `ciBuildNumber` (0..9999) to BuildConfig.VERSION_CODE,
        // so derive the app's canonical base from versionName when parseable.
        val localCanonical = if (isCompanionApp) {
            deriveCanonicalVersionCode(normLocal, 0).takeIf { it > 0 } ?: localCode
        } else {
            deriveCanonicalVersionCode(normLocal, localCode)
        }

        if (normRemote.equals(normLocal, ignoreCase = true)) {
            if (isCompanionApp) return false
            return remoteCode > 0 && localCode > 0 && remoteCode > localCode
        }
        return remoteCanonical > localCanonical
    }

    fun parsePublishedReleaseJson(rawJson: String): PublishedReleaseInfo? {
        val root = MiniJson.parseObject(rawJson) ?: return null
        val tagName = (root["tag_name"] as? String)?.trim().orEmpty()
        if (tagName.isEmpty()) return null

        val htmlUrl = (root["html_url"] as? String)?.takeIf { isTrustedUrl(it) } ?: DEFAULT_RELEASE_PAGE
        val body = (root["body"] as? String).orEmpty()
        val publishedAt = (root["published_at"] as? String).orEmpty()

        var zipUrl: String? = null
        var zipSize = 0L
        var apkUrl: String? = null
        var apkSize = 0L

        val assets = root["assets"] as? List<*> ?: emptyList<Any?>()
        for (item in assets) {
            val asset = item as? Map<*, *> ?: continue
            val name = (asset["name"] as? String)?.trim().orEmpty()
            val dlUrl = (asset["browser_download_url"] as? String)?.trim()
            val size = (asset["size"] as? Number)?.toLong() ?: 0L
            if (!isTrustedUrl(dlUrl)) continue

            val lower = name.lowercase()
            if (lower.endsWith(".zip") && (lower.contains("cfae") || lower.contains("coreflow"))) {
                if (zipUrl == null) {
                    zipUrl = dlUrl
                    zipSize = size
                }
            } else if (lower.endsWith(".apk") && !lower.contains("debug")) {
                if (apkUrl == null) {
                    apkUrl = dlUrl
                    apkSize = size
                }
            }
        }

        val summary = body.lineSequence()
            .map { it.trim() }
            .filter { it.isNotEmpty() && !it.startsWith("#") }
            .take(4)
            .joinToString("\n")
            .take(360)

        return PublishedReleaseInfo(
            tagName = tagName,
            releasePageUrl = htmlUrl,
            zipDownloadUrl = zipUrl,
            zipSizeBytes = zipSize,
            apkDownloadUrl = apkUrl,
            apkSizeBytes = apkSize,
            changelogSummary = summary,
            publishedAt = publishedAt,
        )
    }

    fun parseUpdateManifestJson(rawJson: String): UpdateManifestInfo? {
        val root = MiniJson.parseObject(rawJson) ?: return null
        val version = (root["version"] as? String)?.trim().orEmpty()
        val versionCode = (root["versionCode"] as? Number)?.toInt() ?: 0
        if (version.isEmpty() && versionCode <= 0) return null

        val zipUrl = (root["zipUrl"] as? String)?.takeIf { isTrustedUrl(it) }
        val changelogUrl = (root["changelog"] as? String)?.takeIf { isTrustedUrl(it) }
        val releasePage = (root["releasePage"] as? String)?.takeIf { isTrustedUrl(it) }

        val companion = root["companion"] as? Map<*, *>
        val compVersion = (companion?.get("version") as? String)?.trim()?.takeIf { it.isNotEmpty() }
            ?: normalizeVersion(version)
        val compCode = (companion?.get("versionCode") as? Number)?.toInt() ?: versionCode
        val apkUrl = (companion?.get("apkUrl") as? String)?.takeIf { isTrustedUrl(it) }
        val minModCode = (companion?.get("minModuleVersionCode") as? Number)?.toInt() ?: versionCode

        return UpdateManifestInfo(
            moduleVersion = version,
            moduleVersionCode = versionCode,
            zipUrl = zipUrl,
            changelogUrl = changelogUrl,
            companionVersion = compVersion,
            companionVersionCode = compCode,
            apkUrl = apkUrl,
            minModuleVersionCode = minModCode,
            releasePageUrl = releasePage,
        )
    }

    /**
     * Pure deterministic update evaluator combining GitHub Releases (`published`) and
     * `main/module/update.json` (`manifest`) against the locally installed App & Module.
     */
    fun evaluateUpdateReport(
        published: PublishedReleaseInfo?,
        manifest: UpdateManifestInfo?,
        installed: InstalledSnapshot,
        nowMs: Long,
        errorDetail: String? = null,
    ): UpdateCheckReport {
        val normInstalledApp = normalizeVersion(installed.appVersion)
        val normInstalledMod = normalizeVersion(installed.moduleVersion)
        val versionSkew = installed.moduleInstalled &&
            normInstalledApp.isNotEmpty() &&
            normInstalledMod.isNotEmpty() &&
            !normInstalledApp.equals(normInstalledMod, ignoreCase = true)

        if (published == null && manifest == null) {
            return UpdateCheckReport(
                state = UpdateState.ERROR,
                source = "none",
                installedAppVersion = normInstalledApp,
                installedAppCode = installed.appVersionCode,
                installedModuleVersion = normInstalledMod,
                installedModuleCode = installed.moduleVersionCode,
                moduleInstalled = installed.moduleInstalled,
                latestVersion = "",
                latestModuleCode = 0,
                latestAppVersion = "",
                latestAppCode = 0,
                moduleUpdateAvailable = false,
                appUpdateAvailable = false,
                pendingPublicRelease = false,
                versionSkewDetected = versionSkew,
                zipDownloadUrl = null,
                zipSizeBytes = 0L,
                apkDownloadUrl = null,
                apkSizeBytes = 0L,
                releasePageUrl = DEFAULT_RELEASE_PAGE,
                changelogSummary = "",
                checkedAtMs = nowMs,
                errorMessage = errorDetail ?: "Unable to reach GitHub update endpoints.",
            )
        }

        val pubTag = published?.tagName.orEmpty()
        val pubCode = if (pubTag.isNotEmpty()) deriveCanonicalVersionCode(pubTag, 0) else 0
        val manModVer = manifest?.moduleVersion.orEmpty()
        val manModCode = if (manifest != null) {
            deriveCanonicalVersionCode(manModVer, manifest.moduleVersionCode)
        } else {
            0
        }
        val manAppVer = manifest?.companionVersion.orEmpty()
        val manAppCode = if (manifest != null) {
            deriveCanonicalVersionCode(manAppVer, manifest.companionVersionCode)
        } else {
            0
        }

        // Detect when `main/module/update.json` has advanced beyond the latest published GitHub Release.
        val manifestAheadOfPublished = manifest != null &&
            published != null &&
            isRemoteNewer(manModVer, manModCode, pubTag, pubCode, isCompanionApp = false)

        if (manifestAheadOfPublished && manifest != null) {
            val modNewerThanInstalled = !installed.moduleInstalled ||
                isRemoteNewer(manModVer, manifest.moduleVersionCode, installed.moduleVersion, installed.moduleVersionCode, isCompanionApp = false)
            val appNewerThanInstalled = isRemoteNewer(
                manAppVer,
                manifest.companionVersionCode,
                installed.appVersion,
                installed.appVersionCode,
                isCompanionApp = true,
            )

            if (modNewerThanInstalled || appNewerThanInstalled) {
                return UpdateCheckReport(
                    state = UpdateState.PENDING_PUBLIC_RELEASE,
                    source = "dual_pending_release",
                    installedAppVersion = normInstalledApp,
                    installedAppCode = installed.appVersionCode,
                    installedModuleVersion = normInstalledMod,
                    installedModuleCode = installed.moduleVersionCode,
                    moduleInstalled = installed.moduleInstalled,
                    latestVersion = normalizeVersion(manModVer),
                    latestModuleCode = manifest.moduleVersionCode,
                    latestAppVersion = normalizeVersion(manAppVer),
                    latestAppCode = manifest.companionVersionCode,
                    moduleUpdateAvailable = modNewerThanInstalled,
                    appUpdateAvailable = appNewerThanInstalled,
                    pendingPublicRelease = true,
                    versionSkewDetected = versionSkew,
                    // Intentionally omit direct download links that would 404 before Release publication
                    zipDownloadUrl = null,
                    zipSizeBytes = 0L,
                    apkDownloadUrl = null,
                    apkSizeBytes = 0L,
                    releasePageUrl = manifest.releasePageUrl ?: DEFAULT_RELEASE_PAGE,
                    changelogSummary = published?.changelogSummary.orEmpty(),
                    checkedAtMs = nowMs,
                )
            }
        }

        // Standard path: prefer verified published GitHub Release; fallback to update.json if API failed.
        if (published != null) {
            val modTargetCode = if (manifest != null && normalizeVersion(manModVer).equals(normalizeVersion(pubTag), ignoreCase = true)) {
                manifest.moduleVersionCode
            } else {
                pubCode
            }
            val appTargetCode = if (manifest != null && normalizeVersion(manAppVer).equals(normalizeVersion(pubTag), ignoreCase = true)) {
                manifest.companionVersionCode
            } else {
                pubCode
            }
            val modUpdate = !installed.moduleInstalled ||
                isRemoteNewer(pubTag, modTargetCode, installed.moduleVersion, installed.moduleVersionCode, isCompanionApp = false)
            val appUpdate = isRemoteNewer(
                pubTag,
                appTargetCode,
                installed.appVersion,
                installed.appVersionCode,
                isCompanionApp = true,
            )
            val anyUpdate = modUpdate || appUpdate
            return UpdateCheckReport(
                state = if (anyUpdate) UpdateState.UPDATE_AVAILABLE else UpdateState.UP_TO_DATE,
                source = "github_release",
                installedAppVersion = normInstalledApp,
                installedAppCode = installed.appVersionCode,
                installedModuleVersion = normInstalledMod,
                installedModuleCode = installed.moduleVersionCode,
                moduleInstalled = installed.moduleInstalled,
                latestVersion = normalizeVersion(pubTag),
                latestModuleCode = modTargetCode,
                latestAppVersion = normalizeVersion(pubTag),
                latestAppCode = appTargetCode,
                moduleUpdateAvailable = modUpdate,
                appUpdateAvailable = appUpdate,
                pendingPublicRelease = false,
                versionSkewDetected = versionSkew,
                zipDownloadUrl = published.zipDownloadUrl,
                zipSizeBytes = published.zipSizeBytes,
                apkDownloadUrl = published.apkDownloadUrl,
                apkSizeBytes = published.apkSizeBytes,
                releasePageUrl = published.releasePageUrl,
                changelogSummary = published.changelogSummary,
                checkedAtMs = nowMs,
            )
        }

        // Fallback: GitHub API rate-limited or unreachable, but `raw.githubusercontent.com/main/module/update.json` succeeded.
        val m = manifest!!
        val modUpdate = !installed.moduleInstalled ||
            isRemoteNewer(m.moduleVersion, m.moduleVersionCode, installed.moduleVersion, installed.moduleVersionCode, isCompanionApp = false)
        val appUpdate = isRemoteNewer(
            m.companionVersion,
            m.companionVersionCode,
            installed.appVersion,
            installed.appVersionCode,
            isCompanionApp = true,
        )
        val anyUpdate = modUpdate || appUpdate
        return UpdateCheckReport(
            state = if (anyUpdate) UpdateState.UPDATE_AVAILABLE else UpdateState.UP_TO_DATE,
            source = "update_json_fallback",
            installedAppVersion = normInstalledApp,
            installedAppCode = installed.appVersionCode,
            installedModuleVersion = normInstalledMod,
            installedModuleCode = installed.moduleVersionCode,
            moduleInstalled = installed.moduleInstalled,
            latestVersion = normalizeVersion(m.moduleVersion),
            latestModuleCode = m.moduleVersionCode,
            latestAppVersion = normalizeVersion(m.companionVersion),
            latestAppCode = m.companionVersionCode,
            moduleUpdateAvailable = modUpdate,
            appUpdateAvailable = appUpdate,
            pendingPublicRelease = false,
            versionSkewDetected = versionSkew,
            zipDownloadUrl = m.zipUrl,
            zipSizeBytes = 0L,
            apkDownloadUrl = m.apkUrl,
            apkSizeBytes = 0L,
            releasePageUrl = m.releasePageUrl ?: DEFAULT_RELEASE_PAGE,
            changelogSummary = "",
            checkedAtMs = nowMs,
        )
    }

    suspend fun checkForUpdates(
        context: Context,
        installed: InstalledSnapshot,
        forceRefresh: Boolean = false,
    ): UpdateCheckReport = withContext(Dispatchers.IO) {
        val nowMs = System.currentTimeMillis()
        val cachedCheckedAt = AppPreferences.getUpdateCheckedAtMs(context)
        val cachedReleaseJson = AppPreferences.getCachedReleaseJson(context)
        val cachedManifestJson = AppPreferences.getCachedManifestJson(context)

        if (!forceRefresh && cachedCheckedAt > 0L && (nowMs - cachedCheckedAt) < CACHE_TTL_MS &&
            (cachedReleaseJson != null || cachedManifestJson != null)
        ) {
            val report = evaluateUpdateReport(
                published = cachedReleaseJson?.let { parsePublishedReleaseJson(it) },
                manifest = cachedManifestJson?.let { parseUpdateManifestJson(it) },
                installed = installed,
                nowMs = cachedCheckedAt,
            )
            return@withContext report.copy(source = "${report.source}:cache")
        }

        var releaseJson: String? = null
        var releaseError: String? = null
        val prevEtag = AppPreferences.getUpdateReleaseEtag(context)
        val relFetch = httpGet(
            url = RELEASES_API_URL,
            accept = "application/vnd.github+json",
            ifNoneMatch = prevEtag,
        )
        when {
            relFetch.code == 200 && relFetch.body != null -> {
                releaseJson = relFetch.body
                AppPreferences.setCachedReleasePayload(context, relFetch.body, relFetch.etag)
            }
            relFetch.code == 304 && cachedReleaseJson != null -> {
                releaseJson = cachedReleaseJson
            }
            else -> {
                releaseError = relFetch.error ?: "GitHub API HTTP ${relFetch.code}"
                // Retain previous cached release metadata only on transient 403 rate-limit
                if (relFetch.code == 403 && cachedReleaseJson != null) {
                    releaseJson = cachedReleaseJson
                }
            }
        }

        var manifestJson: String? = null
        var manifestError: String? = null
        val manFetch = httpGet(
            url = UPDATE_MANIFEST_URL,
            accept = "application/json",
            ifNoneMatch = null,
        )
        if (manFetch.code == 200 && manFetch.body != null) {
            manifestJson = manFetch.body
            AppPreferences.setCachedManifestJson(context, manFetch.body)
        } else {
            manifestError = manFetch.error ?: "Manifest HTTP ${manFetch.code}"
        }

        if (releaseJson != null || manifestJson != null) {
            AppPreferences.setUpdateCheckedAtMs(context, nowMs)
        }

        val combinedError = listOfNotNull(releaseError, manifestError).joinToString(" · ").ifBlank { null }
        evaluateUpdateReport(
            published = releaseJson?.let { parsePublishedReleaseJson(it) },
            manifest = manifestJson?.let { parseUpdateManifestJson(it) },
            installed = installed,
            nowMs = nowMs,
            errorDetail = combinedError,
        )
    }

    private data class HttpFetchResult(
        val code: Int,
        val body: String?,
        val etag: String?,
        val error: String?,
    )

    private fun httpGet(url: String, accept: String, ifNoneMatch: String?): HttpFetchResult {
        if (!isTrustedUrl(url)) {
            return HttpFetchResult(0, null, null, "Blocked untrusted update URL")
        }
        var conn: HttpURLConnection? = null
        return try {
            conn = (URL(url).openConnection() as HttpURLConnection).apply {
                requestMethod = "GET"
                connectTimeout = 6000
                readTimeout = 8000
                instanceFollowRedirects = false
                setRequestProperty("Accept", accept)
                setRequestProperty("User-Agent", "CoreFlow-Companion/${BuildConfig.VERSION_NAME}")
                if (!ifNoneMatch.isNullOrBlank()) {
                    setRequestProperty("If-None-Match", ifNoneMatch)
                }
            }
            val code = conn.responseCode
            val etag = conn.getHeaderField("ETag")
            if (code == 304) {
                HttpFetchResult(304, null, etag, null)
            } else if (code in 200..299) {
                val body = conn.inputStream.use { stream ->
                    val out = ByteArrayOutputStream()
                    val buf = ByteArray(4096)
                    var total = 0
                    while (true) {
                        val n = stream.read(buf)
                        if (n <= 0) break
                        total += n
                        if (total > MAX_BODY_BYTES) return HttpFetchResult(code, null, etag, "Response too large")
                        out.write(buf, 0, n)
                    }
                    out.toString(Charsets.UTF_8.name())
                }
                HttpFetchResult(code, body, etag, null)
            } else {
                HttpFetchResult(code, null, etag, "HTTP $code")
            }
        } catch (t: Throwable) {
            HttpFetchResult(0, null, null, t.message ?: t.javaClass.simpleName)
        } finally {
            conn?.disconnect()
        }
    }

    /**
     * Minimal, zero-dependency pure-Kotlin JSON parser so update parsing runs identically
     * on Android runtime and pure JVM unit tests without `org.json` stub limitations.
     */
    internal object MiniJson {
        fun parseObject(input: String): Map<String, Any?>? {
            val parser = Parser(input)
            val value = runCatching { parser.parseValue() }.getOrNull() ?: return null
            @Suppress("UNCHECKED_CAST")
            return value as? Map<String, Any?>
        }

        private class Parser(private val s: String) {
            private var i = 0

            fun parseValue(): Any? {
                skipWs()
                if (i >= s.length) return null
                return when (s[i]) {
                    '{' -> parseObj()
                    '[' -> parseArr()
                    '"' -> parseStr()
                    't' -> expectLiteral("true", true)
                    'f' -> expectLiteral("false", false)
                    'n' -> expectLiteral("null", null)
                    else -> parseNumber()
                }
            }

            private fun parseObj(): Map<String, Any?> {
                val map = LinkedHashMap<String, Any?>()
                i++ // '{'
                skipWs()
                if (i < s.length && s[i] == '}') {
                    i++
                    return map
                }
                while (i < s.length) {
                    skipWs()
                    val key = parseStr()
                    skipWs()
                    if (i >= s.length || s[i] != ':') error("Expected ':'")
                    i++
                    map[key] = parseValue()
                    skipWs()
                    if (i < s.length && s[i] == ',') {
                        i++
                        continue
                    }
                    if (i < s.length && s[i] == '}') {
                        i++
                        return map
                    }
                    error("Expected ',' or '}'")
                }
                error("Unterminated object")
            }

            private fun parseArr(): List<Any?> {
                val list = ArrayList<Any?>()
                i++ // '['
                skipWs()
                if (i < s.length && s[i] == ']') {
                    i++
                    return list
                }
                while (i < s.length) {
                    list.add(parseValue())
                    skipWs()
                    if (i < s.length && s[i] == ',') {
                        i++
                        continue
                    }
                    if (i < s.length && s[i] == ']') {
                        i++
                        return list
                    }
                    error("Expected ',' or ']'")
                }
                error("Unterminated array")
            }

            private fun parseStr(): String {
                if (i >= s.length || s[i] != '"') error("Expected '\"'")
                i++
                val sb = StringBuilder()
                while (i < s.length) {
                    val ch = s[i++]
                    when (ch) {
                        '"' -> return sb.toString()
                        '\\' -> {
                            if (i >= s.length) error("Unterminated escape")
                            when (val esc = s[i++]) {
                                '"', '\\', '/' -> sb.append(esc)
                                'b' -> sb.append('\b')
                                'f' -> sb.append('\u000C')
                                'n' -> sb.append('\n')
                                'r' -> sb.append('\r')
                                't' -> sb.append('\t')
                                'u' -> {
                                    if (i + 4 > s.length) error("Invalid unicode escape")
                                    val code = s.substring(i, i + 4).toInt(16)
                                    sb.append(code.toChar())
                                    i += 4
                                }
                                else -> sb.append(esc)
                            }
                        }
                        else -> sb.append(ch)
                    }
                }
                error("Unterminated string")
            }

            private fun parseNumber(): Number {
                val start = i
                if (i < s.length && s[i] == '-') i++
                while (i < s.length && s[i] in '0'..'9') i++
                var isFloat = false
                if (i < s.length && s[i] == '.') {
                    isFloat = true
                    i++
                    while (i < s.length && s[i] in '0'..'9') i++
                }
                if (i < s.length && (s[i] == 'e' || s[i] == 'E')) {
                    isFloat = true
                    i++
                    if (i < s.length && (s[i] == '+' || s[i] == '-')) i++
                    while (i < s.length && s[i] in '0'..'9') i++
                }
                val token = s.substring(start, i)
                return if (isFloat) token.toDouble() else token.toLong()
            }

            private fun expectLiteral(lit: String, value: Any?): Any? {
                if (!s.startsWith(lit, i)) error("Expected $lit")
                i += lit.length
                return value
            }

            private fun skipWs() {
                while (i < s.length && (s[i] == ' ' || s[i] == '\n' || s[i] == '\r' || s[i] == '\t')) {
                    i++
                }
            }
        }
    }
}
