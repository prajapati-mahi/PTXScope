import os
import subprocess
import tempfile
import json
import shutil
from pathlib import Path
from typing import Dict, Any, List, Optional
from fastapi import HTTPException

class EngineService:
    def __init__(self):
        self._engine_path = self._locate_engine()

    def _locate_engine(self) -> str:
        # Check environment variable
        env_path = os.getenv("PTXSCOPE_ENGINE_PATH")
        if env_path and os.path.exists(env_path):
            return env_path

        # Check relative project build directories
        current_dir = Path(__file__).resolve().parent
        project_root = current_dir.parent.parent
        candidates = [
            project_root / "build" / "engine" / "ptxscope.exe",
            project_root / "build" / "engine" / "ptxscope",
            project_root / "engine" / "build" / "ptxscope.exe",
            project_root / "engine" / "build" / "ptxscope",
        ]

        for cand in candidates:
            if cand.exists():
                return str(cand)

        # Check system PATH
        which_path = shutil.which("ptxscope")
        if which_path:
            return which_path

        return ""

    @property
    def is_available(self) -> bool:
        return bool(self._engine_path and os.path.exists(self._engine_path))

    @property
    def engine_path(self) -> str:
        return self._engine_path

    def run_workbench(self, source: str, passes: Optional[List[str]] = None, gpu_name: str = "sm_70") -> Dict[str, Any]:
        if not self.is_available:
            raise HTTPException(
                status_code=503,
                detail=f"PTXScope engine binary not found. Please compile the C++ engine first."
            )

        if len(source.encode("utf-8")) > 524288:
            raise HTTPException(status_code=400, detail="PTX source exceeds maximum allowed size of 512KB.")

        with tempfile.NamedTemporaryFile(mode="w", suffix=".ptx", delete=False, encoding="utf-8") as f:
            f.write(source)
            temp_path = f.name

        try:
            cmd = [self._engine_path, "json"]
            if passes:
                cmd.extend(["--passes", ",".join(passes)])
            if gpu_name:
                cmd.extend(["--gpu-name", gpu_name])
            cmd.append(temp_path)

            # Ensure toolchain runtime DLLs are found in PATH if on Windows
            env = os.environ.copy()
            local_app_data = os.getenv("LOCALAPPDATA", "")
            llvm_bin = os.path.join(
                local_app_data,
                r"Microsoft\WinGet\Packages\MartinStorsjo.LLVM-MinGW.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\llvm-mingw-20260908-ucrt-x86_64\bin"
            )
            if os.path.exists(llvm_bin):
                env["PATH"] = llvm_bin + os.pathsep + env.get("PATH", "")

            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=10,
                env=env
            )

            if result.returncode != 0 and not result.stdout.strip():
                raise HTTPException(
                    status_code=500,
                    detail=f"Engine error: {result.stderr.strip() or 'Unknown execution failure'}"
                )

            try:
                data = json.loads(result.stdout)
                return data
            except json.JSONDecodeError as jde:
                raise HTTPException(
                    status_code=500,
                    detail=f"Failed to parse engine JSON output: {str(jde)}\nRaw: {result.stdout[:200]}"
                )

        except subprocess.TimeoutExpired:
            raise HTTPException(status_code=408, detail="PTXScope engine timed out (limit: 10 seconds).")
        finally:
            if os.path.exists(temp_path):
                try:
                    os.remove(temp_path)
                except OSError:
                    pass

engine_service = EngineService()
