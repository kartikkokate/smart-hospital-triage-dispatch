import { createContext, useCallback, useContext, useEffect, useMemo, useRef, useState, type ReactNode } from 'react'
import { getHealth } from '../api/analytics'
import type { HealthResponse } from '../types/analytics'

export type NoticeKind = 'success' | 'error' | 'info'
export interface Notice {
  id: number
  kind: NoticeKind
  text: string
}

interface BackendStatus {
  online: boolean | null // null while the first check is in flight
  health?: HealthResponse
}

interface AppContextValue {
  /** Increments after every mutation; data hooks refetch when it changes. */
  refreshKey: number
  refresh: () => void
  notices: Notice[]
  notify: (kind: NoticeKind, text: string) => void
  dismiss: (id: number) => void
  backend: BackendStatus
}

const AppContext = createContext<AppContextValue | null>(null)

export function AppProvider({ children }: { children: ReactNode }) {
  const [refreshKey, setRefreshKey] = useState(0)
  const [notices, setNotices] = useState<Notice[]>([])
  const [backend, setBackend] = useState<BackendStatus>({ online: null })
  const nextId = useRef(1)

  const refresh = useCallback(() => setRefreshKey((k) => k + 1), [])
  const dismiss = useCallback((id: number) => setNotices((list) => list.filter((n) => n.id !== id)), [])
  const notify = useCallback(
    (kind: NoticeKind, text: string) => {
      const id = nextId.current++
      setNotices((list) => [...list.slice(-3), { id, kind, text }])
      window.setTimeout(() => dismiss(id), kind === 'error' ? 9000 : 5000)
    },
    [dismiss],
  )

  // Backend availability + simulation clock, re-checked on every mutation and every 10 seconds.
  useEffect(() => {
    let cancelled = false
    const check = () =>
      getHealth()
        .then((health) => !cancelled && setBackend({ online: true, health }))
        .catch(() => !cancelled && setBackend({ online: false }))
    check()
    const timer = window.setInterval(check, 10000)
    return () => {
      cancelled = true
      window.clearInterval(timer)
    }
  }, [refreshKey])

  const value = useMemo(
    () => ({ refreshKey, refresh, notices, notify, dismiss, backend }),
    [refreshKey, refresh, notices, notify, dismiss, backend],
  )
  return <AppContext.Provider value={value}>{children}</AppContext.Provider>
}

export function useApp(): AppContextValue {
  const ctx = useContext(AppContext)
  if (!ctx) throw new Error('useApp must be used inside <AppProvider>')
  return ctx
}
