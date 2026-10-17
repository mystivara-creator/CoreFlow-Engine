# CoreFlow source manifest.
# Keep platform-independent sources separate from daemon/Android integration.

set(COREFLOW_CORE_SOURCES
    src/observe/ecosystem_knowledge.cpp
    src/observe/high_capability.cpp
    src/observe/capability_registry.cpp
    src/observe/kernel_roots.cpp
    src/observe/vendor_knowledge.cpp
    src/observe/tunable_discovery.cpp

    src/understand/context.cpp
    src/understand/effect_model.cpp
    src/understand/decision_agent.cpp
    src/understand/experience.cpp
    src/understand/thermal_features.cpp
    src/understand/thermal_guard.cpp
    src/understand/baseline_intelligence.cpp

    src/balance/policy.cpp
    src/balance/control.cpp
    src/balance/safety_hold.cpp
    src/balance/resource_model.cpp
    src/balance/consultation.cpp

    src/mutation/resource_actuator.cpp
    src/mutation/resource_mutation.cpp
    src/mutation/cpufreq_actuator.cpp
    src/mutation/mutation.cpp

    src/conscience/mutation_journal.cpp
    src/conscience/retained_state.cpp
    src/conscience/fatal_surface.cpp

    src/runtime/config.cpp
    src/runtime/status_snapshot.cpp
)

set(COREFLOW_ANDROID_SOURCES
    src/runtime/main.cpp
    src/autonomous/autonomous.cpp
    src/understand/thermal_predictor.cpp
    src/conscience/decision_trace.cpp
    src/observe/discovery.cpp
    src/observe/observer.cpp
)
