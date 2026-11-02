import type { Patient } from './patient'

export type ResourceState = 'AVAILABLE' | 'EN_ROUTE_PICKUP' | 'TRANSPORTING_PATIENT'
export type DispatchStatus = 'AWAITING_ASSIGNMENT' | 'RESOURCE_ASSIGNED' | 'IN_TRANSIT'

export interface Resource {
  id: string
  name: string
  state: ResourceState
  nodeId: number
  nodeName: string
  assignedPatientId: string
}

export interface FleetSummary {
  total: number
  available: number
  enRoutePickup: number
  transporting: number
}

export interface RouteSegment {
  u: number
  v: number
  seconds: number
}

export interface Route {
  from: number
  to: number
  fromName: string
  toName: string
  reachable: boolean
  costSeconds: number | null
  path: number[]
  pathNames: string[]
  segments: RouteSegment[]
}

export interface Dispatch {
  patient: Patient
  status: DispatchStatus
  pickupNode: number
  destinationNode: number
  resource: Resource | null
  leg1: Route | null
  leg2: Route | null
  totalCostSeconds: number | null
}

export interface ResourcesResponse {
  resources: Resource[]
  summary: FleetSummary
}

export interface PendingDispatchResponse {
  dispatches: Dispatch[]
  summary: FleetSummary
}

export interface SelectNextResponse {
  patient: Patient
  source: 'TRIAGE_QUEUE' | 'PENDING_POOL'
  triageQueueSize: number
}
