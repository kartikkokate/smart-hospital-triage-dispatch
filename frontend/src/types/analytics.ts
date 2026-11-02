import type { AuditEvent, Patient, PatientState } from './patient'
import type { FleetSummary } from './dispatch'

export interface UrgencyCount {
  level: number
  name: string
  count: number
}

export interface Overview {
  totalPatients: number
  byState: Partial<Record<PatientState, number>>
  triageQueueSize: number
  appointmentQueueSize: number
  activeDispatches: number
  fleet: FleetSummary
  simulationTimeSeconds: number
  highestPriority: Patient | null
  recentActivity: AuditEvent[]
  urgencyDistribution: UrgencyCount[]
}

export interface SortMetrics {
  algorithm: string
  elementCount: number
  comparisons: number
  swaps: number
  moves: number
  durationMicroseconds: number
  stable: boolean
  timeBest: string
  timeAverage: string
  timeWorst: string
  spaceAuxiliary: string
}

export interface SortedPatient {
  id: string
  name: string
  age: number
  complaint: string
  urgency: number
  urgencyName: string
  state: PatientState
  arrivalSequence: number
  arrivalTimeSeconds: number
  billing: number
  waitingSeconds: number | null
}

export type SortField = 'age' | 'arrival' | 'urgency' | 'billing' | 'waiting'
export type SortAlgorithm = 'merge' | 'quick' | 'heap'

export interface SortedPatientsResponse {
  sortBy: string
  algorithm: string
  metrics: SortMetrics
  patients: SortedPatient[]
}

export interface GraphTiming {
  algorithm: string
  complexity: string
  iterations: number
  averageMicroseconds: number
}

export interface AlgorithmComparison {
  sortBy: string
  patientCount: number
  sorts: SortMetrics[]
  graph: GraphTiming[]
}

export interface AuditResponse {
  events: AuditEvent[]
  total: number
  returned: number
  order: 'chronological'
}

export interface HealthResponse {
  status: string
  initialized: boolean
  patients: number
  simulationTimeSeconds: number
  resetEnabled: boolean
}
