import pytest
from fastapi.testclient import TestClient
from app.main import app
from app.samples_service import samples_service

client = TestClient(app)

SAMPLE_PTX = """
.version 7.0
.target sm_70
.address_size 64

.entry test_kernel(
    .param .u64 ptr_out
) {
    .reg .u64 %rd1;
    .reg .u32 %r1, %r2, %r3;

    ld.param.u64 %rd1, [ptr_out];
    add.u32 %r1, 10, 20;
    add.u32 %r2, %r1, 0;
    st.global.u32 [%rd1], %r2;
    ret;
}
"""

def test_health():
    res = client.get("/api/health")
    assert res.status_code == 200
    data = res.json()
    assert data["status"] == "ok"
    assert "engine_ready" in data
    assert "ptxas_installed" in data

def test_capabilities():
    res = client.get("/api/capabilities")
    assert res.status_code == 200
    data = res.json()
    assert data["version"] == "1.0.0"
    assert "const-fold" in data["supported_passes"]
    assert "mov" in data["supported_instructions"]

def test_samples_endpoints():
    res = client.get("/api/samples")
    assert res.status_code == 200
    samples = res.json()
    assert len(samples) >= 6

    first_id = samples[0]["id"]
    detail_res = client.get(f"/api/samples/{first_id}")
    assert detail_res.status_code == 200
    detail = detail_res.json()
    assert ".entry" in detail["source"]

def test_workbench_endpoint():
    res = client.post("/api/workbench", json={"source": SAMPLE_PTX, "passes": ["const-fold", "algebraic-simp"]})
    assert res.status_code == 200
    data = res.json()
    assert data["success"] is True
    assert data["metrics"]["initial_instruction_count"] >= 4
    assert len(data["pass_results"]) == 2
    assert "cfg" in data
    assert "analysis" in data
    assert "diagnostics" in data

def test_malformed_ptx():
    bad_ptx = ".version 7.0\n.entry broken() {\n add.u32 %r1\n ret;\n}"
    res = client.post("/api/workbench", json={"source": bad_ptx})
    assert res.status_code == 200
    data = res.json()
    assert len(data["diagnostics"]) > 0
    # Has syntax error
    has_error = any(d["severity"] == "ERROR" for d in data["diagnostics"])
    assert has_error

def test_cfg_endpoint():
    res = client.post("/api/cfg", json={"source": SAMPLE_PTX})
    assert res.status_code == 200
    cfg = res.json()
    assert "nodes" in cfg
    assert len(cfg["nodes"]) >= 1

def test_optimize_endpoint():
    res = client.post("/api/optimize", json={"source": SAMPLE_PTX, "passes": ["const-fold"]})
    assert res.status_code == 200
    data = res.json()
    assert "optimized_ptx" in data
    assert len(data["pass_results"]) == 1
    assert data["pass_results"][0]["pass_name"] == "const-fold"

def test_validate_endpoint():
    res = client.post("/api/validate", json={"source": SAMPLE_PTX, "gpu_name": "sm_70"})
    assert res.status_code == 200
    val = res.json()
    assert "available" in val
    assert "compilation_success" in val
