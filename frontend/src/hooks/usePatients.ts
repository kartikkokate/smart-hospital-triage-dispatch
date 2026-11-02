import { listPatients, getPatient, type PatientFilters } from '../api/patients'
import { useAsync } from './useAsync'

export function usePatients(filters: PatientFilters = {}) {
  return useAsync(() => listPatients(filters), [filters.q, filters.state, filters.urgency])
}

export function usePatientDetail(id: string | undefined) {
  return useAsync(() => (id ? getPatient(id) : Promise.resolve(undefined)), [id])
}
