import React, { useState } from 'react';
import { Copy, Check, Zap, CheckCircle2, AlertCircle } from 'lucide-react';
import { PassResultItem, Metrics } from '../types';

interface OptimizationDiffProps {
  originalPtx: string;
  optimizedPtx: string;
  passResults: PassResultItem[];
  metrics?: Metrics;
}

export const OptimizationDiff: React.FC<OptimizationDiffProps> = ({
  originalPtx,
  optimizedPtx,
  passResults,
  metrics
}) => {
  const [copied, setCopied] = useState(false);

  const handleCopy = () => {
    navigator.clipboard.writeText(optimizedPtx || originalPtx);
    setCopied(true);
    setTimeout(() => setCopied(false), 2000);
  };

  const totalTransformations = passResults.reduce((acc, p) => acc + p.transformations, 0);

  return (
    <div className="h-full flex flex-col bg-slate-950 text-slate-200">
      {/* Top Action Bar */}
      <div className="p-3 border-b border-slate-800 flex items-center justify-between gap-4 flex-wrap">
        <div className="flex items-center gap-2">
          <Zap className="w-4 h-4 text-yellow-400" />
          <span className="text-xs font-mono font-bold text-white">
            Transformations Applied: {totalTransformations}
          </span>
          {metrics && (
            <span className="text-xs text-slate-400 font-mono">
              ({metrics.initial_instruction_count} → {metrics.optimized_instruction_count} instructions)
            </span>
          )}
        </div>

        <button
          onClick={handleCopy}
          className="flex items-center gap-1.5 px-3 py-1 rounded bg-slate-900 border border-slate-700 hover:border-slate-500 text-xs text-slate-200 transition"
        >
          {copied ? <Check className="w-3.5 h-3.5 text-emerald-400" /> : <Copy className="w-3.5 h-3.5" />}
          <span>{copied ? 'Copied to Clipboard' : 'Copy Optimized PTX'}</span>
        </button>
      </div>

      <div className="flex-1 flex flex-col md:flex-row min-h-0 divide-y md:divide-y-0 md:divide-x divide-slate-800">
        {/* Left: Pass Execution Log */}
        <div className="w-full md:w-1/2 p-3 overflow-y-auto space-y-3">
          <div className="text-xs font-semibold text-slate-400 uppercase tracking-wider mb-2">
            Optimization Passes & Safety Audit
          </div>

          {passResults.length === 0 ? (
            <div className="p-4 rounded border border-slate-800 bg-slate-900/40 text-xs text-slate-400 text-center">
              Run optimization to inspect pass transformations.
            </div>
          ) : (
            passResults.map((pr, idx) => (
              <div
                key={idx}
                className={`p-3 rounded-lg border text-xs ${
                  pr.transformations > 0
                    ? 'bg-emerald-950/20 border-emerald-900/60'
                    : 'bg-slate-900/40 border-slate-800/80'
                }`}
              >
                <div className="flex items-center justify-between">
                  <div className="flex items-center gap-2">
                    {pr.transformations > 0 ? (
                      <CheckCircle2 className="w-4 h-4 text-emerald-400" />
                    ) : (
                      <span className="w-4 h-4 rounded-full border border-slate-700 flex items-center justify-center text-[10px] text-slate-500">
                        0
                      </span>
                    )}
                    <span className="font-mono font-bold text-slate-200">
                      Pass [{pr.pass_name}]
                    </span>
                  </div>

                  <span className={`px-2 py-0.5 rounded text-[11px] font-mono font-semibold ${
                    pr.transformations > 0
                      ? 'bg-emerald-900/40 text-emerald-300 border border-emerald-800'
                      : 'bg-slate-800 text-slate-400'
                  }`}>
                    {pr.transformations} applied
                  </span>
                </div>

                {/* Explanations */}
                {pr.explanations.length > 0 && (
                  <div className="mt-2 space-y-1">
                    {pr.explanations.map((exp, i) => (
                      <div key={i} className="text-emerald-300 bg-emerald-950/40 p-1.5 rounded font-mono text-[11px] border border-emerald-900/30">
                        {exp}
                      </div>
                    ))}
                  </div>
                )}

                {/* Skipped safety checks */}
                {pr.skipped_reasons.length > 0 && (
                  <div className="mt-2 space-y-1">
                    {pr.skipped_reasons.map((skip, i) => (
                      <div key={i} className="text-amber-300 bg-amber-950/30 p-1.5 rounded font-mono text-[11px] border border-amber-900/30 flex items-start gap-1.5">
                        <AlertCircle className="w-3.5 h-3.5 text-amber-400 shrink-0 mt-0.5" />
                        <span>{skip}</span>
                      </div>
                    ))}
                  </div>
                )}
              </div>
            ))
          )}
        </div>

        {/* Right: Optimized Assembly Code Preview */}
        <div className="w-full md:w-1/2 flex flex-col min-h-0 bg-[#090d16]">
          <div className="px-3 py-2 bg-slate-900/80 border-b border-slate-800 text-xs font-mono text-slate-400 flex items-center justify-between">
            <span>Optimized PTX Output</span>
            <span className="text-[11px] text-cyan-400">Read-Only</span>
          </div>
          <pre className="flex-1 p-3 overflow-auto font-mono text-xs text-slate-200 leading-relaxed m-0 whitespace-pre">
            {optimizedPtx || originalPtx || '// No code'}
          </pre>
        </div>
      </div>
    </div>
  );
};
