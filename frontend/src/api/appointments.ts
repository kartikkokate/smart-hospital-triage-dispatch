import { del, get, post } from './client'
import type { AppointmentsResponse } from '../types/appointment'
import type { Patient } from '../types/patient'

export const listAppointments = () => get<AppointmentsResponse>('/appointments')
export const scheduleAppointment = (patientId: string) =>
  post<{ patient: Patient; queueSize: number }>('/appointments', { patientId })
export const cancelAppointment = (patientId: string) =>
  del<{ patient: Patient; queueSize: number }>(`/appointments/${encodeURIComponent(patientId)}`)
export const callNextAppointment = () => post<{ patient: Patient; queueSize: number }>('/appointments/next')
export const convertAppointment = (patientId: string, urgency: number) =>
  post<{ patient: Patient; triageQueueSize: number }>(`/appointments/${encodeURIComponent(patientId)}/convert`, { urgency })
