# CoreFlow Companion R8 / ProGuard rules
-keepattributes *Annotation*
-keep class com.topjohnwu.superuser.** { *; }
-dontwarn com.topjohnwu.superuser.**
-keep class io.coreflow.CoreFlowTileService { *; }
-keep class io.coreflow.CoreFlowBypassTileService { *; }
-keep class io.coreflow.CoreFlowThermalReceiver { *; }
-keep class io.coreflow.CoreFlowOverlayService { *; }
-keep class ai.onnxruntime.** { *; }
-dontwarn ai.onnxruntime.**
