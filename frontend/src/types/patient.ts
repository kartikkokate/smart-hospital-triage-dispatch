export type PatientState =
  | 'REGISTERED'
  | 'TRIAGED_WAITING'
  | 'APPOINTMENT_SCHEDULED'
  | 'DISPATCH_PENDING'
  | 'IN_TRANSIT'
  | 'UNDER_TREATMENT'
  | 'DISCHARGED'
  | 'CANCELLED'

export interface Patient {
  id: string
  name: string
  age: number
  complaint: string
  urgency: number
  urgencyName: string
  state: PatientState
  arrivalSequence: number
  arrivalTimeSeconds: number
  locationNode: number
  locationName: string
  targetNode: number
  targetName: string
  assignedResourceId: string
  billing: number
  /** Simulated seconds waited; null when the patient is not in a waiting state. */
  waitingSeconds: number | null
}

export interface PatientListResponse {
  patients: Patient[]
  total: number
  matched: number
}

export interface NewPatientInput {
  id: string
  name: string
  age: number
  complaint: string
  urgency: number
  locationNode: number
  targetNode: number
  billing: number
}

export interface AuditEvent {
  index: number
  timestampSeconds: number
  patientId: string
  patientName: string
  action: string
  priorState: PatientState
  newState: PatientState
  urgency: number
  locationNode: number
  locationName: string
  resourceId: string
  details: string
}

export interface PatientDetailResponse {
  patient: Patient
  history: AuditEvent[]
  dispatch: import('./dispatch').Dispatch | null
}

export interface TriageRow {
  rank: number
  patient: Patient
}

export interface HeapNode {
  index: number
  patientId: string
  urgency: number
  arrivalSequence: number
}

export interface TriageQueueResponse {
  size: number
  queue: TriageRow[]
  heapArray: HeapNode[]
  urgencyUpdates: number
  policy: string
}

export interface TriageNextResponse {
  patient: Patient | null
  queueSize: number
}
