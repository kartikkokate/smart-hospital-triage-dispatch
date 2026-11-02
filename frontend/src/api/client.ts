const configured = import.meta.env.VITE_API_BASE_URL?.trim()

/** Single source of truth for the backend URL (set VITE_API_BASE_URL in .env files). */
export const API_BASE_URL = (configured || 'http://localhost:8080/api').replace(/\/+$/, '')

const REQUEST_TIMEOUT_MS = 15000

export class ApiError extends Error {
  readonly status: number
  readonly code: string
  /** True when the backend could not be reached at all (as opposed to rejecting the request). */
  readonly network: boolean

  constructor(status: number, code: string, message: string, network = false) {
    super(message)
    this.name = 'ApiError'
    this.status = status
    this.code = code
    this.network = network
  }
}

type Query = Record<string, string | number | undefined>

interface RequestOptions {
  body?: unknown
  query?: Query
}

function buildUrl(path: string, query?: Query): string {
  const url = new URL(API_BASE_URL + path)
  if (query) {
    for (const [key, value] of Object.entries(query)) {
      if (value !== undefined && value !== '') url.searchParams.set(key, String(value))
    }
  }
  return url.toString()
}

export async function request<T>(method: string, path: string, options: RequestOptions = {}): Promise<T> {
  const controller = new AbortController()
  const timer = setTimeout(() => controller.abort(), REQUEST_TIMEOUT_MS)
  let response: Response
  try {
    response = await fetch(buildUrl(path, options.query), {
      method,
      headers: options.body !== undefined ? { 'Content-Type': 'application/json' } : undefined,
      body: options.body !== undefined ? JSON.stringify(options.body) : undefined,
      signal: controller.signal,
    })
  } catch (err) {
    const timedOut = err instanceof DOMException && err.name === 'AbortError'
    throw new ApiError(
      0,
      timedOut ? 'TIMEOUT' : 'NETWORK',
      timedOut
        ? 'The hospital backend did not respond in time.'
        : `Cannot reach the hospital backend at ${API_BASE_URL}. Make sure the C++ server is running.`,
      true,
    )
  } finally {
    clearTimeout(timer)
  }

  let payload: unknown = null
  const text = await response.text()
  if (text) {
    try {
      payload = JSON.parse(text)
    } catch {
      payload = null
    }
  }

  if (!response.ok) {
    const err = (payload as { error?: { code?: string; message?: string } } | null)?.error
    throw new ApiError(response.status, err?.code ?? 'ERROR', err?.message ?? `Request failed (HTTP ${response.status}).`)
  }
  if (payload === null) throw new ApiError(response.status, 'BAD_RESPONSE', 'The backend returned an unreadable response.')
  return payload as T
}

export const get = <T>(path: string, query?: Query) => request<T>('GET', path, { query })
export const post = <T>(path: string, body: unknown = {}) => request<T>('POST', path, { body })
export const put = <T>(path: string, body: unknown) => request<T>('PUT', path, { body })
export const del = <T>(path: string) => request<T>('DELETE', path)

export function errorMessage(err: unknown): string {
  if (err instanceof Error) return err.message
  return 'Unexpected error.'
}
