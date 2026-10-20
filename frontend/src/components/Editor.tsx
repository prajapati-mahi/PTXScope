import React, { useEffect, useRef } from 'react';
import MonacoEditor, { OnMount } from '@monaco-editor/react';
import { ptxLanguageDefinition, ptxTheme } from '../ptxLanguage';
import { DiagnosticItem } from '../types';

interface EditorProps {
  value: string;
  onChange: (val: string) => void;
  diagnostics: DiagnosticItem[];
  jumpLine?: number | null;
}

export const Editor: React.FC<EditorProps> = ({
  value,
  onChange,
  diagnostics,
  jumpLine
}) => {
  const editorRef = useRef<any>(null);
  const monacoRef = useRef<any>(null);

  const handleEditorDidMount: OnMount = (editor, monaco) => {
    editorRef.current = editor;
    monacoRef.current = monaco;

    // Register PTX Language
    monaco.languages.register({ id: 'ptx' });
    monaco.languages.setMonarchTokensProvider('ptx', ptxLanguageDefinition);
    monaco.editor.defineTheme('ptx-dark', ptxTheme);
    monaco.editor.setTheme('ptx-dark');
  };

  // Update diagnostic markers in the editor
  useEffect(() => {
    if (!monacoRef.current || !editorRef.current) return;
    const model = editorRef.current.getModel();
    if (!model) return;

    const markers = diagnostics.map((d) => ({
      startLineNumber: d.line,
      startColumn: d.column,
      endLineNumber: d.line,
      endColumn: d.column + (d.length || 4),
      message: `${d.code}: ${d.message}\n${d.explanation ? 'Explanation: ' + d.explanation : ''}`,
      severity:
        d.severity === 'ERROR'
          ? monacoRef.current.MarkerSeverity.Error
          : d.severity === 'WARNING'
          ? monacoRef.current.MarkerSeverity.Warning
          : monacoRef.current.MarkerSeverity.Info
    }));

    monacoRef.current.editor.setModelMarkers(model, 'ptxscope', markers);
  }, [diagnostics]);

  // Jump to line when selected in diagnostics panel
  useEffect(() => {
    if (jumpLine && editorRef.current) {
      editorRef.current.revealLineInCenter(jumpLine);
      editorRef.current.setPosition({ lineNumber: jumpLine, column: 1 });
      editorRef.current.focus();
    }
  }, [jumpLine]);

  return (
    <div className="h-full w-full relative bg-[#090d16] flex flex-col">
      <div className="h-7 bg-slate-900/80 border-b border-slate-800/80 px-3 flex items-center justify-between text-xs text-slate-400 font-mono select-none">
        <span className="flex items-center gap-2 text-slate-300">
          <span className="w-2 h-2 rounded-full bg-cyan-500"></span>
          Input PTX Assembly (.ptx)
        </span>
        <span>PTX ISA v7.0+ | sm_70+</span>
      </div>
      <div className="flex-1 min-h-0">
        <MonacoEditor
          height="100%"
          language="ptx"
          theme="ptx-dark"
          value={value}
          onChange={(val) => onChange(val || '')}
          onMount={handleEditorDidMount}
          options={{
            fontSize: 13,
            fontFamily: "'Fira Code', monospace",
            lineNumbers: 'on',
            minimap: { enabled: false },
            scrollBeyondLastLine: false,
            wordWrap: 'off',
            renderWhitespace: 'selection',
            automaticLayout: true,
            tabSize: 4,
            cursorBlinking: 'smooth',
            smoothScrolling: true,
            contextmenu: true
          }}
        />
      </div>
    </div>
  );
};
