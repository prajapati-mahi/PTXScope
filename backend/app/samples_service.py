import os
from pathlib import Path
from typing import List, Dict, Optional

SAMPLE_METADATA = {
    "01_vector_add.ptx": {
        "title": "Vector Addition (vecAdd)",
        "description": "Standard parallel GPU vector addition kernel demonstrating 64-bit parameter loading, thread indexing with special registers, and memory coalescing."
    },
    "02_constant_folding.ptx": {
        "title": "Constant Folding & Type Safety",
        "description": "Compile-time integer arithmetic folding with strict type truncation, preserving floating-point operations according to IEEE-754 semantics."
    },
    "03_redundant_moves.ptx": {
        "title": "Redundant Move & Algebraic Identities",
        "description": "Detection of self-moves (mov %r1, %r1) and algebraic identity simplifications (x + 0 -> x, x * 1 -> x, x * 0 -> 0)."
    },
    "04_dead_code.ptx": {
        "title": "Dead Code & Unreachable Blocks",
        "description": "Control-flow reachability analysis, dead basic block elimination, and dead register assignment pruning with strict preservation of observable stores."
    },
    "05_branching_loop.ptx": {
        "title": "Branching & Loop CFG",
        "description": "Multi-block loop with predicate evaluation, induction stepping, and conditional back-edges visualized in the Control Flow Graph."
    },
    "06_load_store.ptx": {
        "title": "Load / Store Memory Pipeline",
        "description": "Pipelined memory reads with register displacement offsets, intra-warp barrier synchronization (bar.sync), and global store operations."
    }
}

class SamplesService:
    def __init__(self):
        current_dir = Path(__file__).resolve().parent
        self._samples_dir = current_dir.parent.parent / "samples"

    def list_samples(self) -> List[Dict[str, str]]:
        samples = []
        if not self._samples_dir.exists():
            return samples

        for file_path in sorted(self._samples_dir.glob("*.ptx")):
            meta = SAMPLE_METADATA.get(file_path.name, {
                "title": file_path.stem.replace("_", " ").title(),
                "description": "PTX sample assembly program."
            })
            samples.append({
                "id": file_path.stem,
                "filename": file_path.name,
                "title": meta["title"],
                "description": meta["description"]
            })
        return samples

    def get_sample_source(self, name_or_id: str) -> Optional[Dict[str, str]]:
        if not self._samples_dir.exists():
            return None

        # Try exact filename or id
        target = self._samples_dir / name_or_id
        if not target.exists():
            target = self._samples_dir / (name_or_id + ".ptx")
        if not target.exists():
            # Search by stem
            for f in self._samples_dir.glob("*.ptx"):
                if f.stem == name_or_id:
                    target = f
                    break

        if target.exists() and target.is_file():
            meta = SAMPLE_METADATA.get(target.name, {
                "title": target.stem.replace("_", " ").title(),
                "description": "PTX sample assembly program."
            })
            return {
                "id": target.stem,
                "filename": target.name,
                "title": meta["title"],
                "description": meta["description"],
                "source": target.read_text(encoding="utf-8")
            }
        return None

samples_service = SamplesService()
