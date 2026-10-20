import React, { useState, useEffect, useCallback } from 'react';
import { Header } from './components/Header';
import { Dashboard } from './components/Dashboard';
import { Editor } from './components/Editor';
import { DiagnosticsPanel } from './components/DiagnosticsPanel';
import { CFGView } from './components/CFGView';
import { OptimizationDiff } from './components/OptimizationDiff';
import { RegisterAnalysis } from './components/RegisterAnalysis';
import { ValidationPanel } from './components/ValidationPanel';
import {
  fetchCapabilities,
  fetchSamples,
  fetchSampleSource,
  runWorkbench,
  validatePtx
} from './api';
import {
  WorkbenchResponse,
  SampleItem,
  Capabilities,
  PtxasValidationResult
} from './types';
import { GitBranch, Zap, AlertTriangle, Database, ShieldCheck } from 'lucide-react';

export const App: React.FC = () => {
  const [samples, setSamples] = useState<SampleItem[]>([]);
  const [currentSampleId, setCurrentSampleId] = useState<string>('');
  const [ptxSource, setPtxSource] = useState<string>('');
  const [capabilities, setCapabilities] = useState<Capabilities | null>(null);
  const [workbenchData, setWorkbenchData] = useState<WorkbenchResponse | null>(null);
  const [validationData, setValidationData] = useState<PtxasValidationResult | undefined>(undefined);
  const [selectedArch, setSelectedArch] = useState<string>('sm_70');
  const [activeTab, setActiveTab] = useState<'CFG' | 'OPTIMIZE' | 'DIAGNOSTICS' | 'REGISTERS' | 'VALIDATION'>('CFG');
  const [jumpLine, setJumpLine] = useState<number | null>(null);
  const [loading, setLoading] = useState<boolean>(false);
  const [errorMessage, setErrorMessage] = useState<string | null>(null);

  // Initialize samples and capabilities
  useEffect(() => {
    async function init() {
      try {
        const [caps, sampleList] = await Promise.all([
          fetchCapabilities().catch(() => null),
          fetchSamples().catch(() => [])
        ]);
        setCapabilities(caps);
        setSamples(sampleList);

        if (sampleList.length > 0) {
          const first = sampleList[0];
          setCurrentSampleId(first.id);
          const sampleData = await fetchSampleSource(first.id);
          setPtxSource(sampleData.source);
          executeWorkbench(sampleData.source, []);
        }
      } catch (err: any) {
        console.error('Init error:', err);
        setErrorMessage(err.message || 'Failed to initialize PTXScope backend connection.');
      }
    }
    init();
  }, []);

  const executeWorkbench = useCallback(async (source: string, passes?: string[]) => {
    setLoading(true);
    setErrorMessage(null);
    try {
      const data = await runWorkbench(source, passes, selectedArch);
      setWorkbenchData(data);
      if (data.ptxas_validation) {
        setValidationData(data.ptxas_validation);
      }
    } catch (err: any) {
      setErrorMessage(err.message || 'Execution error');
    } finally {
      setLoading(false);
    }
  }, [selectedArch]);

  const handleSelectSample = async (id: string) => {
    setCurrentSampleId(id);
    try {
      const sample = await fetchSampleSource(id);
      setPtxSource(sample.source);
      executeWorkbench(sample.source, []);
    } catch (err: any) {
      setErrorMessage(err.message || 'Failed to load sample');
    }
  };

  const handleRunAnalyze = () => {
    executeWorkbench(ptxSource, []);
  };

  const handleRunOptimize = async () => {
    await executeWorkbench(ptxSource, ['const-fold', 'algebraic-simp', 'redundant-mov', 'dce']);
    setActiveTab('OPTIMIZE');
  };

  const handleRunValidate = async (arch?: string) => {
    const target = arch || selectedArch;
    setLoading(true);
    try {
      const res = await validatePtx(ptxSource, target);
      setValidationData(res);
      setActiveTab('VALIDATION');
    } catch (err: any) {
      setErrorMessage(err.message || 'ptxas validation error');
    } finally {
      setLoading(false);
    }
  };

  const handleReset = async () => {
    if (currentSampleId) {
      handleSelectSample(currentSampleId);
    }
  };

  const handleSelectLine = (line: number) => {
    setJumpLine(line);
  };

  const handleSelectBlock = (label: string) => {
    // Search for label line in ptxSource
    const lines = ptxSource.split('\n');
    for (let i = 0; i < lines.length; ++i) {
      if (lines[i].trim().startsWith(label + ':') || lines[i].includes(label)) {
        setJumpLine(i + 1);
        break;
      }
    }
  };

  const diagnostics = workbenchData?.diagnostics || [];
  const cfg = workbenchData?.cfg;
  const analysis = workbenchData?.analysis;
  const passResults = workbenchData?.pass_results || [];
  const metrics = workbenchData?.metrics;

  return (
    <div className="flex flex-col h-screen w-screen overflow-hidden bg-slate-950 font-sans">
      {/* Top Header */}
      <Header
        samples={samples}
        currentSampleId={currentSampleId}
        onSelectSample={handleSelectSample}
        onRunAnalyze={handleRunAnalyze}
        onRunOptimize={handleRunOptimize}
        onRunValidate={() => handleRunValidate()}
        onReset={handleReset}
        loading={loading}
        capabilities={capabilities}
        ptxasAvailable={capabilities?.ptxas_available ?? false}
      />

      {/* Compiler Dashboard */}
      <Dashboard metrics={metrics} diagnostics={diagnostics} />

      {/* Error alert if any */}
      {errorMessage && (
        <div className="bg-red-950/80 border-b border-red-800 text-red-200 px-4 py-2 text-xs flex items-center justify-between">
          <span className="font-mono">Error: {errorMessage}</span>
          <button
            onClick={() => setErrorMessage(null)}
            className="text-red-400 hover:text-white font-bold ml-4"
          >
            ✕
          </button>
        </div>
      )}

      {/* Main Split Workbench */}
      <div className="flex-1 flex flex-col md:flex-row min-h-0 divide-y md:divide-y-0 md:divide-x divide-slate-800">
        {/* Left Column: Monaco Editor (50%) */}
        <div className="w-full md:w-1/2 h-1/2 md:h-full flex flex-col min-h-0">
          <Editor
            value={ptxSource}
            onChange={(val) => setPtxSource(val)}
            diagnostics={diagnostics}
            jumpLine={jumpLine}
          />
        </div>

        {/* Right Column: Interactive Compiler Views (50%) */}
        <div className="w-full md:w-1/2 h-1/2 md:h-full flex flex-col min-h-0 bg-slate-950">
          {/* View Tabs */}
          <div className="h-10 bg-slate-900 border-b border-slate-800 px-2 flex items-center gap-1 select-none overflow-x-auto">
            <button
              onClick={() => setActiveTab('CFG')}
              className={`flex items-center gap-1.5 px-3 py-1.5 rounded-md text-xs font-medium transition ${
                activeTab === 'CFG'
                  ? 'bg-slate-800 text-cyan-400 border border-slate-700 shadow-sm'
                  : 'text-slate-400 hover:text-slate-200'
              }`}
            >
              <GitBranch className="w-3.5 h-3.5" />
              <span>Control Flow Graph</span>
            </button>

            <button
              onClick={() => setActiveTab('OPTIMIZE')}
              className={`flex items-center gap-1.5 px-3 py-1.5 rounded-md text-xs font-medium transition ${
                activeTab === 'OPTIMIZE'
                  ? 'bg-slate-800 text-cyan-400 border border-slate-700 shadow-sm'
                  : 'text-slate-400 hover:text-slate-200'
              }`}
            >
              <Zap className="w-3.5 h-3.5" />
              <span>Optimizations</span>
            </button>

            <button
              onClick={() => setActiveTab('DIAGNOSTICS')}
              className={`flex items-center gap-1.5 px-3 py-1.5 rounded-md text-xs font-medium transition ${
                activeTab === 'DIAGNOSTICS'
                  ? 'bg-slate-800 text-cyan-400 border border-slate-700 shadow-sm'
                  : 'text-slate-400 hover:text-slate-200'
              }`}
            >
              <AlertTriangle className="w-3.5 h-3.5" />
              <span>Diagnostics ({diagnostics.length})</span>
            </button>

            <button
              onClick={() => setActiveTab('REGISTERS')}
              className={`flex items-center gap-1.5 px-3 py-1.5 rounded-md text-xs font-medium transition ${
                activeTab === 'REGISTERS'
                  ? 'bg-slate-800 text-cyan-400 border border-slate-700 shadow-sm'
                  : 'text-slate-400 hover:text-slate-200'
              }`}
            >
              <Database className="w-3.5 h-3.5" />
              <span>Registers & Liveness</span>
            </button>

            <button
              onClick={() => setActiveTab('VALIDATION')}
              className={`flex items-center gap-1.5 px-3 py-1.5 rounded-md text-xs font-medium transition ${
                activeTab === 'VALIDATION'
                  ? 'bg-slate-800 text-cyan-400 border border-slate-700 shadow-sm'
                  : 'text-slate-400 hover:text-slate-200'
              }`}
            >
              <ShieldCheck className="w-3.5 h-3.5" />
              <span>ptxas Validator</span>
            </button>
          </div>

          {/* View Tab Body */}
          <div className="flex-1 min-h-0">
            {activeTab === 'CFG' && (
              <CFGView cfg={cfg} onSelectBlock={handleSelectBlock} />
            )}

            {activeTab === 'OPTIMIZE' && (
              <OptimizationDiff
                originalPtx={workbenchData?.original_ptx || ptxSource}
                optimizedPtx={workbenchData?.optimized_ptx || ''}
                passResults={passResults}
                metrics={metrics}
              />
            )}

            {activeTab === 'DIAGNOSTICS' && (
              <DiagnosticsPanel
                diagnostics={diagnostics}
                onSelectLine={handleSelectLine}
              />
            )}

            {activeTab === 'REGISTERS' && (
              <RegisterAnalysis
                analysis={analysis}
                onSelectLine={handleSelectLine}
              />
            )}

            {activeTab === 'VALIDATION' && (
              <ValidationPanel
                validation={validationData}
                onValidate={handleRunValidate}
                loading={loading}
                selectedArch={selectedArch}
                onSelectArch={setSelectedArch}
              />
            )}
          </div>
        </div>
      </div>
    </div>
  );
};
export default App;
