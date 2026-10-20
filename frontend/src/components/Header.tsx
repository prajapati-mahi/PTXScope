import React from 'react';
import { Cpu, Play, CheckCircle, AlertTriangle, ShieldCheck, RefreshCw, Zap } from 'lucide-react';
import { SampleItem, Capabilities } from '../types';

interface HeaderProps {
  samples: SampleItem[];
  currentSampleId: string;
  onSelectSample: (id: string) => void;
  onRunAnalyze: () => void;
  onRunOptimize: () => void;
  onRunValidate: () => void;
  onReset: () => void;
  loading: boolean;
  capabilities: Capabilities | null;
  ptxasAvailable: boolean;
}

export const Header: React.FC<HeaderProps> = ({
  samples,
  currentSampleId,
  onSelectSample,
  onRunAnalyze,
  onRunOptimize,
  onRunValidate,
  onReset,
  loading,
  capabilities,
  ptxasAvailable
}) => {
  return (
    <header className="bg-slate-900 border-b border-slate-800 px-4 py-2.5 flex flex-wrap items-center justify-between gap-4 select-none">
      {/* Brand & Subtitle */}
      <div className="flex items-center gap-3">
        <div className="w-9 h-9 rounded-lg bg-gradient-to-br from-cyan-500 to-blue-600 flex items-center justify-center shadow-lg shadow-cyan-500/20">
          <Cpu className="w-5 h-5 text-white" />
        </div>
        <div>
          <div className="flex items-center gap-2">
            <h1 className="font-bold text-base tracking-tight text-white flex items-center gap-1.5">
              PTXScope
              <span className="text-[10px] font-mono px-1.5 py-0.5 rounded bg-cyan-950/80 border border-cyan-800/60 text-cyan-400">
                {capabilities?.version ? `v${capabilities.version}` : 'v1.0.0'}
              </span>
            </h1>
          </div>
          <p className="text-xs text-slate-400 font-medium">GPU Assembly Analyzer, Optimizer & Debugging Workbench</p>
        </div>
      </div>

      {/* Sample Selector & Action Buttons */}
      <div className="flex items-center gap-2.5 flex-wrap">
        <div className="flex items-center gap-1.5 bg-slate-950 border border-slate-800 rounded-md px-2.5 py-1 text-xs">
          <span className="text-slate-400 font-medium">Sample:</span>
          <select
            value={currentSampleId}
            onChange={(e) => onSelectSample(e.target.value)}
            className="bg-transparent text-slate-200 font-medium outline-none cursor-pointer pr-2"
          >
            <option value="" disabled>Select sample program...</option>
            {samples.map((s) => (
              <option key={s.id} value={s.id} className="bg-slate-900 text-slate-200">
                {s.title}
              </option>
            ))}
          </select>
        </div>

        <button
          onClick={onRunAnalyze}
          disabled={loading}
          className="flex items-center gap-1.5 bg-slate-800 hover:bg-slate-700 text-slate-200 border border-slate-700 px-3 py-1.5 rounded-md text-xs font-semibold transition disabled:opacity-50"
        >
          <Play className="w-3.5 h-3.5 text-cyan-400" />
          Analyze
        </button>

        <button
          onClick={onRunOptimize}
          disabled={loading}
          className="flex items-center gap-1.5 bg-gradient-to-r from-cyan-600 to-blue-600 hover:from-cyan-500 hover:to-blue-500 text-white px-3.5 py-1.5 rounded-md text-xs font-semibold shadow-md shadow-cyan-600/20 transition disabled:opacity-50"
        >
          <Zap className="w-3.5 h-3.5 text-yellow-300" />
          Optimize
        </button>

        <button
          onClick={onRunValidate}
          disabled={loading}
          className="flex items-center gap-1.5 bg-slate-800 hover:bg-slate-700 text-slate-200 border border-slate-700 px-3 py-1.5 rounded-md text-xs font-semibold transition disabled:opacity-50"
          title={ptxasAvailable ? "Validate using local NVIDIA ptxas" : "ptxas not found on PATH"}
        >
          <ShieldCheck className="w-3.5 h-3.5 text-emerald-400" />
          Validate
        </button>

        <button
          onClick={onReset}
          disabled={loading}
          className="p-1.5 text-slate-400 hover:text-slate-200 rounded-md hover:bg-slate-800 transition"
          title="Reset to original sample"
        >
          <RefreshCw className="w-3.5 h-3.5" />
        </button>
      </div>

      {/* Toolchain Status Badges */}
      <div className="flex items-center gap-2 text-xs font-mono">
        <div className="flex items-center gap-1.5 px-2.5 py-1 rounded bg-slate-950 border border-slate-800 text-emerald-400">
          <CheckCircle className="w-3 h-3 text-emerald-400" />
          <span>C++ Engine: Active</span>
        </div>

        <div className={`flex items-center gap-1.5 px-2.5 py-1 rounded border ${
          ptxasAvailable
            ? 'bg-slate-950 border-emerald-900/60 text-emerald-400'
            : 'bg-slate-950 border-amber-900/60 text-amber-400'
        }`}>
          {ptxasAvailable ? (
            <>
              <CheckCircle className="w-3 h-3" />
              <span>ptxas: Ready</span>
            </>
          ) : (
            <>
              <AlertTriangle className="w-3 h-3" />
              <span>ptxas: Optional</span>
            </>
          )}
        </div>
      </div>
    </header>
  );
};
