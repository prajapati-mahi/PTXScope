import React, { useState } from 'react';
import { AlertCircle, AlertTriangle, Lightbulb, CheckCircle2, Info, ArrowUpRight } from 'lucide-react';
import { DiagnosticItem } from '../types';

interface DiagnosticsPanelProps {
  diagnostics: DiagnosticItem[];
  onSelectLine: (line: number) => void;
}

export const DiagnosticsPanel: React.FC<DiagnosticsPanelProps> = ({
  diagnostics,
  onSelectLine
}) => {
  const [filter, setFilter] = useState<'ALL' | 'ERROR' | 'WARNING' | 'SUGGESTION'>('ALL');

  const filtered = diagnostics.filter((d) => {
    if (filter === 'ERROR') return d.severity === 'ERROR';
    if (filter === 'WARNING') return d.severity === 'WARNING';
    if (filter === 'SUGGESTION') return d.severity === 'OPTIMIZATION_SUGGESTION';
    return true;
  });

  return (
    <div className="h-full flex flex-col bg-slate-950 text-slate-200">
      {/* Header & Filter Tabs */}
      <div className="p-3 border-b border-slate-800 flex items-center justify-between gap-2 flex-wrap">
        <div className="flex items-center gap-1.5 text-xs">
          <button
            onClick={() => setFilter('ALL')}
            className={`px-2.5 py-1 rounded text-xs font-medium transition ${
              filter === 'ALL'
                ? 'bg-slate-800 text-white border border-slate-700'
                : 'text-slate-400 hover:text-slate-200'
            }`}
          >
            All ({diagnostics.length})
          </button>
          <button
            onClick={() => setFilter('ERROR')}
            className={`px-2.5 py-1 rounded text-xs font-medium transition ${
              filter === 'ERROR'
                ? 'bg-red-950/80 text-red-300 border border-red-800'
                : 'text-slate-400 hover:text-red-300'
            }`}
          >
            Errors ({diagnostics.filter((d) => d.severity === 'ERROR').length})
          </button>
          <button
            onClick={() => setFilter('WARNING')}
            className={`px-2.5 py-1 rounded text-xs font-medium transition ${
              filter === 'WARNING'
                ? 'bg-amber-950/80 text-amber-300 border border-amber-800'
                : 'text-slate-400 hover:text-amber-300'
            }`}
          >
            Warnings ({diagnostics.filter((d) => d.severity === 'WARNING').length})
          </button>
          <button
            onClick={() => setFilter('SUGGESTION')}
            className={`px-2.5 py-1 rounded text-xs font-medium transition ${
              filter === 'SUGGESTION'
                ? 'bg-cyan-950/80 text-cyan-300 border border-cyan-800'
                : 'text-slate-400 hover:text-cyan-300'
            }`}
          >
            Suggestions ({diagnostics.filter((d) => d.severity === 'OPTIMIZATION_SUGGESTION').length})
          </button>
        </div>
      </div>

      {/* Diagnostics List */}
      <div className="flex-1 overflow-y-auto p-3 space-y-2.5 min-h-0">
        {filtered.length === 0 ? (
          <div className="h-40 flex flex-col items-center justify-center text-slate-500 gap-2 select-none">
            <CheckCircle2 className="w-8 h-8 text-emerald-500/70" />
            <p className="text-sm font-medium">No diagnostics found matching filter.</p>
            <p className="text-xs text-slate-600">The compiler engine parsed instructions cleanly.</p>
          </div>
        ) : (
          filtered.map((d, idx) => {
            const isError = d.severity === 'ERROR';
            const isWarn = d.severity === 'WARNING';
            const isSugg = d.severity === 'OPTIMIZATION_SUGGESTION';

            return (
              <div
                key={idx}
                className={`p-3 rounded-lg border text-xs transition ${
                  isError
                    ? 'bg-red-950/20 border-red-900/60'
                    : isWarn
                    ? 'bg-amber-950/20 border-amber-900/60'
                    : 'bg-cyan-950/20 border-cyan-900/60'
                }`}
              >
                <div className="flex items-start justify-between gap-2">
                  <div className="flex items-center gap-2">
                    {isError && <AlertCircle className="w-4 h-4 text-red-400 shrink-0" />}
                    {isWarn && <AlertTriangle className="w-4 h-4 text-amber-400 shrink-0" />}
                    {isSugg && <Lightbulb className="w-4 h-4 text-cyan-400 shrink-0" />}
                    <span className="font-mono font-bold text-slate-200">{d.code}</span>
                  </div>

                  <button
                    onClick={() => onSelectLine(d.line)}
                    className="flex items-center gap-1 font-mono text-[11px] px-2 py-0.5 rounded bg-slate-900 border border-slate-700 hover:border-cyan-500 text-slate-300 hover:text-cyan-400 transition"
                    title="Jump to line in editor"
                  >
                    <span>L{d.line}:{d.column}</span>
                    <ArrowUpRight className="w-3 h-3" />
                  </button>
                </div>

                <p className="mt-2 text-slate-200 font-medium leading-relaxed">{d.message}</p>

                {d.explanation && (
                  <div className="mt-2 text-slate-400 bg-slate-900/80 p-2 rounded border border-slate-800/80">
                    <span className="text-slate-500 font-semibold uppercase text-[10px] block mb-0.5">
                      Compiler Rationale:
                    </span>
                    {d.explanation}
                  </div>
                )}

                {d.suggested_action && (
                  <div className="mt-1.5 text-cyan-300 bg-cyan-950/30 p-2 rounded border border-cyan-900/40">
                    <span className="text-cyan-500 font-semibold uppercase text-[10px] block mb-0.5">
                      Suggested Fix / Action:
                    </span>
                    {d.suggested_action}
                  </div>
                )}
              </div>
            );
          })
        )}
      </div>

      {/* Heuristic Footer Note */}
      <div className="p-2.5 bg-slate-900 border-t border-slate-800 text-[11px] text-slate-400 flex items-center gap-2">
        <Info className="w-4 h-4 text-slate-500 shrink-0" />
        <span>
          Static heuristics evaluate assembly patterns; runtime performance is determined by warp scheduling, occupancy, and cache hit rates on target GPU hardware.
        </span>
      </div>
    </div>
  );
};
