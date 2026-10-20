export interface DiagnosticItem {
  severity: 'ERROR' | 'WARNING' | 'OPTIMIZATION_SUGGESTION' | 'NOTE';
  code: string;
  message: string;
  line: number;
  column: number;
  length: number;
  explanation: string;
  suggested_action: string;
}

export interface CFGNode {
  id: string;
  label: string;
  is_entry: boolean;
  is_unreachable: boolean;
  instruction_count: number;
  instructions: string[];
  predecessors: string[];
  successors: string[];
}

export interface CFGEdge {
  id: string;
  source: string;
  target: string;
  type: string;
  condition: string;
}

export interface CFGData {
  function?: string;
  nodes?: CFGNode[];
  edges?: CFGEdge[];
  reachable_blocks?: string[];
  unreachable_blocks?: string[];
}

export interface RegisterItem {
  name: string;
  def_count: number;
  use_count: number;
  is_builtin: boolean;
  def_lines: number[];
  use_lines: number[];
}

export interface BlockLivenessItem {
  block: string;
  live_in: string[];
  live_out: string[];
}

export interface FunctionAnalysis {
  function: string;
  registers: RegisterItem[];
  liveness: BlockLivenessItem[];
}

export interface PassResultItem {
  pass_name: string;
  modified: boolean;
  transformations: number;
  explanations: string[];
  skipped_reasons: string[];
}

export interface PtxasValidationResult {
  available: boolean;
  executable_path: string;
  compilation_success: boolean;
  exit_code: number;
  stdout: string;
  stderr: string;
  duration_ms: number;
  target_arch: string;
  status_message: string;
}

export interface Metrics {
  initial_instruction_count: number;
  optimized_instruction_count: number;
  basic_block_count: number;
  function_count: number;
  execution_duration_ms: number;
}

export interface WorkbenchResponse {
  success: boolean;
  metrics: Metrics;
  original_ptx: string;
  optimized_ptx: string;
  cfg: CFGData;
  analysis: FunctionAnalysis[];
  diagnostics: DiagnosticItem[];
  pass_results: PassResultItem[];
  ptxas_validation: PtxasValidationResult;
}

export interface SampleItem {
  id: string;
  filename: string;
  title: string;
  description: string;
}

export interface Capabilities {
  version: string;
  engine_available: boolean;
  engine_path: string;
  ptxas_available: boolean;
  ptxas_path: string | null;
  supported_directives: string[];
  supported_instructions: string[];
  supported_passes: string[];
  supported_architectures: string[];
}
