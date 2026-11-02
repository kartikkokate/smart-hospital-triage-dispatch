import { listPendingDispatches, listResources } from '../api/dispatch'
import { useAsync } from './useAsync'

export function useDispatch() {
  return useAsync(async () => {
    const [pending, resources] = await Promise.all([listPendingDispatches(), listResources()])
    return { dispatches: pending.dispatches, resources: resources.resources, summary: resources.summary }
  }, [])
}
