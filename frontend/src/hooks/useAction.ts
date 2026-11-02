import { useCallback, useState } from 'react'
import { errorMessage } from '../api/client'
import { useApp } from './AppContext'

/**
 * Runs a backend mutation. Success is only reported after the backend confirms it; any rejection is shown
 * as an error. All data is refetched afterwards because even rejected operations are audited by the backend.
 */
export function useAction() {
  const { notify, refresh } = useApp()
  const [busy, setBusy] = useState(false)

  const run = useCallback(
    async <T,>(action: () => Promise<T>, success?: string | ((result: T) => string)): Promise<T | undefined> => {
      setBusy(true)
      try {
        const result = await action()
        if (success) notify('success', typeof success === 'function' ? success(result) : success)
        refresh()
        return result
      } catch (err) {
        notify('error', errorMessage(err))
        refresh()
        return undefined
      } finally {
        setBusy(false)
      }
    },
    [notify, refresh],
  )

  return { run, busy }
}
