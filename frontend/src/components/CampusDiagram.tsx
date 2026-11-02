import type { CampusEdge, CampusNode } from '../types/campus'
import { edgeKey } from '../lib/format'
import { Icon } from './ui'

/**
 * Presentation-only layout: screen coordinates (percent) per node id, hand-placed so that no corridor
 * passes through another node. Nodes, corridors, weights and blocked state all come from the backend;
 * only the on-screen position is chosen here. Unknown node ids fall back to a circle.
 */
const LAYOUT: Record<number, { x: number; y: number; s: string }> = {
  0: { x: 10, y: 56, s: 'AMB' },
  1: { x: 30, y: 50, s: 'TRI' },
  2: { x: 22, y: 22, s: 'RES' },
  3: { x: 72, y: 35, s: 'ORT' },
  4: { x: 45, y: 10, s: 'ICU' },
  5: { x: 52, y: 35, s: 'RAD' },
  6: { x: 28, y: 84, s: 'LAB' },
  7: { x: 52, y: 72, s: 'PHA' },
  8: { x: 90, y: 50, s: 'WRD' },
  9: { x: 8, y: 8, s: 'HEL' },
  10: { x: 90, y: 78, s: 'ISO' },
  11: { x: 72, y: 92, s: 'UTL' },
}

function place(nodes: CampusNode[]): Map<number, { x: number; y: number; s: string }> {
  const map = new Map<number, { x: number; y: number; s: string }>()
  nodes.forEach((n, i) => {
    const known = LAYOUT[n.id]
    if (known && nodes.length === 12) {
      map.set(n.id, known)
    } else {
      const angle = (2 * Math.PI * i) / nodes.length - Math.PI / 2
      map.set(n.id, { x: 50 + 40 * Math.cos(angle), y: 50 + 38 * Math.sin(angle), s: n.name.slice(0, 3).toUpperCase() })
    }
  })
  return map
}

export interface DiagramProps {
  nodes: CampusNode[]
  edges: CampusEdge[]
  full?: boolean
  zoom?: number
  /** Corridors to draw as the highlighted route / tree (pairs of node ids). */
  highlightEdges?: [number, number][]
  /** Nodes drawn as selected (route path, traversal order, ...). */
  highlightNodes?: number[]
  pickupNode?: number
  destinationNode?: number
  vehicleNode?: number
  selectedEdge?: [number, number] | null
  onSelectEdge?: (u: number, v: number) => void
  showKey?: boolean
}

export function CampusDiagram({
  nodes, edges, full = false, zoom = 1, highlightEdges = [], highlightNodes = [], pickupNode, destinationNode,
  vehicleNode, selectedEdge, onSelectEdge, showKey = false,
}: DiagramProps) {
  const pos = place(nodes)
  const hl = new Set(highlightEdges.map(([a, b]) => edgeKey(a, b)))
  const hn = new Set(highlightNodes)
  const sel = selectedEdge ? edgeKey(selectedEdge[0], selectedEdge[1]) : null
  const vehicle = vehicleNode !== undefined ? pos.get(vehicleNode) : undefined

  return <div className={`campus-diagram ${full ? 'full' : ''}`}>
    <div className="zoom-layer" style={{ transform: `scale(${zoom})` }}>
      <svg viewBox="0 0 100 100" preserveAspectRatio="none" style={{ inset: 0, width: '100%', height: '100%' }}>
        {edges.map(e => {
          const a = pos.get(e.u), b = pos.get(e.v)
          if (!a || !b) return null
          const key = edgeKey(e.u, e.v)
          const cls = e.blocked ? 'blocked-line' : hl.has(key) ? 'route-line' : 'corridor'
          return <g key={key}>
            <line className={`${cls} ${sel === key ? 'selected-edge' : ''}`} x1={a.x} y1={a.y} x2={b.x} y2={b.y}/>
            {onSelectEdge && <line className="edge-hit" x1={a.x} y1={a.y} x2={b.x} y2={b.y} onClick={() => onSelectEdge(e.u, e.v)}><title>{`${e.uName} ↔ ${e.vName} · ${e.seconds}s${e.blocked ? ' · blocked' : ''}`}</title></line>}
          </g>
        })}
      </svg>
      {nodes.map(n => {
        const p = pos.get(n.id)!
        return <div className={`map-node ${hn.has(n.id) ? 'selected' : ''} ${n.id === pickupNode ? 'pickup' : ''} ${n.id === destinationNode ? 'destination' : ''}`} style={{ left: `${p.x}%`, top: `${p.y}%` }} key={n.id} title={n.name}><b>{p.s}</b><span>{n.id}</span></div>
      })}
      {vehicle && <div className="vehicle-marker" style={{ left: `${vehicle.x}%`, top: `calc(${vehicle.y}% + 24px)` }}><Icon name="truck" size={14}/></div>}
    </div>
    {showKey && <div className="map-key"><span><i className="pickup"/>Pickup</span><span><i className="dest"/>Destination</span><span><i className="route"/>Optimal route</span></div>}
  </div>
}
