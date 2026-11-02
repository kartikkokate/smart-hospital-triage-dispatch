import { listAppointments } from '../api/appointments'
import { useAsync } from './useAsync'

export function useAppointments() {
  return useAsync(() => listAppointments(), [])
}
