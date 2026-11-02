import { get, post, put } from './client'
import type { NewPatientInput, Patient, PatientDetailResponse, PatientListResponse } from '../types/patient'

export interface PatientFilters {
  q?: string
  state?: string
  urgency?: number
}

export const listPatients = (filters: PatientFilters = {}) => get<PatientListResponse>('/patients', { ...filters })
export const getPatient = (id: string) => get<PatientDetailResponse>(`/patients/${encodeURIComponent(id)}`)
export const registerPatient = (input: NewPatientInput) => post<{ patient: Patient }>('/patients', input)
export const updatePatientUrgency = (id: string, urgency: number) =>
  put<{ patient: Patient }>(`/patients/${encodeURIComponent(id)}`, { urgency })
export const dischargePatient = (id: string) => post<{ patient: Patient }>(`/patients/${encodeURIComponent(id)}/discharge`)
