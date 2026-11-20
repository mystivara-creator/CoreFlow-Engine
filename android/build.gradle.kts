import java.util.Properties

plugins {
    id("com.android.application") version "9.4.1"
    id("org.jetbrains.kotlin.plugin.compose") version "2.4.20"
}

val localProps = Properties().apply {
    runCatching { load(rootProject.file("local.properties").inputStream()) }
}
val keystorePath: String? = localProps.getProperty("KEYSTORE_PATH") ?: System.getenv("KEYSTORE_PATH")
val keystorePass: String? = localProps.getProperty("KEYSTORE_PASS") ?: System.getenv("KEYSTORE_PASS")
val keyAliasProp: String? = localProps.getProperty("KEY_ALIAS") ?: System.getenv("KEY_ALIAS")
val keyPassProp: String? = localProps.getProperty("KEY_PASSWORD") ?: System.getenv("KEY_PASSWORD")

android {
    namespace = "io.coreflow"
    compileSdk = 37

    defaultConfig {
        applicationId = "io.coreflow.companion"
        minSdk = 33
        targetSdk = 36
        versionCode = 60000
        versionName = "6.0.0"
    }

    signingConfigs {
        if (!keystorePath.isNullOrBlank()) {
            create("release") {
                storeFile = file(keystorePath)
                storePassword = keystorePass
                keyAlias = keyAliasProp
                keyPassword = keyPassProp
                enableV3Signing = true
                enableV4Signing = true
            }
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = true
            isShrinkResources = true
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro",
            )
            signingConfig = if (!keystorePath.isNullOrBlank()) {
                signingConfigs.getByName("release")
            } else {
                signingConfigs.getByName("debug")
            }
        }
        debug {
            applicationIdSuffix = ".debug"
            versionNameSuffix = "-debug"
        }
    }

    buildFeatures {
        compose = true
        buildConfig = true
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_21
        targetCompatibility = JavaVersion.VERSION_21
    }

    packaging {
        resources {
            excludes += setOf(
                "/META-INF/{AL2.0,LGPL2.1}",
                "/META-INF/DEPENDENCIES",
                "/META-INF/*.kotlin_module",
            )
        }
    }
}

dependencies {
    val miuixVersion = "0.9.4"

    implementation("top.yukonga.miuix.kmp:miuix-ui:$miuixVersion")
    implementation("top.yukonga.miuix.kmp:miuix-preference:$miuixVersion")
    implementation("top.yukonga.miuix.kmp:miuix-icons:$miuixVersion")

    implementation("androidx.activity:activity-compose:1.13.0")
    implementation("androidx.core:core-ktx:1.16.0")
    implementation("androidx.annotation:annotation:1.9.1")
    implementation("androidx.lifecycle:lifecycle-runtime-ktx:2.9.0")
    implementation("androidx.lifecycle:lifecycle-runtime-compose:2.9.0")
    implementation("org.jetbrains.kotlinx:kotlinx-coroutines-android:1.10.2")

    // Persistent root shell for Magisk, KernelSU, KernelSU-Next & APatch
    implementation("com.github.topjohnwu.libsu:core:6.0.0")
}
