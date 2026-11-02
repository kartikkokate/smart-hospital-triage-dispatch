import { useCallback, useEffect, useRef, useState, type DependencyList } from 'react'
import { errorMessage } from '../api/client'
import { useApp } from './AppContext'

export interface AsyncState<T> {
  data: T | undefined
  error: string | null
  loading: boolean
  reload: () => void
}

/**
 * Loads data from the backend. Refetches when `deps` change, when the user retries, and after every
 * mutation anywhere in the app (AppContext.refreshKey). On failure no data is kept: pages show the error
 * instead of stale or substitute data.
 */
export function useAsync<T>(fetcher: () => Promise<T>, deps: DependencyList): AsyncState<T> {
  const { refreshKey } = useApp()
  const [state, setState] = useState<{ data: T | undefined; error: string | null; loading: boolean }>({
    data: undefined,
    error: null,
    loading: true,
  })
  const [retry, setRetry] = useState(0)
  const latest = useRef(0)

  useEffect(() => {
    const id = ++latest.current
    setState((s) => ({ ...s, loading: true }))
    fetcher()
      .then((data) => {
        if (id === latest.current) setState({ data, error: null, loading: false })
      })
      .catch((err) => {
        if (id === latest.current) setState({ data: undefined, error: errorMessage(err), loading: false })
      })
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [...deps, refreshKey, retry])

  const reload = useCallback(() => setRetry((r) => r + 1), [])
  return { ...state, reload }
}
