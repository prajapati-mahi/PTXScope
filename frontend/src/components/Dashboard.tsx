import React from 'react';
import { Layers, Activity, Clock, AlertCircle, AlertTriangle, Lightbulb } from 'lucide-react';
import { Metrics, DiagnosticItem } from '../types';

interface DashboardProps {
  metrics?: Metrics;
  diagnostics: DiagnosticItem[];
}

export const Dashboard: React.FC<DashboardProps> = ({ metrics, diagnostics }) => {
  const errCount = diagnostics.filter((d) => d.severity === 'ERROR').length;
  const warnCount = diagnostics.filter((d) => d.severity === 'WARNING').length;
  const suggCount = diagnostics.filter((d) => d.severity === 'OPTIMIZATION_SUGGESTION').length;

  const initCount = metrics?.initial_instruction_count ?? 0;
  const optCount = metrics?.optimized_instruction_count ?? 0;
  const delta = initCount > 0 ? ((optCount - initCount) / initCount) * 100 : 0;

  return (
    <div className="grid grid-cols-2 sm:grid-cols-4 gap-2.5 p-3 bg-slate-900 border-b border-slate-800">
      {/* Metric 1: Instructions & Delta */}
      <div className="bg-slate-950/70 border border-slate-800 rounded-lg p-2.5 flex flex-col justify-between">
        <div className="flex items-center justify-between text-xs text-slate-400">
          <span className="font-medium">Instructions</span>
          <Activity className="w-3.5 h-3.5 text-cyan-400" />
        </div>
        <div className="mt-1 flex items-baseline gap-2">
          <span className="text-xl font-bold font-mono text-white">{initCount}</span>
          {metrics && optCount !== initCount && (
            <span className="text-xs font-mono text-emerald-400">
              → {optCount} ({delta.toFixed(0)}%)
            </span>
          )}
        </div>
        <div className="text-[10px] text-slate-500 mt-0.5">
          {optCount < initCount ? `${initCount - optCount} eliminated` : 'Baseline count'}
        </div>
      </div>

      {/* Metric 2: Basic Blocks */}
      <div className="bg-slate-950/70 border border-slate-800 rounded-lg p-2.5 flex flex-col justify-between">
        <div className="flex items-center justify-between text-xs text-slate-400">
          <span className="font-medium">Basic Blocks</span>
          <Layers className="w-3.5 h-3.5 text-purple-400" />
        </div>
        <div className="mt-1 flex items-baseline gap-2">
          <span className="text-xl font-bold font-mono text-white">
            {metrics?.basic_block_count ?? 0}
          </span>
          <span className="text-[11px] text-slate-400 font-mono">
            in {metrics?.function_count ?? 1} func
          </span>
        </div>
        <div className="text-[10px] text-slate-500 mt-0.5">CFG graph nodes</div>
      </div>

      {/* Metric 3: Execution Duration */}
      <div className="bg-slate-950/70 border border-slate-800 rounded-lg p-2.5 flex flex-col justify-between">
        <div className="flex items-center justify-between text-xs text-slate-400">
          <span className="font-medium">Engine Latency</span>
          <Clock className="w-3.5 h-3.5 text-blue-400" />
        </div>
        <div className="mt-1 flex items-baseline gap-1.5">
          <span className="text-xl font-bold font-mono text-white">
            {metrics ? metrics.execution_duration_ms.toFixed(1) : '0.0'}
          </span>
          <span className="text-xs text-slate-400 font-mono">ms</span>
        </div>
        <div className="text-[10px] text-slate-500 mt-0.5">C++ parser & passes</div>
      </div>

      {/* Metric 4: Diagnostics */}
      <div className="bg-slate-950/70 border border-slate-800 rounded-lg p-2.5 flex flex-col justify-between">
        <div className="flex items-center justify-between text-xs text-slate-400">
          <span className="font-medium">Diagnostics</span>
          <span className="text-xs font-mono font-bold text-slate-300">
            {diagnostics.length}
          </span>
        </div>
        <div className="mt-1 flex items-center gap-2 text-xs font-mono">
          <span className="flex items-center gap-0.5 text-red-400" title="Errors">
            <AlertCircle className="w-3 h-3" /> {errCount}
          </span>
          <span className="flex items-center gap-0.5 text-amber-400" title="Warnings">
            <AlertTriangle className="w-3 h-3" /> {warnCount}
          </span>
          <span className="flex items-center gap-0.5 text-cyan-400" title="Optimization Suggestions">
            <Lightbulb className="w-3 h-3" /> {suggCount}
          </span>
        </div>
        <div className="text-[10px] text-slate-500 mt-0.5">Rule-based findings</div>
      </div>
    </div>
  );
};
