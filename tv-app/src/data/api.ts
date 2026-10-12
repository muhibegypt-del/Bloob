export interface ResolveResult {
  state: 'ready' | 'waiting' | 'qualities' | 'error';
  url?: string | null;
  delayed?: number | null;
  id?: string | null;
  streams?: { id: string; quality?: string | number; ext?: string }[];
  filename?: string;
  timeLeft?: number;
}

export async function api<T>(path: string, body?: object): Promise<T> {
  const response = await fetch('/api/' + path, {
    method: body ? 'POST' : 'GET',
    headers: body ? { 'Content-Type': 'application/json' } : undefined,
    body: body ? JSON.stringify(body) : undefined,
  });
  const result = await response.json();
  if (!response.ok) throw new Error(result.error || `Request failed (${response.status})`);
  return result as T;
}
