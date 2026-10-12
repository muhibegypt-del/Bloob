export interface ResolveResult {
  state: 'ready' | 'waiting' | 'qualities' | 'error';
  url?: string | null;
  delayed?: number | null;
  id?: string | null;
  streams?: { id: string; quality?: string | number; ext?: string; filesize?: number }[];
  filename?: string;
  filesize?: number;
  timeLeft?: number;
}

export interface AccountStatus {
  paired: boolean;
  persistent: boolean;
  /** Where the pairing is saved: app-data, home, service, temporary, configured, or null. */
  storage: string | null;
  /** Survives TV reboots and app reinstalls. */
  durable: boolean;
  attempts: { location: string; result: string }[];
}

export class ApiError extends Error {
  constructor(message: string, readonly status: number) { super(message); }
}

/** Call the local service. Rejects with ApiError; aborts with the signal's DOMException. */
export async function api<T>(path: string, body?: object, signal?: AbortSignal): Promise<T> {
  const response = await fetch('/api/' + path, {
    method: body ? 'POST' : 'GET',
    headers: body ? { 'Content-Type': 'application/json' } : undefined,
    body: body ? JSON.stringify(body) : undefined,
    signal,
  });
  let result: { error?: string } & Record<string, unknown>;
  try { result = await response.json(); }
  catch { throw new ApiError(`The local service returned an unreadable response (${response.status})`, response.status); }
  if (!response.ok) throw new ApiError(result.error || `Request failed (${response.status})`, response.status);
  return result as T;
}
