import {
  WorkbenchResponse,
  Capabilities,
  SampleItem,
  PtxasValidationResult
} from './types';

const API_BASE = '/api';

export async function fetchHealth() {
  const res = await fetch(`${API_BASE}/health`);
  if (!res.ok) throw new Error('Health check failed');
  return res.json();
}

export async function fetchCapabilities(): Promise<Capabilities> {
  const res = await fetch(`${API_BASE}/capabilities`);
  if (!res.ok) throw new Error('Failed to fetch compiler capabilities');
  return res.json();
}

export async function fetchSamples(): Promise<SampleItem[]> {
  const res = await fetch(`${API_BASE}/samples`);
  if (!res.ok) throw new Error('Failed to fetch sample programs');
  return res.json();
}

export async function fetchSampleSource(id: string): Promise<{ source: string; title: string; description: string }> {
  const res = await fetch(`${API_BASE}/samples/${id}`);
  if (!res.ok) throw new Error(`Failed to load sample ${id}`);
  return res.json();
}

export async function runWorkbench(
  source: string,
  passes?: string[],
  gpu_name: string = 'sm_70'
): Promise<WorkbenchResponse> {
  const res = await fetch(`${API_BASE}/workbench`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ source, passes, gpu_name })
  });
  if (!res.ok) {
    const err = await res.json().catch(() => ({ detail: 'Unknown error' }));
    throw new Error(err.detail || `Server returned ${res.status}`);
  }
  return res.json();
}

export async function validatePtx(source: string, gpu_name: string = 'sm_70'): Promise<PtxasValidationResult> {
  const res = await fetch(`${API_BASE}/validate`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ source, gpu_name })
  });
  if (!res.ok) {
    const err = await res.json().catch(() => ({ detail: 'Validation failed' }));
    throw new Error(err.detail || 'Validation failed');
  }
  return res.json();
}
