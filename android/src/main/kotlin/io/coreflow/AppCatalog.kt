package io.coreflow

import android.content.Context
import android.content.Intent
import android.content.pm.ApplicationInfo
import android.content.pm.PackageManager
import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.drawable.BitmapDrawable
import android.graphics.drawable.Drawable
import androidx.compose.ui.graphics.ImageBitmap
import androidx.compose.ui.graphics.asImageBitmap
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

data class InstalledAppEntry(
    val packageName: String,
    val label: String,
    val isGame: Boolean,
    val isSystem: Boolean,
    val icon: ImageBitmap?,
)

object AppCatalog {

    suspend fun loadLaunchableApps(context: Context): List<InstalledAppEntry> = withContext(Dispatchers.IO) {
        val pm = context.packageManager
        val launcherIntent = Intent(Intent.ACTION_MAIN).addCategory(Intent.CATEGORY_LAUNCHER)
        val resolved = pm.queryIntentActivities(launcherIntent, PackageManager.ResolveInfoFlags.of(0))
        val seen = HashSet<String>(resolved.size)
        val entries = ArrayList<InstalledAppEntry>(resolved.size)

        for (ri in resolved) {
            val appInfo = ri.activityInfo?.applicationInfo ?: continue
            val pkg = appInfo.packageName ?: continue
            if (pkg == context.packageName || !seen.add(pkg)) continue

            val label = runCatching { pm.getApplicationLabel(appInfo).toString() }
                .getOrDefault(pkg)
                .ifBlank { pkg }
            val isGame = appInfo.category == ApplicationInfo.CATEGORY_GAME
            val isSystem = (appInfo.flags and ApplicationInfo.FLAG_SYSTEM) != 0 &&
                (appInfo.flags and ApplicationInfo.FLAG_UPDATED_SYSTEM_APP) == 0
            val icon = runCatching { pm.getApplicationIcon(appInfo).toSmallImageBitmap(72) }.getOrNull()

            entries.add(
                InstalledAppEntry(
                    packageName = pkg,
                    label = label,
                    isGame = isGame,
                    isSystem = isSystem,
                    icon = icon,
                ),
            )
        }

        entries.sortWith(compareByDescending<InstalledAppEntry> { it.isGame }.thenBy { it.label.lowercase() })
        entries
    }

    private fun Drawable.toSmallImageBitmap(sizePx: Int): ImageBitmap {
        if (this is BitmapDrawable && bitmap != null) {
            val scaled = if (bitmap.width > sizePx || bitmap.height > sizePx) {
                Bitmap.createScaledBitmap(bitmap, sizePx, sizePx, true)
            } else {
                bitmap
            }
            return scaled.asImageBitmap()
        }
        val bmp = Bitmap.createBitmap(sizePx, sizePx, Bitmap.Config.ARGB_8888)
        val canvas = Canvas(bmp)
        setBounds(0, 0, sizePx, sizePx)
        draw(canvas)
        return bmp.asImageBitmap()
    }
}
