import { get, post } from './client'
import type { CampusNode, EdgesResponse, MstResponse, Route, TraversalResponse } from '../types/campus'

export const listNodes = () => get<{ nodes: CampusNode[] }>('/campus/nodes')
export const listEdges = () => get<EdgesResponse>('/campus/edges')
export const getRoute = (from: number, to: number) => get<Route>('/campus/route', { from, to })
export const getTraversal = (algorithm: 'bfs' | 'dfs', start: number) =>
  get<TraversalResponse>('/campus/traversal', { algorithm, start })
export const getMst = (algorithm: 'prim' | 'kruskal') => get<MstResponse>('/campus/mst', { algorithm })
export const blockCorridor = (u: number, v: number) =>
  post<{ u: number; v: number; blocked: boolean }>('/campus/corridor/block', { u, v })
export const reopenCorridor = (u: number, v: number) =>
  post<{ u: number; v: number; blocked: boolean }>('/campus/corridor/reopen', { u, v })
