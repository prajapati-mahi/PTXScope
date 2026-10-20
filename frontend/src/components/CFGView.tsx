import React, { useState } from 'react';
import { GitBranch, ArrowRight, CornerDownRight, AlertOctagon, Copy, Check } from 'lucide-react';
import { CFGData } from '../types';

interface CFGViewProps {
  cfg?: CFGData;
  onSelectBlock?: (label: string) => void;
}

export const CFGView: React.FC<CFGViewProps> = ({ cfg, onSelectBlock }) => {
  const [copiedDot, setCopiedDot] = useState(false);

  if (!cfg || !cfg.nodes || cfg.nodes.length === 0) {
    return (
      <div className="h-full flex flex-col items-center justify-center p-6 text-slate-500 gap-2 select-none">
        <GitBranch className="w-10 h-10 text-slate-600" />
        <p className="text-sm font-medium">No Control Flow Graph Available</p>
        <p className="text-xs text-slate-600">Analyze or compile a function to generate basic blocks and branch edges.</p>
      </div>
    );
  }

  const nodes = cfg.nodes || [];
  const edges = cfg.edges || [];

  const handleCopyDot = () => {
    // Generate simple dot string
    let dot = `digraph "${cfg.function || 'PTX_CFG'}" {\n`;
    for (const n of nodes) {
      dot += `  "${n.label}";\n`;
    }
    for (const e of edges) {
      dot += `  "${e.source}" -> "${e.target}" [label="${e.condition || e.type}"];\n`;
    }
    dot += '}';
    navigator.clipboard.writeText(dot);
    setCopiedDot(true);
    setTimeout(() => setCopiedDot(false), 2000);
  };

  return (
    <div className="h-full flex flex-col bg-slate-950 text-slate-200">
      {/* Header toolbar */}
      <div className="p-3 border-b border-slate-800 flex items-center justify-between gap-4">
        <div className="flex items-center gap-2">
          <GitBranch className="w-4 h-4 text-cyan-400" />
          <span className="text-xs font-mono font-bold text-slate-200">
            Function: {cfg.function || 'main'}
          </span>
          <span className="text-xs text-slate-400">
            ({nodes.length} blocks, {edges.length} edges)
          </span>
        </div>

        <button
          onClick={handleCopyDot}
          className="flex items-center gap-1.5 px-2.5 py-1 rounded bg-slate-900 border border-slate-700 hover:border-slate-500 text-xs text-slate-300 transition"
        >
          {copiedDot ? <Check className="w-3.5 h-3.5 text-emerald-400" /> : <Copy className="w-3.5 h-3.5" />}
          <span>{copiedDot ? 'Copied DOT' : 'Export DOT'}</span>
        </button>
      </div>

      {/* Basic Blocks Flow Grid */}
      <div className="flex-1 overflow-y-auto p-4 space-y-4 min-h-0">
        {nodes.map((node) => {
          const outEdges = edges.filter((e) => e.source === node.id);
          const inEdges = edges.filter((e) => e.target === node.id);

          return (
            <div
              key={node.id}
              onClick={() => onSelectBlock && onSelectBlock(node.label)}
              className={`rounded-xl border transition cursor-pointer hover:shadow-lg ${
                node.is_unreachable
                  ? 'bg-red-950/20 border-red-800/80 hover:border-red-600'
                  : node.is_entry
                  ? 'bg-slate-900/90 border-cyan-500/80 shadow-cyan-950/30'
                  : 'bg-slate-900/60 border-slate-800 hover:border-slate-700'
              }`}
            >
              {/* Block Header */}
              <div className="px-3.5 py-2 border-b border-slate-800/80 flex items-center justify-between text-xs font-mono">
                <div className="flex items-center gap-2">
                  <span className={`w-2 h-2 rounded-full ${
                    node.is_unreachable ? 'bg-red-500' : node.is_entry ? 'bg-cyan-400' : 'bg-slate-500'
                  }`} />
                  <span className="font-bold text-white tracking-wide">
                    {node.label}
                  </span>
                  {node.is_entry && (
                    <span className="text-[10px] bg-cyan-950 text-cyan-300 border border-cyan-800 px-1.5 py-0.2 rounded">
                      ENTRY
                    </span>
                  )}
                  {node.is_unreachable && (
                    <span className="text-[10px] bg-red-950 text-red-300 border border-red-800 px-1.5 py-0.2 rounded flex items-center gap-1">
                      <AlertOctagon className="w-3 h-3" /> UNREACHABLE
                    </span>
                  )}
                </div>

                <span className="text-slate-400 text-[11px]">
                  {node.instruction_count} instructions
                </span>
              </div>

              {/* Instructions list */}
              <div className="p-3 bg-slate-950/60 font-mono text-xs text-slate-300 space-y-1 overflow-x-auto">
                {node.instructions.map((inst, i) => (
                  <div key={i} className="hover:bg-slate-800/50 px-1.5 py-0.5 rounded flex items-center gap-2">
                    <span className="text-slate-600 select-none text-[10px] w-4 text-right">{i + 1}</span>
                    <span className="text-slate-200">{inst}</span>
                  </div>
                ))}
              </div>

              {/* Successor & Predecessor Edge Tags */}
              <div className="px-3 py-2 bg-slate-900/40 border-t border-slate-800/60 flex flex-wrap items-center justify-between gap-2 text-xs font-mono">
                {/* Inflow */}
                <div className="flex items-center gap-1.5 text-slate-400 text-[11px]">
                  <CornerDownRight className="w-3 h-3 text-slate-500" />
                  <span>Preds:</span>
                  {inEdges.length === 0 ? (
                    <span className="text-slate-600">none</span>
                  ) : (
                    inEdges.map((e, idx) => (
                      <span key={idx} className="bg-slate-800 px-1.5 py-0.5 rounded text-slate-300">
                        {e.source}
                      </span>
                    ))
                  )}
                </div>

                {/* Outflow */}
                <div className="flex items-center gap-1.5 text-[11px]">
                  <ArrowRight className="w-3 h-3 text-cyan-400" />
                  <span className="text-slate-400">Succs:</span>
                  {outEdges.length === 0 ? (
                    <span className="text-slate-600 font-mono">none (terminal)</span>
                  ) : (
                    outEdges.map((e, idx) => (
                      <span
                        key={idx}
                        className={`px-1.5 py-0.5 rounded border text-[11px] ${
                          e.type === 'branch_true'
                            ? 'bg-emerald-950/80 border-emerald-800 text-emerald-300'
                            : e.type === 'fallthrough'
                            ? 'bg-slate-800 border-slate-700 text-slate-300'
                            : 'bg-blue-950/80 border-blue-800 text-blue-300'
                        }`}
                      >
                        {e.target} {e.condition ? `[${e.condition}]` : ''}
                      </span>
                    ))
                  )}
                </div>
              </div>
            </div>
          );
        })}
      </div>
    </div>
  );
};
