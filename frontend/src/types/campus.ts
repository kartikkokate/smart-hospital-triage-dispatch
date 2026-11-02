import type { Route } from './dispatch'

export interface CampusNode {
  id: number
  name: string
  wing: string
  isTreatmentUnit: boolean
}

export interface CampusEdge {
  u: number
  v: number
  uName: string
  vName: string
  seconds: number
  blocked: boolean
}

export interface EdgesResponse {
  edges: CampusEdge[]
  blockedCount: number
}

export interface TraversalResponse {
  algorithm: 'bfs' | 'dfs'
  start: number
  order: number[]
  orderNames: string[]
  visitedCount: number
}

export interface MstResponse {
  algorithm: 'prim' | 'kruskal'
  connected: boolean
  totalWeightSeconds: number
  componentCount: number
  overflow: boolean
  edges: CampusEdge[]
}

export type { Route }
