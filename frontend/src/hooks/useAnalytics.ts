import { getAlgorithmComparison, getAudit, getOverview, getSortedPatients } from '../api/analytics'
import type { SortAlgorithm, SortField } from '../types/analytics'
import { useAsync } from './useAsync'

export const useOverview = () => useAsync(() => getOverview(), [])
export const useAudit = () => useAsync(() => getAudit(), [])
export const useSortedPatients = (sortBy: SortField, algorithm: SortAlgorithm) =>
  useAsync(() => getSortedPatients(sortBy, algorithm), [sortBy, algorithm])
export const useAlgorithmComparison = () => useAsync(() => getAlgorithmComparison(), [])
