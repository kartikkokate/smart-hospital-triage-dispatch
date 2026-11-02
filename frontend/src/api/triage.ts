import { get, post, put } from './client'
import type { Patient, TriageNextResponse, TriageQueueResponse } from '../types/patient'

export const getTriageQueue = () => get<TriageQueueResponse>('/triage/queue')
export const getTriageNext = () => get<TriageNextResponse>('/triage/next')
export const admitToTriage = (patientId: string, urgency: number) =>
  post<{ patient: Patient; rank: number; queueSize: number }>('/triage/admit', { patientId, urgency })
export const updateTriageUrgency = (id: string, urgency: number) =>
  put<{ patient: Patient; rank: number | null }>(`/triage/${encodeURIComponent(id)}/urgency`, { urgency })
