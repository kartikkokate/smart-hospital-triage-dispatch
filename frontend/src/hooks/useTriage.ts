import { getTriageNext, getTriageQueue } from '../api/triage'
import { useAsync } from './useAsync'

/** Priority queue in real TriageMinHeap order, plus the heap root reported by /triage/next. */
export function useTriage() {
  return useAsync(async () => {
    const [queue, next] = await Promise.all([getTriageQueue(), getTriageNext()])
    return { ...queue, next: next.patient }
  }, [])
}
