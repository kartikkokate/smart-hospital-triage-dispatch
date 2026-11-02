import { get, post } from './client'
import type { Dispatch, PendingDispatchResponse, ResourcesResponse, SelectNextResponse } from '../types/dispatch'
import type { Patient } from '../types/patient'

export const listResources = () => get<ResourcesResponse>('/dispatch/resources')
export const listPendingDispatches = () => get<PendingDispatchResponse>('/dispatch/pending')
export const selectNextForDispatch = () => post<SelectNextResponse>('/dispatch/select-next')
export const assignResource = (patientId: string) => post<{ dispatch: Dispatch }>('/dispatch/assign', { patientId })
export const runPickup = (patientId: string) =>
  post<{ dispatch: Dispatch; simulationTimeSeconds: number }>('/dispatch/pickup', { patientId })
export const completeTransport = (patientId: string) =>
  post<{ patient: Patient; simulationTimeSeconds: number }>('/dispatch/complete', { patientId })
export const cancelDispatch = (patientId: string) => post<{ patient: Patient }>('/dispatch/cancel', { patientId })
