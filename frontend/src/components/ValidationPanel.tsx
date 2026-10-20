import React from 'react';
import { ShieldCheck, Terminal, AlertTriangle, CheckCircle, Info, Play } from 'lucide-react';
import { PtxasValidationResult } from '../types';

interface ValidationPanelProps {
  validation?: PtxasValidationResult;
  onValidate: (arch: string) => void;
  loading: boolean;
  selectedArch: string;
  onSelectArch: (arch: string) => void;
}

export const ValidationPanel: React.FC<ValidationPanelProps> = ({
  validation,
  onValidate,
  loading,
  selectedArch,
  onSelectArch
}) => {
  const architectures = ['sm_70', 'sm_75', 'sm_80', 'sm_86', 'sm_89', 'sm_90'];

  return (
    <div className="h-full flex flex-col bg-slate-950 text-slate-200">
      {/* Top Toolbar */}
      <div className="p-3 border-b border-slate-800 flex items-center justify-between gap-4 flex-wrap">
        <div className="flex items-center gap-3">
          <ShieldCheck className="w-4 h-4 text-emerald-400" />
          <span className="text-xs font-mono font-bold text-slate-200">
            NVIDIA ptxas Toolchain Validation
          </span>

          <div className="flex items-center gap-1.5 text-xs">
            <span className="text-slate-400">Target SM:</span>
            <select
              value={selectedArch}
              onChange={(e) => onSelectArch(e.target.value)}
              className="bg-slate-900 border border-slate-700 text-slate-200 rounded px-2 py-0.5 font-mono text-xs outline-none"
            >
              {architectures.map((arch) => (
                <option key={arch} value={arch}>{arch}</option>
              ))}
            </select>
          </div>
        </div>

        <button
          onClick={() => onValidate(selectedArch)}
          disabled={loading}
          className="flex items-center gap-1.5 bg-emerald-600 hover:bg-emerald-500 text-white px-3 py-1.5 rounded-md text-xs font-semibold shadow-md shadow-emerald-600/20 transition disabled:opacity-50"
        >
          <Play className="w-3.5 h-3.5" />
          Run ptxas Validation
        </button>
      </div>

      {/* Main Content */}
      <div className="flex-1 overflow-y-auto p-4 space-y-4 min-h-0">
        {/* Status Card */}
        <div className={`p-4 rounded-xl border text-xs ${
          !validation || !validation.available
            ? 'bg-amber-950/20 border-amber-900/60'
            : validation.compilation_success
            ? 'bg-emerald-950/20 border-emerald-900/60'
            : 'bg-red-950/20 border-red-900/60'
        }`}>
          <div className="flex items-center justify-between">
            <div className="flex items-center gap-2">
              {!validation || !validation.available ? (
                <AlertTriangle className="w-5 h-5 text-amber-400 shrink-0" />
              ) : validation.compilation_success ? (
                <CheckCircle className="w-5 h-5 text-emerald-400 shrink-0" />
              ) : (
                <AlertTriangle className="w-5 h-5 text-red-400 shrink-0" />
              )}
              <div>
                <div className="font-bold text-sm text-white">
                  {!validation || !validation.available
                    ? 'NVIDIA ptxas Not Detected on Host'
                    : validation.compilation_success
                    ? 'Hardware Assembler Validation: PASSED'
                    : 'Compilation Failed with Errors'}
                </div>
                <div className="text-slate-400 mt-0.5">
                  {validation?.status_message ||
                    'Install the NVIDIA CUDA Toolkit to enable physical GPU binary (.cubin) assembly validation.'}
                </div>
              </div>
            </div>

            {validation?.available && (
              <div className="text-right font-mono text-xs">
                <span className="text-slate-400 block">Exit Code: {validation.exit_code}</span>
                <span className="text-slate-400 block">Duration: {validation.duration_ms.toFixed(1)} ms</span>
              </div>
            )}
          </div>
        </div>

        {/* Compiler Output Console */}
        <div className="rounded-xl border border-slate-800 bg-[#090d16] flex flex-col overflow-hidden">
          <div className="px-3 py-2 bg-slate-900/80 border-b border-slate-800 flex items-center justify-between text-xs font-mono text-slate-400">
            <span className="flex items-center gap-1.5 text-slate-300">
              <Terminal className="w-3.5 h-3.5 text-cyan-400" />
              ptxas Standard Output & Diagnostics
            </span>
            <span>Target: {selectedArch}</span>
          </div>

          <div className="p-3 font-mono text-xs overflow-auto max-h-64 leading-relaxed whitespace-pre">
            {validation?.stdout && (
              <div className="text-emerald-300">{validation.stdout}</div>
            )}
            {validation?.stderr && (
              <div className="text-red-400">{validation.stderr}</div>
            )}
            {!validation?.stdout && !validation?.stderr && (
              <div className="text-slate-600 italic">
                {validation?.available
                  ? 'Compiler executed silently with 0 warnings.'
                  : '// No external compiler output yet. Click "Run ptxas Validation" to execute.'}
              </div>
            )}
          </div>
        </div>

        {/* PTX vs SASS Technical Architecture Notice */}
        <div className="p-4 rounded-xl border border-slate-800/80 bg-slate-900/40 text-xs text-slate-300 space-y-2">
          <div className="flex items-center gap-2 font-semibold text-white">
            <Info className="w-4 h-4 text-cyan-400" />
            <span>Understanding PTX vs SASS & Static Optimization Heuristics</span>
          </div>
          <p className="text-slate-400 leading-relaxed">
            <strong className="text-slate-200">PTX (Parallel Thread Execution)</strong> is a virtual, architecture-independent instruction set representation. NVIDIA’s <code className="bg-slate-800 px-1 py-0.5 rounded text-cyan-300">ptxas</code> tool consumes PTX and generates <strong className="text-slate-200">SASS (Streaming Assembler)</strong> machine instructions tailored to specific hardware architectures (e.g., Ada Lovelace, Hopper, Blackwell).
          </p>
          <p className="text-slate-400 leading-relaxed">
            PTXScope performs rigorous ahead-of-time static analysis and intermediate representation transformations. However, actual kernel performance on physical GPU hardware is influenced by warp divergence, register spilling, L1/L2 cache hit ratios, and memory coalescing, which cannot be modeled solely from static assembly without real profiling runs.
          </p>
        </div>
      </div>
    </div>
  );
};
