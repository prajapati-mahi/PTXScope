#!/usr/bin/env bash
set -e

echo "=== Starting PTXScope Local Workbench ==="

# Trap INT and TERM signals to kill background processes
trap 'kill $(jobs -p)' EXIT

echo "Starting Backend on http://127.0.0.1:8000..."
PYTHONPATH=backend python3 -m uvicorn app.main:app --host 127.0.0.1 --port 8000 &

echo "Starting Frontend on http://localhost:5173..."
(cd frontend && npm run dev) &

echo "Servers running. Press Ctrl+C to stop."
wait
