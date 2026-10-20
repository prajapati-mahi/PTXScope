import os
import shutil
import subprocess
import tempfile
import time
from typing import Dict, Any, Optional

class PtxasService:
    def __init__(self):
        self._executable = self._find_ptxas()

    def _find_ptxas(self, custom_path: Optional[str] = None) -> str:
        if custom_path and os.path.exists(custom_path):
            return custom_path

        env_custom = os.getenv("PTXAS_PATH")
        if env_custom and os.path.exists(env_custom):
            return env_custom

        which = shutil.which("ptxas")
        if which:
            return which

        # Check standard Windows CUDA locations
        standard_win = [
            r"C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.6\bin\ptxas.exe",
            r"C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.5\bin\ptxas.exe",
            r"C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.4\bin\ptxas.exe",
            r"C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.0\bin\ptxas.exe",
            r"C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v11.8\bin\ptxas.exe"
        ]
        for p in standard_win:
            if os.path.exists(p):
                return p

        return ""

    @property
    def is_available(self) -> bool:
        return bool(self._find_ptxas())

    @property
    def executable_path(self) -> str:
        return self._find_ptxas()

    def validate(self, source: str, gpu_name: str = "sm_70", custom_path: Optional[str] = None) -> Dict[str, Any]:
        exe = self._find_ptxas(custom_path)
        if not exe:
            return {
                "available": False,
                "executable_path": "",
                "compilation_success": False,
                "exit_code": -1,
                "stdout": "",
                "stderr": "",
                "duration_ms": 0.0,
                "target_arch": gpu_name,
                "status_message": "NVIDIA ptxas compiler not found on host. Hardware verification skipped."
            }

        with tempfile.NamedTemporaryFile(mode="w", suffix=".ptx", delete=False, encoding="utf-8") as f_in:
            f_in.write(source)
            temp_in = f_in.name

        temp_out = temp_in + ".cubin"
        start_t = time.perf_counter()

        try:
            cmd = [exe, "--gpu-name", gpu_name, temp_in, "-o", temp_out]
            res = subprocess.run(cmd, capture_output=True, text=True, timeout=15)
            duration_ms = (time.perf_counter() - start_t) * 1000.0

            success = (res.returncode == 0)
            return {
                "available": True,
                "executable_path": exe,
                "compilation_success": success,
                "exit_code": res.returncode,
                "stdout": res.stdout,
                "stderr": res.stderr,
                "duration_ms": round(duration_ms, 2),
                "target_arch": gpu_name,
                "status_message": "Compilation succeeded via NVIDIA ptxas." if success else f"ptxas failed with exit code {res.returncode}."
            }
        except subprocess.TimeoutExpired:
            return {
                "available": True,
                "executable_path": exe,
                "compilation_success": False,
                "exit_code": -1,
                "stdout": "",
                "stderr": "ptxas execution timed out (15s limit).",
                "duration_ms": 15000.0,
                "target_arch": gpu_name,
                "status_message": "ptxas execution timed out."
            }
        finally:
            for p in [temp_in, temp_out]:
                if os.path.exists(p):
                    try:
                        os.remove(p)
                    except OSError:
                        pass

ptxas_service = PtxasService()
