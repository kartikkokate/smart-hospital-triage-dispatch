import { get } from './client'
import type {
  AlgorithmComparison,
  AuditResponse,
  HealthResponse,
  Overview,
  SortAlgorithm,
  SortField,
  SortedPatientsResponse,
} from '../types/analytics'

export const getHealth = () => get<HealthResponse>('/health')
export const getOverview = () => get<Overview>('/analytics/overview')
export const getSortedPatients = (sortBy: SortField, algorithm: SortAlgorithm) =>
  get<SortedPatientsResponse>('/analytics/patients', { sortBy, algorithm })
export const getAlgorithmComparison = () => get<AlgorithmComparison>('/analytics/algorithms')
export const getAudit = (limit = 500) => get<AuditResponse>('/audit', { limit })
