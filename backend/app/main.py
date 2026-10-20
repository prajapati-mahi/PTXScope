from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from typing import List, Dict, Any

from app.models import (
    PTXInputRequest,
    ValidateRequest,
    WorkbenchResponse,
    CapabilitiesResponse,
    SampleItem,
    SampleDetailResponse,
    PtxasValidationResult,
    CFGResponse
)
from app.engine_service import engine_service
from app.ptxas_service import ptxas_service
from app.samples_service import samples_service

app = FastAPI(
    title="PTXScope Backend API",
    description="GPU Assembly Analyzer, Optimizer & Debugging Workbench Backend",
    version="1.0.0"
)

# Enable CORS for frontend development
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

@app.get("/api/health")
def get_health() -> Dict[str, Any]:
    return {
        "status": "ok",
        "engine_ready": engine_service.is_available,
        "engine_path": engine_service.engine_path,
        "ptxas_installed": ptxas_service.is_available,
        "ptxas_path": ptxas_service.executable_path or None
    }

@app.get("/api/capabilities", response_model=CapabilitiesResponse)
def get_capabilities():
    return CapabilitiesResponse(
        version="1.0.0",
        engine_available=engine_service.is_available,
        engine_path=engine_service.engine_path,
        ptxas_available=ptxas_service.is_available,
        ptxas_path=ptxas_service.executable_path or None,
        supported_directives=[
            ".version", ".target", ".address_size", ".entry", ".func",
            ".param", ".reg", ".local", ".shared", ".global", ".const"
        ],
        supported_instructions=[
            "mov", "add", "sub", "mul", "mad", "setp", "ld", "st", "bra", "ret", "exit", "bar.sync"
        ],
        supported_passes=[
            "const-fold", "algebraic-simp", "redundant-mov", "dce"
        ],
        supported_architectures=[
            "sm_70", "sm_75", "sm_80", "sm_86", "sm_89", "sm_90"
        ]
    )

@app.post("/api/workbench", response_model=WorkbenchResponse)
def analyze_workbench(req: PTXInputRequest):
    data = engine_service.run_workbench(req.source, passes=req.passes, gpu_name=req.gpu_name or "sm_70")
    return data

@app.post("/api/parse")
def parse_ptx(req: PTXInputRequest):
    data = engine_service.run_workbench(req.source, passes=[])
    return {
        "success": data.get("success", False),
        "metrics": data.get("metrics"),
        "diagnostics": data.get("diagnostics", [])
    }

@app.post("/api/analyze")
def analyze_ptx(req: PTXInputRequest):
    data = engine_service.run_workbench(req.source, passes=[])
    return {
        "analysis": data.get("analysis", []),
        "diagnostics": data.get("diagnostics", [])
    }

@app.post("/api/cfg")
def get_cfg(req: PTXInputRequest):
    data = engine_service.run_workbench(req.source, passes=[])
    return data.get("cfg", {})

@app.post("/api/optimize")
def optimize_ptx(req: PTXInputRequest):
    data = engine_service.run_workbench(req.source, passes=req.passes)
    return {
        "original_ptx": data.get("original_ptx", ""),
        "optimized_ptx": data.get("optimized_ptx", ""),
        "pass_results": data.get("pass_results", []),
        "metrics": data.get("metrics", {})
    }

@app.post("/api/validate", response_model=PtxasValidationResult)
def validate_ptx(req: ValidateRequest):
    res = ptxas_service.validate(req.source, gpu_name=req.gpu_name or "sm_70", custom_path=req.custom_path)
    return PtxasValidationResult(**res)

@app.get("/api/samples", response_model=List[SampleItem])
def list_samples():
    return samples_service.list_samples()

@app.get("/api/samples/{name}", response_model=SampleDetailResponse)
def get_sample(name: str):
    s = samples_service.get_sample_source(name)
    if not s:
        raise HTTPException(status_code=404, detail=f"Sample '{name}' not found.")
    return SampleDetailResponse(**s)

if __name__ == "__main__":
    import uvicorn
    uvicorn.run("app.main:app", host="127.0.0.1", port=8000, reload=True)
