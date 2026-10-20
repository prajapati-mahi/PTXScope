import React, { useState } from 'react';
import { Database, Search, Activity, AlertTriangle } from 'lucide-react';
import { FunctionAnalysis } from '../types';

interface RegisterAnalysisProps {
  analysis?: FunctionAnalysis[];
  onSelectLine?: (line: number) => void;
}

export const RegisterAnalysis: React.FC<RegisterAnalysisProps> = ({
  analysis,
  onSelectLine
}) => {
  const [tab, setTab] = useState<'REGISTERS' | 'LIVENESS'>('REGISTERS');
  const [search, setSearch] = useState('');

  if (!analysis || analysis.length === 0) {
    return (
      <div className="h-full flex flex-col items-center justify-center p-6 text-slate-500 gap-2 select-none">
        <Database className="w-10 h-10 text-slate-600" />
        <p className="text-sm font-medium">No Register Analysis Available</p>
        <p className="text-xs text-slate-600">Run static analysis to compute def-use chains and live ranges.</p>
      </div>
    );
  }

  const currentFn = analysis[0];
  const registers = (currentFn.registers || []).filter((r) =>
    r.name.toLowerCase().includes(search.toLowerCase())
  );
  const liveness = currentFn.liveness || [];

  return (
    <div className="h-full flex flex-col bg-slate-950 text-slate-200">
      {/* Top Header & Tabs */}
      <div className="p-3 border-b border-slate-800 flex items-center justify-between gap-4 flex-wrap">
        <div className="flex items-center gap-2">
          <div className="flex items-center gap-1 bg-slate-900 border border-slate-800 rounded-lg p-0.5 text-xs">
            <button
              onClick={() => setTab('REGISTERS')}
              className={`px-3 py-1 rounded-md font-medium transition ${
                tab === 'REGISTERS'
                  ? 'bg-slate-800 text-white shadow-sm'
                  : 'text-slate-400 hover:text-slate-200'
              }`}
            >
              Register Census ({currentFn.registers?.length || 0})
            </button>
            <button
              onClick={() => setTab('LIVENESS')}
              className={`px-3 py-1 rounded-md font-medium transition ${
                tab === 'LIVENESS'
                  ? 'bg-slate-800 text-white shadow-sm'
                  : 'text-slate-400 hover:text-slate-200'
              }`}
            >
              Block Liveness ({liveness.length})
            </button>
          </div>
        </div>

        {tab === 'REGISTERS' && (
          <div className="flex items-center gap-1.5 bg-slate-900 border border-slate-800 rounded-md px-2.5 py-1 text-xs">
            <Search className="w-3.5 h-3.5 text-slate-400" />
            <input
              type="text"
              placeholder="Filter registers..."
              value={search}
              onChange={(e) => setSearch(e.target.value)}
              className="bg-transparent text-slate-200 placeholder-slate-500 outline-none w-32"
            />
          </div>
        )}
      </div>

      {/* Content Area */}
      <div className="flex-1 overflow-auto p-3 min-h-0">
        {tab === 'REGISTERS' ? (
          <div className="overflow-x-auto border border-slate-800 rounded-lg">
            <table className="w-full text-left text-xs font-mono">
              <thead className="bg-slate-900 text-slate-400 border-b border-slate-800 select-none">
                <tr>
                  <th className="py-2.5 px-3">Register</th>
                  <th className="py-2.5 px-3">Status</th>
                  <th className="py-2.5 px-3 text-center">Defs</th>
                  <th className="py-2.5 px-3 text-center">Uses</th>
                  <th className="py-2.5 px-3">Def Lines</th>
                  <th className="py-2.5 px-3">Use Lines</th>
                </tr>
              </thead>
              <tbody className="divide-y divide-slate-800/80 bg-slate-950">
                {registers.map((r, idx) => {
                  const isDead = r.def_count > 0 && r.use_count === 0 && !r.is_builtin;

                  return (
                    <tr key={idx} className="hover:bg-slate-900/40 transition">
                      <td className="py-2 px-3 font-bold text-white flex items-center gap-1.5">
                        <span className="text-cyan-400">{r.name}</span>
                        {r.is_builtin && (
                          <span className="text-[10px] bg-slate-800 text-slate-400 px-1 py-0.5 rounded font-normal">
                            builtin
                          </span>
                        )}
                      </td>
                      <td className="py-2 px-3">
                        {isDead ? (
                          <span className="text-amber-400 flex items-center gap-1 text-[11px]">
                            <AlertTriangle className="w-3 h-3" /> Unused (Dead)
                          </span>
                        ) : (
                          <span className="text-emerald-400 flex items-center gap-1 text-[11px]">
                            <Activity className="w-3 h-3" /> Active
                          </span>
                        )}
                      </td>
                      <td className="py-2 px-3 text-center text-slate-300">{r.def_count}</td>
                      <td className="py-2 px-3 text-center text-slate-300">{r.use_count}</td>
                      <td className="py-2 px-3 text-slate-400">
                        {r.def_lines.map((ln, i) => (
                          <button
                            key={i}
                            onClick={() => onSelectLine && onSelectLine(ln)}
                            className="mr-1 hover:text-cyan-400 hover:underline cursor-pointer"
                          >
                            L{ln}
                          </button>
                        ))}
                      </td>
                      <td className="py-2 px-3 text-slate-400">
                        {r.use_lines.map((ln, i) => (
                          <button
                            key={i}
                            onClick={() => onSelectLine && onSelectLine(ln)}
                            className="mr-1 hover:text-cyan-400 hover:underline cursor-pointer"
                          >
                            L{ln}
                          </button>
                        ))}
                      </td>
                    </tr>
                  );
                })}
              </tbody>
            </table>
          </div>
        ) : (
          <div className="space-y-3">
            {liveness.map((bl, idx) => (
              <div key={idx} className="p-3 bg-slate-900/60 border border-slate-800 rounded-lg text-xs font-mono">
                <div className="font-bold text-white text-sm mb-2 flex items-center gap-2">
                  <span className="w-2 h-2 rounded-full bg-cyan-400"></span>
                  Block: {bl.block}
                </div>

                <div className="grid grid-cols-1 sm:grid-cols-2 gap-3 mt-2">
                  <div className="p-2.5 rounded bg-slate-950 border border-slate-800/80">
                    <span className="text-slate-400 text-[11px] font-semibold block mb-1">
                      Live-In Set:
                    </span>
                    <div className="flex flex-wrap gap-1">
                      {bl.live_in.length === 0 ? (
                        <span className="text-slate-600 text-[11px]">∅ (empty)</span>
                      ) : (
                        bl.live_in.map((r, i) => (
                          <span key={i} className="bg-slate-800 px-1.5 py-0.5 rounded text-cyan-300">
                            {r}
                          </span>
                        ))
                      )}
                    </div>
                  </div>

                  <div className="p-2.5 rounded bg-slate-950 border border-slate-800/80">
                    <span className="text-slate-400 text-[11px] font-semibold block mb-1">
                      Live-Out Set:
                    </span>
                    <div className="flex flex-wrap gap-1">
                      {bl.live_out.length === 0 ? (
                        <span className="text-slate-600 text-[11px]">∅ (empty)</span>
                      ) : (
                        bl.live_out.map((r, i) => (
                          <span key={i} className="bg-slate-800 px-1.5 py-0.5 rounded text-purple-300">
                            {r}
                          </span>
                        ))
                      )}
                    </div>
                  </div>
                </div>
              </div>
            ))}
          </div>
        )}
      </div>
    </div>
  );
};
