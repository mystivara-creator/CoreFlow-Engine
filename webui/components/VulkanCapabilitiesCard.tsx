import React from 'react';
import { Check, Cpu, Layers, Shield, Sliders, X } from 'lucide-react';
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
  onChangeVulkanConfig: (next: VulkanEngineConfig) => void;
  onSaveVulkanConfig: (next: VulkanEngineConfig) => Promise<void>;
  isSaving: boolean;
  saveMessage: string | null;
}

interface CapabilityRowProps {
  label: string;
  value: boolean | string | number;
  note?: string;
}

const CapabilityRow: React.FC<CapabilityRowProps> = ({ label, value, note }) => {
  const isBool = typeof value === 'boolean';
  return (
    <div className="flex items-center justify-between py-2.5 border-b border-slate-900/[0.05] dark:border-white/[0.05] last:border-b-0 text-xs">
      <div className="text-slate-600 dark:text-slate-300 pr-3">
        <span>{label}</span>
        {note && (
          <span className="text-[11px] text-slate-400 dark:text-slate-500"> · {note}</span>
        )}
      </div>
      {isBool ? (
        <div
          className={
            'inline-flex items-center gap-1.5 font-semibold font-mono tabular-nums shrink-0 ' +
            (value
              ? 'text-emerald-700 dark:text-emerald-400'
              : 'text-slate-500 dark:text-slate-400')
          }
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
        <span className="font-semibold font-mono tabular-nums text-slate-900 dark:text-slate-100 shrink-0">
          {value}
        </span>
      )}
    </div>
  );
};

export const VulkanCapabilitiesCard: React.FC<VulkanCapabilitiesCardProps> = ({
  vulkanTelemetry,
  vulkanConfig,
  nativeMode,
  onChangeVulkanConfig,
  onSaveVulkanConfig,
  isSaving,
  saveMessage,
}) => {
  const vk = computeVulkanTelemetry(vulkanConfig, vulkanTelemetry);
  const badgeFor = (active: boolean, hwSupported?: boolean, isOverrideOnly = false): string | undefined => {
    if (!active) return undefined;
    if (hwSupported) return 'HW Native';
    return isOverrideOnly ? 'Layer Override' : 'VkLayer Bridged';
  };
  const addedExts = Math.max(0, vk.effectiveDeviceExtensions - vk.deviceExtensions);

  const handleSelectMode = async (mode: VulkanLayerMode) => {
    const updated: VulkanEngineConfig = { ...vulkanConfig, mode };
    onChangeVulkanConfig(updated);
    await onSaveVulkanConfig(updated);
  };

  const handleToggleOption = (key: keyof Omit<VulkanEngineConfig, 'mode'>) => {
    const updated: VulkanEngineConfig = {
      ...vulkanConfig,
      [key]: !vulkanConfig[key],
    };
    onChangeVulkanConfig(updated);
  };

  return (
    <div className="space-y-6">
      {/* Primary Policy Control Surface */}
      <div className="rounded-[28px] bg-white dark:bg-[#161a22] border border-slate-900/[0.06] dark:border-white/[0.07] p-6 sm:p-8 space-y-6">
        <div className="flex flex-col lg:flex-row lg:items-start lg:justify-between gap-4">
          <div className="space-y-1.5 max-w-2xl">
            <div className="text-xs text-slate-500 dark:text-slate-400">
              <span>Universal Multi-Vendor GPU & Vulkan 1.4 Architecture</span>
              <span aria-hidden="true"> · </span>
              <span>{vk.deviceName}</span>
              <span aria-hidden="true"> · </span>
              <span>{nativeMode ? 'Live Device ICD Probe' : 'Interactive Capability Model'}</span>
            </div>
            <h2 className="text-xl sm:text-2xl font-bold tracking-tight text-slate-900 dark:text-slate-100">
              Vulkan Capabilities & Implicit Layer Bridge
            </h2>
            <p className="text-xs sm:text-sm text-slate-600 dark:text-slate-400 leading-relaxed">
              Dynamically probes the active device&apos;s physical GPU ICD (Qualcomm Adreno KGSL, ARM Mali /
              Immortalis, Samsung Xclipse, PowerVR) and bridges missing Vulkan 1.4 features via{' '}
              <code className="font-mono text-xs">libVkLayer_coreflow.so</code> with{' '}
              <code className="font-mono text-xs">vkCreateDevice</code> extension guard.
            </p>
          </div>

          <div className="text-left lg:text-right font-mono tabular-nums text-xs text-slate-600 dark:text-slate-400 shrink-0 space-y-1">
            <div>
              Effective API:{' '}
              <span className="font-semibold text-slate-900 dark:text-slate-100">
                Vulkan {vk.effectiveApiVersion}
              </span>{' '}
              <span className="text-slate-400">(HW {vk.apiVersion})</span>
            </div>
            <div>
              Device Extensions:{' '}
              <span className="font-semibold text-slate-900 dark:text-slate-100">
                {vk.effectiveDeviceExtensions}
              </span>{' '}
              <span className="text-slate-400">
                (HW Base {vk.deviceExtensions}{addedExts > 0 ? ` + ${addedExts} Bridged` : ''})
              </span>
              {' · '}Instance: {vk.instanceExtensions}
            </div>
          </div>
        </div>

        {/* Mode Selector (Opsi A vs Opsi B vs OEM Passthrough) */}
        <div className="space-y-3">
          <div className="text-xs font-semibold text-slate-700 dark:text-slate-300">
            Select Capability Interception Mode
          </div>
          <div className="grid grid-cols-1 md:grid-cols-3 gap-3">
            <button
              type="button"
              onClick={() => void handleSelectMode('safe_hardware')}
              className={
                'text-left p-4 rounded-2xl border transition-colors ' +
                (vulkanConfig.mode === 'safe_hardware'
                  ? 'bg-[#d3e3fd]/45 dark:bg-[#0842a0]/30 border-[#0842a0]/40 dark:border-[#d3e3fd]/40 text-slate-900 dark:text-slate-100'
                  : 'bg-slate-50/70 dark:bg-[#1b202a] border-slate-900/[0.06] dark:border-white/[0.06] text-slate-700 dark:text-slate-300 hover:bg-slate-100/80 dark:hover:bg-[#202632]')
              }
            >
              <div className="flex items-center justify-between gap-2 mb-1">
                <span className="text-xs font-bold whitespace-nowrap">
                  Opsi A · Safe Hardware & 1.4 Bridge
                </span>
                <Shield className="w-4 h-4 shrink-0 text-sky-600 dark:text-sky-400" />
              </div>
              <p className="text-[11px] text-slate-600 dark:text-slate-400 leading-relaxed">
                Completes Vulkan 1.4 features missing on this device (Index UINT8, Line Rasterization,
                Push Descriptors, Maintenance 5/6/7, Descriptor Buffer, GPL) while preserving native
                hardware capabilities. 100% crash-safe in games.
              </p>
            </button>

            <button
              type="button"
              onClick={() => void handleSelectMode('full_capability_override')}
              className={
                'text-left p-4 rounded-2xl border transition-colors ' +
                (vulkanConfig.mode === 'full_capability_override'
                  ? 'bg-[#d3e3fd]/45 dark:bg-[#0842a0]/30 border-[#0842a0]/40 dark:border-[#d3e3fd]/40 text-slate-900 dark:text-slate-100'
                  : 'bg-slate-50/70 dark:bg-[#1b202a] border-slate-900/[0.06] dark:border-white/[0.06] text-slate-700 dark:text-slate-300 hover:bg-slate-100/80 dark:hover:bg-[#202632]')
              }
            >
              <div className="flex items-center justify-between gap-2 mb-1">
                <span className="text-xs font-bold whitespace-nowrap">
                  Opsi B · Full Capability Override
                </span>
                <Layers className="w-4 h-4 shrink-0 text-sky-600 dark:text-sky-400" />
              </div>
              <p className="text-[11px] text-slate-600 dark:text-slate-400 leading-relaxed">
                Enables all Opsi A bridges plus full Vulkan capability reporting (Ray Tracing, RT
                Pipeline, Ray Query, Mesh Shaders, Cooperative Matrix, Shader Float64) guarded by{' '}
                <code className="font-mono">vkCreateDevice</code> extension filtering.
              </p>
            </button>

            <button
              type="button"
              onClick={() => void handleSelectMode('disabled')}
              className={
                'text-left p-4 rounded-2xl border transition-colors ' +
                (vulkanConfig.mode === 'disabled'
                  ? 'bg-[#d3e3fd]/45 dark:bg-[#0842a0]/30 border-[#0842a0]/40 dark:border-[#d3e3fd]/40 text-slate-900 dark:text-slate-100'
                  : 'bg-slate-50/70 dark:bg-[#1b202a] border-slate-900/[0.06] dark:border-white/[0.06] text-slate-700 dark:text-slate-300 hover:bg-slate-100/80 dark:hover:bg-[#202632]')
              }
            >
              <div className="flex items-center justify-between gap-2 mb-1">
                <span className="text-xs font-bold whitespace-nowrap">
                  OEM Passthrough (Unmodified)
                </span>
                <Cpu className="w-4 h-4 shrink-0 text-slate-500" />
              </div>
              <p className="text-[11px] text-slate-600 dark:text-slate-400 leading-relaxed">
                Disables <code className="font-mono">VK_LAYER_COREFLOW_universal_bridge</code>{' '}
                feature promotion and reports the raw physical vendor ICD readout of this device.
              </p>
            </button>
          </div>
        </div>

        {/* Runtime Pipeline & Multi-Vendor GPU Surface Controls */}
        <div className="pt-4 border-t border-slate-900/[0.06] dark:border-white/[0.06] flex flex-col lg:flex-row lg:items-center justify-between gap-4">
          <div className="flex flex-wrap items-center gap-4 text-xs">
            <label className="inline-flex items-center gap-2 cursor-pointer select-none">
              <input
                type="checkbox"
                checked={vulkanConfig.enableSkiavkThreaded}
                onChange={() => handleToggleOption('enableSkiavkThreaded')}
                className="rounded border-slate-300 dark:border-slate-700"
              />
              <span className="text-slate-700 dark:text-slate-300 font-medium">
                SkiaVK Threaded Renderer
              </span>
            </label>

            <label className="inline-flex items-center gap-2 cursor-pointer select-none">
              <input
                type="checkbox"
                checked={vulkanConfig.enableAdaptiveVrs}
                onChange={() => handleToggleOption('enableAdaptiveVrs')}
                className="rounded border-slate-300 dark:border-slate-700"
              />
              <span className="text-slate-700 dark:text-slate-300 font-medium">
                Adaptive Variable Rate Shading (VRS)
              </span>
            </label>

            <label className="inline-flex items-center gap-2 cursor-pointer select-none">
              <input
                type="checkbox"
                checked={vulkanConfig.enablePipelineCachePrewarm}
                onChange={() => handleToggleOption('enablePipelineCachePrewarm')}
                className="rounded border-slate-300 dark:border-slate-700"
              />
              <span className="text-slate-700 dark:text-slate-300 font-medium">
                Pipeline Cache Pre-Warming
              </span>
            </label>
          </div>

          <div className="flex items-center gap-3 shrink-0">
            {saveMessage && (
              <span className="text-xs text-slate-600 dark:text-slate-400">{saveMessage}</span>
            )}
            <button
              type="button"
              disabled={isSaving}
              onClick={() => void onSaveVulkanConfig(vulkanConfig)}
              className="px-4 py-2 rounded-full bg-[#041e49] text-white dark:bg-[#d3e3fd] dark:text-[#041e49] text-xs font-semibold hover:opacity-90 transition-opacity disabled:opacity-50 whitespace-nowrap shrink-0 inline-flex items-center gap-2"
            >
              <Sliders className="w-3.5 h-3.5" />
              <span>{isSaving ? 'Applying Policy...' : 'Commit Vulkan Policy'}</span>
            </button>
          </div>
        </div>
      </div>

      {/* DevCheck-Aligned Capability Grid */}
      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
        {/* Column 1: Capabilities & Core Features */}
        <div className="rounded-[24px] bg-white dark:bg-[#161a22] border border-slate-900/[0.06] dark:border-white/[0.07] p-6 space-y-5">
          <div>
            <h3 className="text-sm font-bold text-slate-900 dark:text-slate-100 mb-2">
              Capabilities
            </h3>
            <div className="divide-y divide-slate-900/[0.05] dark:divide-white/[0.05]">
              <CapabilityRow
                label="Ray tracing"
                value={vk.rayTracing}
                note={badgeFor(vk.rayTracing, vk.hwRayTracing, true)}
              />
              <CapabilityRow
                label="Mesh shaders"
                value={vk.meshShaders}
                note={badgeFor(vk.meshShaders, vk.hwMeshShaders, true)}
              />
              <CapabilityRow
                label="Variable rate shading"
                value={vk.variableRateShading}
                note={badgeFor(vk.variableRateShading, vk.hwVariableRateShading)}
              />
              <CapabilityRow
                label="Cooperative matrix"
                value={vk.cooperativeMatrix}
                note={badgeFor(vk.cooperativeMatrix, vk.hwCooperativeMatrix, true)}
              />
              <CapabilityRow
                label="RT pipeline"
                value={vk.rtPipeline}
                note={badgeFor(vk.rtPipeline, vk.hwRtPipeline, true)}
              />
              <CapabilityRow
                label="Ray query"
                value={vk.rayQuery}
                note={badgeFor(vk.rayQuery, vk.hwRayQuery, true)}
              />
              <CapabilityRow
                label="Acceleration structure"
                value={vk.accelerationStructure}
                note={badgeFor(vk.accelerationStructure, vk.hwAccelerationStructure, true)}
              />
              <CapabilityRow label="Portability subset" value={vk.portabilitySubset} />
            </div>
          </div>

          <div className="pt-2 border-t border-slate-900/[0.06] dark:border-white/[0.06]">
            <h3 className="text-sm font-bold text-slate-900 dark:text-slate-100 mb-2">
              Core Features
            </h3>
            <div>
              <CapabilityRow label="Robust buffer access" value={vk.robustBufferAccess} />
              <CapabilityRow label="Sampler anisotropy" value={vk.samplerAnisotropy} />
              <CapabilityRow label="Shader Int16" value={vk.shaderInt16} />
              <CapabilityRow
                label="Shader Float64"
                value={vk.shaderFloat64}
                note={badgeFor(vk.shaderFloat64, vk.hwShaderFloat64, true)}
              />
            </div>
          </div>
        </div>

        {/* Column 2: Vulkan 1.1, 1.2, 1.3 & 1.4 */}
        <div className="rounded-[24px] bg-white dark:bg-[#161a22] border border-slate-900/[0.06] dark:border-white/[0.07] p-6 space-y-5">
          <div>
            <h3 className="text-sm font-bold text-slate-900 dark:text-slate-100 mb-2">
              Vulkan 1.1 · 1.2 · 1.3
            </h3>
            <div>
              <CapabilityRow label="Protected memory" value={vk.protectedMemory} note="1.1" />
              <CapabilityRow label="Timeline semaphore" value={vk.timelineSemaphore} note="1.2" />
              <CapabilityRow label="Descriptor indexing" value={vk.descriptorIndexing} note="1.2" />
              <CapabilityRow label="Buffer device address" value={vk.bufferDeviceAddress} note="1.2" />
              <CapabilityRow label="Dynamic rendering" value={vk.dynamicRendering} note="1.3" />
              <CapabilityRow label="Synchronization2" value={vk.synchronization2} note="1.3" />
            </div>
          </div>

          <div className="pt-2 border-t border-slate-900/[0.06] dark:border-white/[0.06]">
            <h3 className="text-sm font-bold text-slate-900 dark:text-slate-100 mb-2">
              Vulkan 1.4
            </h3>
            <div>
              <CapabilityRow
                label="Index type UINT8"
                value={vk.indexTypeUint8}
                note={badgeFor(vk.indexTypeUint8, vk.hwIndexTypeUint8)}
              />
              <CapabilityRow
                label="Line rasterization"
                value={vk.lineRasterization}
                note={badgeFor(vk.lineRasterization, vk.hwLineRasterization)}
              />
              <CapabilityRow
                label="Push descriptors"
                value={vk.pushDescriptors ? 'Yes (promoted)' : 'No'}
                note={badgeFor(vk.pushDescriptors, vk.hwPushDescriptors)}
              />
              <CapabilityRow
                label="Global priority query"
                value={vk.globalPriorityQuery}
                note={badgeFor(vk.globalPriorityQuery, vk.hwGlobalPriorityQuery)}
              />
              <CapabilityRow
                label="Shader float controls 2"
                value={vk.shaderFloatControls2}
                note={badgeFor(vk.shaderFloatControls2, vk.hwShaderFloatControls2)}
              />
              <CapabilityRow label="Max push descriptors" value={vk.maxPushDescriptors} />
            </div>
          </div>
        </div>

        {/* Column 3: Highlights, Universal GPU Surface & UUIDs */}
        <div className="rounded-[24px] bg-white dark:bg-[#161a22] border border-slate-900/[0.06] dark:border-white/[0.07] p-6 space-y-5">
          <div>
            <h3 className="text-sm font-bold text-slate-900 dark:text-slate-100 mb-2">
              Highlights
            </h3>
            <div>
              <CapabilityRow
                label="Maintenance5"
                value={vk.maintenance5}
                note={badgeFor(vk.maintenance5, vk.hwMaintenance5)}
              />
              <CapabilityRow
                label="Maintenance6"
                value={vk.maintenance6}
                note={badgeFor(vk.maintenance6, vk.hwMaintenance6)}
              />
              <CapabilityRow
                label="Maintenance7"
                value={vk.maintenance7}
                note={badgeFor(vk.maintenance7, vk.hwMaintenance7)}
              />
              <CapabilityRow
                label="Descriptor buffer"
                value={vk.descriptorBuffer}
                note={badgeFor(vk.descriptorBuffer, vk.hwDescriptorBuffer)}
              />
              <CapabilityRow
                label="Graphics pipeline library"
                value={vk.graphicsPipelineLibrary}
                note={badgeFor(vk.graphicsPipelineLibrary, vk.hwGraphicsPipelineLibrary)}
              />
            </div>
          </div>

          <div className="pt-2 border-t border-slate-900/[0.06] dark:border-white/[0.06]">
            <h3 className="text-sm font-bold text-slate-900 dark:text-slate-100 mb-2">
              Universal Multi-Vendor GPU Surface
            </h3>
            <div>
              <CapabilityRow label="Detected GPU" value={vk.deviceName} />
              <CapabilityRow label="Vendor Domain" value={vk.vendor} />
              <CapabilityRow label="Active Governor" value={vk.gpuGovernor} />
              <CapabilityRow
                label="Clock Window"
                value={`${vk.gpuCurrentFreqMhz} / ${vk.gpuMaxFreqMhz} MHz`}
              />
              <CapabilityRow label="Adrenoboost / Pacing" value={`Level ${vk.adrenoboostLevel}`} />
            </div>
          </div>

          <div className="pt-2 border-t border-slate-900/[0.06] dark:border-white/[0.06] space-y-2">
            <h3 className="text-sm font-bold text-slate-900 dark:text-slate-100">
              Hardware & Cache UUIDs
            </h3>
            <div className="space-y-1.5 text-[11px] font-mono tabular-nums text-slate-600 dark:text-slate-400">
              <div className="flex flex-col">
                <span className="text-slate-400 dark:text-slate-500">Device UUID</span>
                <span className="text-slate-800 dark:text-slate-200 break-all">{vk.deviceUuid}</span>
              </div>
              <div className="flex flex-col">
                <span className="text-slate-400 dark:text-slate-500">Driver UUID</span>
                <span className="text-slate-800 dark:text-slate-200 break-all">{vk.driverUuid}</span>
              </div>
              <div className="flex flex-col">
                <span className="text-slate-400 dark:text-slate-500">PipelineCache UUID</span>
                <span className="text-slate-800 dark:text-slate-200 break-all">
                  {vk.pipelineCacheUuid}
                </span>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
};
