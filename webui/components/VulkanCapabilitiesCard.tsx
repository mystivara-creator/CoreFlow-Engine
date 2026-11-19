import React from 'react';
import { Check, Cpu, Shield, X } from 'lucide-react';
import {
  computeVulkanTelemetry,
  VulkanEngineConfig,
  VulkanLayerMode,
  VulkanTelemetry,
} from '../engine/autonomousEngine';

interface VulkanCapabilitiesCardProps {
  vulkanTelemetry?: VulkanTelemetry;
  vulkanConfig: VulkanEngineConfig;
  nativeMode: boolean;
  onSaveVulkanConfig: (next: VulkanEngineConfig) => Promise<void>;
  isSaving: boolean;
  saveMessage: string | null;
}

interface CapabilityRowProps {
  label: string;
  simulated?: boolean;
  value: boolean | string | number;
  note?: string;
}

const CapabilityRow: React.FC<CapabilityRowProps> = ({
  label,
  value,
  note,
  simulated = false,
}) => {
  const isBool = typeof value === 'boolean';
  return (
    <div
      className="flex items-center justify-between py-2.5 last:border-b-0 text-xs"
      style={{ borderBottom: '1px solid var(--md-sys-color-outline-variant)' }}
    >
      <div className="pr-3">
        <span>
          {label}
          {simulated ? ' (sim)' : ''}
        </span>
        {note && (
          <span
            className="text-[11px]"
            style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
          >
            {' '}
            · {note}
          </span>
        )}
      </div>
      {isBool ? (
        <div
          className="inline-flex items-center gap-1.5 font-bold font-mono tabular-nums shrink-0"
          style={{
            color: value
              ? 'var(--md-sys-color-success)'
              : 'var(--md-sys-color-on-surface-variant)',
          }}
        >
          {value ? (
            <>
              <Check className="w-3.5 h-3.5 shrink-0" />
              <span>Yes</span>
            </>
          ) : (
            <>
              <X className="w-3.5 h-3.5 shrink-0 opacity-60" />
              <span>No</span>
            </>
          )}
        </div>
      ) : (
        <span className="font-bold font-mono tabular-nums shrink-0">{value}</span>
      )}
    </div>
  );
};

export const VulkanCapabilitiesCard: React.FC<VulkanCapabilitiesCardProps> = ({
  vulkanTelemetry,
  vulkanConfig,
  nativeMode,
  onSaveVulkanConfig,
  isSaving,
  saveMessage,
}) => {
  const vk = computeVulkanTelemetry(vulkanConfig, vulkanTelemetry);
  const badgeFor = (active: boolean, hwSupported?: boolean): string | undefined => {
    if (!active) return undefined;
    return hwSupported ? 'HW Native' : undefined;
  };

  const handleSelectMode = async (mode: VulkanLayerMode) => {
    const normalizedMode: VulkanLayerMode =
      mode === 'disabled' ? 'disabled' : 'safe_hardware';
    const updated: VulkanEngineConfig = { ...vulkanConfig, mode: normalizedMode };
    await onSaveVulkanConfig(updated);
  };

  return (
    <div className="space-y-6">
      {/* Primary Policy Control Surface */}
      <div className="md3-card-elevated p-6 sm:p-8 space-y-6">
        <div className="flex flex-col lg:flex-row lg:items-start lg:justify-between gap-4">
          <div className="space-y-1.5 max-w-2xl">
            <div className="flex flex-wrap items-center gap-2">
              <span className="md3-badge md3-badge-primary">
                {nativeMode ? 'LIVE DEVICE ICD PROBE' : 'HARDWARE PASS-THROUGH MODEL (SIM)'}
              </span>
              <span className="md3-badge md3-badge-neutral font-mono">
                {vk.deviceName}
              </span>
            </div>
            <h2 className="text-xl sm:text-2xl font-bold tracking-tight">
              Vulkan Capabilities &amp; Truthful Hardware Pass-Through{!nativeMode ? ' (sim)' : ''}
            </h2>
            <p
              className="text-xs sm:text-sm leading-relaxed"
              style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
            >
              Dynamically probes the active device&apos;s physical GPU ICD (Qualcomm Adreno KGSL, ARM Mali / Immortalis, Samsung Xclipse, PowerVR) and reports genuine hardware capabilities without fabricating unsupported extensions, feature flags, or API versions. Opt-in diagnostic layer{' '}
              <code className="font-mono text-xs">libVkLayer_coreflow.so</code> requires{' '}
              <code className="font-mono text-xs">ENABLE_COREFLOW_VULKAN_LAYER=1</code>.
            </p>
          </div>

          <div className="md3-card-filled px-4 py-3 text-left lg:text-right font-mono tabular-nums text-xs shrink-0 space-y-1">
            <div>
              <span style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>Hardware API: </span>
              <span className="font-bold">Vulkan {vk.apiVersion}</span>
            </div>
            <div>
              <span style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>Device Extensions: </span>
              <span className="font-bold">{vk.deviceExtensions}</span>
              <span style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
                {' '}· Instance: {vk.instanceExtensions}
              </span>
            </div>
          </div>
        </div>

        {/* M3 Mode Selector Cards */}
        <div className="space-y-3">
          <div className="text-xs font-bold">Select Vulkan Layer Mode</div>
          <div className="grid grid-cols-1 md:grid-cols-2 gap-3">
            <button
              type="button"
              id="btn-vulkan-mode-disabled"
              disabled={isSaving}
              aria-pressed={vulkanConfig.mode === 'disabled'}
              onClick={() => void handleSelectMode('disabled')}
              className="text-left p-4 rounded-2xl transition-all"
              style={{
                backgroundColor:
                  vulkanConfig.mode === 'disabled'
                    ? 'var(--md-sys-color-primary-container)'
                    : 'var(--md-sys-color-surface-container)',
                color:
                  vulkanConfig.mode === 'disabled'
                    ? 'var(--md-sys-color-on-primary-container)'
                    : 'var(--md-sys-color-on-surface)',
                border: '1px solid var(--md-sys-color-outline-variant)',
              }}
            >
              <div className="flex items-center justify-between gap-2 mb-1.5">
                <span className="text-xs font-bold">
                  Default · Implicit Layer Disabled (Recommended)
                </span>
                <Cpu className="w-4 h-4 shrink-0" />
              </div>
              <p className="text-[11px] leading-relaxed opacity-85">
                Keeps <code className="font-mono">VK_LAYER_COREFLOW_universal_bridge</code> inactive for normal applications while still reading native GPU devfreq and physical ICD capabilities.
              </p>
            </button>

            <button
              type="button"
              id="btn-vulkan-mode-passthrough"
              disabled={isSaving}
              aria-pressed={vulkanConfig.mode !== 'disabled'}
              onClick={() => void handleSelectMode('safe_hardware')}
              className="text-left p-4 rounded-2xl transition-all"
              style={{
                backgroundColor:
                  vulkanConfig.mode !== 'disabled'
                    ? 'var(--md-sys-color-primary-container)'
                    : 'var(--md-sys-color-surface-container)',
                color:
                  vulkanConfig.mode !== 'disabled'
                    ? 'var(--md-sys-color-on-primary-container)'
                    : 'var(--md-sys-color-on-surface)',
                border: '1px solid var(--md-sys-color-outline-variant)',
              }}
            >
              <div className="flex items-center justify-between gap-2 mb-1.5">
                <span className="text-xs font-bold">
                  Truthful Hardware Pass-Through
                </span>
                <Shield className="w-4 h-4 shrink-0" />
              </div>
              <p className="text-[11px] leading-relaxed opacity-85">
                Uses per-handle 64-bit aligned Vulkan dispatch and forwards 100% authentic ICD capabilities without spoofing API versions, injecting extensions, or stripping <code className="font-mono">vkCreateDevice</code> requests.
              </p>
            </button>
          </div>
        </div>

        <div
          className="flex flex-col gap-3 pt-4 sm:flex-row sm:items-center sm:justify-between"
          style={{ borderTop: '1px solid var(--md-sys-color-outline-variant)' }}
        >
          <div
            className="max-w-2xl text-xs leading-relaxed"
            style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
          >
            <p className="font-bold" style={{ color: 'var(--md-sys-color-on-surface)' }}>
              Manual Vulkan Diagnostic Layer
            </p>
            <p className="mt-0.5">
              This opt-in controls only the Vulkan diagnostic / pass-through layer. All CPU, memory, storage, and thermal policies remain daemon-owned.
            </p>
          </div>
          <div className="flex shrink-0 items-center gap-2.5">
            {isSaving ? (
              <span className="text-xs font-semibold">Applying Vulkan mode…</span>
            ) : (
              saveMessage && (
                <span
                  className="text-xs"
                  style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
                >
                  {saveMessage}
                </span>
              )
            )}
            <span
              className={
                vulkanConfig.mode === 'disabled'
                  ? 'md3-badge md3-badge-neutral'
                  : 'md3-badge md3-badge-primary'
              }
            >
              {vulkanConfig.mode === 'disabled' ? 'Layer Disabled' : 'Layer Enabled'}
            </span>
          </div>
        </div>
      </div>

      {/* Capability Grid */}
      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
        {/* Card 1: Capabilities & Device Identity */}
        <div className="md3-card-elevated p-6 space-y-4">
          <div className="flex items-center justify-between">
            <h3 className="text-sm font-bold">Capabilities &amp; Identity</h3>
            <span className="md3-badge md3-badge-primary font-mono">{vk.vendor}</span>
          </div>
          <div>
            <CapabilityRow
              simulated={!nativeMode}
              label="Ray Tracing"
              value={vk.rayTracing}
              note={badgeFor(vk.rayTracing, vk.hwRayTracing)}
            />
            <CapabilityRow
              simulated={!nativeMode}
              label="Mesh Shaders"
              value={vk.meshShaders}
              note={badgeFor(vk.meshShaders, vk.hwMeshShaders)}
            />
            <CapabilityRow
              simulated={!nativeMode}
              label="Variable Rate Shading"
              value={vk.variableRateShading}
              note={badgeFor(vk.variableRateShading, vk.hwVariableRateShading)}
            />
            <CapabilityRow
              simulated={!nativeMode}
              label="Cooperative Matrix"
              value={vk.cooperativeMatrix}
              note={badgeFor(vk.cooperativeMatrix, vk.hwCooperativeMatrix)}
            />
            <CapabilityRow
              simulated={!nativeMode}
              label="RT Pipeline"
              value={vk.rtPipeline}
              note={badgeFor(vk.rtPipeline, vk.hwRtPipeline)}
            />
            <CapabilityRow
              simulated={!nativeMode}
              label="Ray Query"
              value={vk.rayQuery}
              note={badgeFor(vk.rayQuery, vk.hwRayQuery)}
            />
            <CapabilityRow
              simulated={!nativeMode}
              label="Acceleration Structure"
              value={vk.accelerationStructure}
              note={badgeFor(vk.accelerationStructure, vk.hwAccelerationStructure)}
            />
            <CapabilityRow
              simulated={!nativeMode}
              label="Portability Subset"
              value={vk.portabilitySubset}
            />
            <CapabilityRow
              simulated={!nativeMode}
              label="Device Extensions"
              value={vk.deviceExtensions}
              note="HW Native ICD"
            />
            <CapabilityRow
              simulated={!nativeMode}
              label="Instance Extensions"
              value={vk.instanceExtensions}
            />
          </div>
          <div
            className="pt-2 space-y-1.5 text-[11px] font-mono"
            style={{
              borderTop: '1px solid var(--md-sys-color-outline-variant)',
              color: 'var(--md-sys-color-on-surface-variant)',
            }}
          >
            <div className="truncate" title={vk.deviceUuid}>
              Device UUID: {vk.deviceUuid}
            </div>
            <div className="truncate" title={vk.driverUuid}>
              Driver UUID: {vk.driverUuid}
            </div>
            <div className="truncate" title={vk.pipelineCacheUuid}>
              Cache UUID: {vk.pipelineCacheUuid}
            </div>
          </div>
        </div>

        {/* Card 2: Core Features & Vulkan 1.1 / 1.2 / 1.3 */}
        <div className="md3-card-elevated p-6 space-y-4">
          <div className="flex items-center justify-between">
            <h3 className="text-sm font-bold">Core &amp; Vulkan 1.1–1.3</h3>
            <span
              className="text-xs font-mono"
              style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
            >
              Spec Matrix
            </span>
          </div>
          <div>
            <CapabilityRow simulated={!nativeMode} label="Robust Buffer Access" value={vk.robustBufferAccess} />
            <CapabilityRow simulated={!nativeMode} label="Sampler Anisotropy" value={vk.samplerAnisotropy} />
            <CapabilityRow simulated={!nativeMode} label="16-Bit Integers" value={vk.shaderInt16} />
            <CapabilityRow
              simulated={!nativeMode}
              label="64-Bit Floats (FP64)"
              value={vk.shaderFloat64}
              note={badgeFor(vk.shaderFloat64, vk.hwShaderFloat64)}
            />
            <CapabilityRow
              simulated={!nativeMode}
              label="Protected Memory"
              value={vk.protectedMemory}
              note="Vulkan 1.1"
            />
            <CapabilityRow
              simulated={!nativeMode}
              label="Timeline Semaphore"
              value={vk.timelineSemaphore}
              note="Vulkan 1.2"
            />
            <CapabilityRow
              simulated={!nativeMode}
              label="Descriptor Indexing"
              value={vk.descriptorIndexing}
              note="Vulkan 1.2"
            />
            <CapabilityRow
              simulated={!nativeMode}
              label="Buffer Device Address"
              value={vk.bufferDeviceAddress}
              note="Vulkan 1.2"
            />
            <CapabilityRow
              simulated={!nativeMode}
              label="Dynamic Rendering"
              value={vk.dynamicRendering}
              note="Vulkan 1.3"
            />
            <CapabilityRow
              simulated={!nativeMode}
              label="Synchronization2"
              value={vk.synchronization2}
              note="Vulkan 1.3"
            />
          </div>
        </div>

        {/* Card 3: Vulkan 1.4, Highlights & GPU Kernel Surface */}
        <div className="md3-card-elevated p-6 space-y-4">
          <div className="flex items-center justify-between">
            <h3 className="text-sm font-bold">Vulkan 1.4 &amp; GPU Surface</h3>
            <span className="md3-badge md3-badge-secondary font-mono">
              API {vk.apiVersion}
            </span>
          </div>
          <div>
            <CapabilityRow
              simulated={!nativeMode}
              label="8-Bit Indices (indexTypeUint8)"
              value={vk.indexTypeUint8}
              note={badgeFor(vk.indexTypeUint8, vk.hwIndexTypeUint8)}
            />
            <CapabilityRow
              simulated={!nativeMode}
              label="Line Rasterization"
              value={vk.lineRasterization}
              note={badgeFor(vk.lineRasterization, vk.hwLineRasterization)}
            />
            <CapabilityRow
              simulated={!nativeMode}
              label="Push Descriptors"
              value={vk.pushDescriptors}
              note={badgeFor(vk.pushDescriptors, vk.hwPushDescriptors)}
            />
            <CapabilityRow
              simulated={!nativeMode}
              label="Global Priority Query"
              value={vk.globalPriorityQuery}
              note={badgeFor(vk.globalPriorityQuery, vk.hwGlobalPriorityQuery)}
            />
            <CapabilityRow
              simulated={!nativeMode}
              label="Shader Float Controls 2"
              value={vk.shaderFloatControls2}
              note={badgeFor(vk.shaderFloatControls2, vk.hwShaderFloatControls2)}
            />
            <CapabilityRow
              simulated={!nativeMode}
              label="Max Push Descriptors"
              value={vk.maxPushDescriptors}
            />
            <CapabilityRow
              simulated={!nativeMode}
              label="Maintenance 5 / 6 / 7"
              value={`${vk.maintenance5 ? 'M5' : '—'} · ${vk.maintenance6 ? 'M6' : '—'} · ${vk.maintenance7 ? 'M7' : '—'}`}
            />
            <CapabilityRow
              simulated={!nativeMode}
              label="Descriptor Buffer"
              value={vk.descriptorBuffer}
              note={badgeFor(vk.descriptorBuffer, vk.hwDescriptorBuffer)}
            />
            <CapabilityRow
              simulated={!nativeMode}
              label="Graphics Pipeline Library"
              value={vk.graphicsPipelineLibrary}
              note={badgeFor(vk.graphicsPipelineLibrary, vk.hwGraphicsPipelineLibrary)}
            />
          </div>

          {/* Multi-Vendor GPU Kernel Surface */}
          <div
            className="pt-3 space-y-1.5"
            style={{ borderTop: '1px solid var(--md-sys-color-outline-variant)' }}
          >
            <div className="text-xs font-bold">
              Multi-Vendor GPU Kernel Surface ({vk.gpuKernelLabel})
            </div>
            <CapabilityRow
              simulated={!nativeMode}
              label="Active Governor"
              value={vk.gpuGovernor}
            />
            <CapabilityRow
              simulated={!nativeMode}
              label="GPU Clock (Cur / Max)"
              value={`${vk.gpuCurrentFreqMhz} / ${vk.gpuMaxFreqMhz} MHz`}
            />
            <CapabilityRow
              simulated={!nativeMode}
              label="Adrenoboost / Pacing"
              value={`Level ${vk.adrenoboostLevel}`}
            />
          </div>
        </div>
      </div>
    </div>
  );
};
