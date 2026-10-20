from typing import List, Optional, Dict, Any
from pydantic import BaseModel, Field

class PTXInputRequest(BaseModel):
    source: str = Field(..., max_length=524288, description="PTX assembly code (max 512KB)")
    passes: Optional[List[str]] = Field(default=None, description="Optimization passes to run")
    gpu_name: Optional[str] = Field(default="sm_70", description="Target GPU architecture, e.g., sm_70, sm_80")

class ValidateRequest(BaseModel):
    source: str = Field(..., max_length=524288, description="PTX assembly code")
    gpu_name: Optional[str] = Field(default="sm_70", description="Target GPU architecture")
    custom_path: Optional[str] = Field(default=None, description="Custom path to ptxas executable")

class DiagnosticItem(BaseModel):
    severity: str
    code: str
    message: str
    line: int
    column: int
    length: int
    explanation: str
    suggested_action: str

class CFGNode(BaseModel):
    id: str
    label: str
    is_entry: bool
    is_unreachable: bool
    instruction_count: int
    instructions: List[str]
    predecessors: List[str]
    successors: List[str]

class CFGEdge(BaseModel):
    id: str
    source: str
    target: str
    type: str
    condition: str

class CFGResponse(BaseModel):
    function: str
    nodes: List[CFGNode]
    edges: List[CFGEdge]
    reachable_blocks: List[str]
    unreachable_blocks: List[str]

class RegisterItem(BaseModel):
    name: str
    def_count: int
    use_count: int
    is_builtin: bool
    def_lines: List[int]
    use_lines: List[int]

class BlockLivenessItem(BaseModel):
    block: str
    live_in: List[str]
    live_out: List[str]

class FunctionAnalysis(BaseModel):
    function: str
    registers: List[RegisterItem]
    liveness: List[BlockLivenessItem]

class PassResultItem(BaseModel):
    pass_name: str
    modified: bool
    transformations: int
    explanations: List[str]
    skipped_reasons: List[str]

class PtxasValidationResult(BaseModel):
    available: bool
    executable_path: str
    compilation_success: bool
    exit_code: int
    stdout: str
    stderr: str
    duration_ms: float
    target_arch: str
    status_message: str

class Metrics(BaseModel):
    initial_instruction_count: int
    optimized_instruction_count: int
    basic_block_count: int
    function_count: int
    execution_duration_ms: float

class WorkbenchResponse(BaseModel):
    success: bool
    metrics: Metrics
    original_ptx: str
    optimized_ptx: str
    cfg: Dict[str, Any]
    analysis: List[Dict[str, Any]]
    diagnostics: List[DiagnosticItem]
    pass_results: List[PassResultItem]
    ptxas_validation: PtxasValidationResult

class CapabilitiesResponse(BaseModel):
    version: str
    engine_available: bool
    engine_path: str
    ptxas_available: bool
    ptxas_path: Optional[str]
    supported_directives: List[str]
    supported_instructions: List[str]
    supported_passes: List[str]
    supported_architectures: List[str]

class SampleItem(BaseModel):
    id: str
    filename: str
    title: str
    description: str

class SampleDetailResponse(BaseModel):
    id: str
    filename: str
    title: str
    description: str
    source: str
