import type { Patient } from './patient'

export interface AppointmentRow {
  position: number
  patient: Patient
}

export interface AppointmentStats {
  scheduledTotal: number
  called: number
  cancelled: number
  convertedToEmergency: number
}

export interface AppointmentsResponse {
  size: number
  queue: AppointmentRow[]
  stats: AppointmentStats
}
