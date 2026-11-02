import { useState } from 'react'
import { blockCorridor, getMst, getRoute, getTraversal, reopenCorridor } from '../api/campus'
import { CampusDiagram } from '../components/CampusDiagram'
import { Async, Button, Card, Icon, Info, PageHeader } from '../components/ui'
import { useAction } from '../hooks/useAction'
import { useAsync } from '../hooks/useAsync'
import { useCampus } from '../hooks/useCampus'
import { edgeKey } from '../lib/format'
import type { MstResponse, Route, TraversalResponse } from '../types/campus'

const ALGORITHMS = ['Dijkstra', 'BFS', 'DFS', "Prim's MST", "Kruskal's MST"] as const
type Algorithm = (typeof ALGORITHMS)[number]
const COMPLEXITY: Record<Algorithm, string> = {
  Dijkstra: 'O((V + E) log V)',
  BFS: 'O(V + E)',
  DFS: 'O(V + E)',
  "Prim's MST": 'O((V + E) log V)',
  "Kruskal's MST": 'O(E log E)',
}

type AlgoOut = { kind: 'route'; route: Route } | { kind: 'traversal'; t: TraversalResponse } | { kind: 'mst'; m: MstResponse }

export function Campus() {
  const campus = useCampus()
  const { run, busy } = useAction()
  const [algo, setAlgo] = useState<Algorithm>('Dijkstra')
  const [zoom, setZoom] = useState(1)
  const [edgeSel, setEdgeSel] = useState<[number, number] | null>(null)
  const [showAll, setShowAll] = useState(false)
  const [from, setFrom] = useState<string>('')
  const [to, setTo] = useState<string>('')
  const [start, setStart] = useState<string>('')

  const nodes = campus.data?.nodes
  const src = from !== '' ? Number(from) : nodes?.[0]?.id
  const dst = to !== '' ? Number(to) : nodes?.[3]?.id ?? nodes?.[nodes.length - 1]?.id
  const trav = start !== '' ? Number(start) : nodes?.[0]?.id

  // The algorithm explorer runs the real C++ algorithm for every selection / graph change.
  const result = useAsync<AlgoOut | undefined>(async () => {
    if (!nodes || nodes.length === 0 || src === undefined || dst === undefined || trav === undefined) return undefined
    if (algo === 'Dijkstra') return { kind: 'route', route: await getRoute(src, dst) }
    if (algo === 'BFS') return { kind: 'traversal', t: await getTraversal('bfs', trav) }
    if (algo === 'DFS') return { kind: 'traversal', t: await getTraversal('dfs', trav) }
    return { kind: 'mst', m: await getMst(algo === "Prim's MST" ? 'prim' : 'kruskal') }
  }, [algo, src, dst, trav, !!nodes])

  const out = result.data
  const highlightEdges: [number, number][] = out?.kind === 'route' ? out.route.segments.map(s => [s.u, s.v] as [number, number])
    : out?.kind === 'mst' ? out.m.edges.map(e => [e.u, e.v] as [number, number]) : []
  const highlightNodes = out?.kind === 'route' ? out.route.path : out?.kind === 'traversal' ? out.t.order : []

  return <>
    <PageHeader title="Campus Map" subtitle="Manage hospital locations, corridors and route optimization." action={<div className="live"><i/> {nodes ? `${nodes.length} locations online` : 'Loading locations…'}{campus.data && campus.data.blockedCount > 0 ? ` · ${campus.data.blockedCount} corridor${campus.data.blockedCount === 1 ? '' : 's'} blocked` : ''}</div>}/>
    <Async q={campus}>{c => {
      const edge = c.edges.find(e => edgeSel && edgeKey(e.u, e.v) === edgeKey(edgeSel[0], edgeSel[1])) ?? c.edges[0]
      const locations = showAll ? c.nodes : c.nodes.slice(0, 6)
      const select = (value: string, set: (v: string) => void, label: string, fallback: number | undefined) =>
        <label className="algo-select">{label}<select value={value !== '' ? value : String(fallback ?? '')} onChange={e => set(e.target.value)}>{c.nodes.map(n => <option key={n.id} value={n.id}>{n.id} · {n.name}</option>)}</select></label>
      return <>
        <div className="campus-layout"><Card className="campus-map-card"><div className="section-head"><div><span className="eyebrow">Hospital topology</span><h2>Connected campus network</h2></div><div className="map-actions"><Button secondary onClick={() => setZoom(z => Math.max(0.8, +(z - 0.1).toFixed(1)))}>−</Button><Button secondary onClick={() => setZoom(z => Math.min(1.4, +(z + 0.1).toFixed(1)))}>+</Button></div></div>
          <CampusDiagram full zoom={zoom} nodes={c.nodes} edges={c.edges} highlightEdges={highlightEdges} highlightNodes={highlightNodes} selectedEdge={edge ? [edge.u, edge.v] : null} onSelectEdge={(u, v) => setEdgeSel([u, v])}/>
          <div className="map-legend"><span><i className="normal"/>Operational corridor</span><span><i className="route"/>Selected route / tree</span><span><i className="blocked"/>Blocked</span><span className="hint">Click a corridor to select it</span></div></Card>
          <div className="side-stack">
            <Card><span className="eyebrow">Selected corridor</span>
              {edge ? <>
                <h2>{edge.uName} ↔ {edge.vName}</h2>
                <div className="corridor-details"><Info label="Nodes" value={`${edge.u} — ${edge.v}`}/><Info label="Transit time" value={`${edge.seconds} seconds`}/><Info label="Status" value={edge.blocked ? 'Blocked' : 'Operational'}/></div>
                <Button danger={edge.blocked} disabled={busy} onClick={() => run(
                  () => (edge.blocked ? reopenCorridor(edge.u, edge.v) : blockCorridor(edge.u, edge.v)),
                  edge.blocked ? `Corridor ${edge.u}–${edge.v} reopened.` : `Corridor ${edge.u}–${edge.v} blocked. Later route calculations avoid it.`)}>{edge.blocked ? 'Reopen corridor' : 'Block corridor'}</Button>
              </> : <p className="muted">No corridors loaded.</p>}
            </Card>
            <Card><span className="eyebrow">Location directory</span><div className="location-list">{locations.map(x => <div key={x.id}><b>{x.id}</b><span>{x.name}</span></div>)}</div>
              {c.nodes.length > 6 && <button className="text-link" onClick={() => setShowAll(v => !v)}>{showAll ? 'Show fewer locations' : `View all ${c.nodes.length} locations`} <Icon name="arrow"/></button>}</Card>
          </div></div>
        <Card className="algorithm-card"><div><span className="eyebrow">Algorithm explorer</span><h2>Visual route demonstration</h2><p>Runs the original C++ graph algorithms on the live campus network, including any blocked corridors.</p></div>
          <div className="algo-tabs">{ALGORITHMS.map(a => <button key={a} className={algo === a ? 'active' : ''} onClick={() => setAlgo(a)}>{a}</button>)}</div>
          <div className="algo-controls">
            {algo === 'Dijkstra' && <>{select(from, setFrom, 'From', src)}{select(to, setTo, 'To', dst)}</>}
            {(algo === 'BFS' || algo === 'DFS') && select(start, setStart, 'Start at', trav)}
          </div>
          <div className="algo-result"><span>Current algorithm</span><b>{algo}</b>
            <span>Result</span><b>{result.error ? <span className="field-error">{result.error}</span> : !out ? 'Calculating…' :
              out.kind === 'route' ? (out.route.reachable ? `${out.route.path.join(' → ')} · ${out.route.costSeconds}s` : 'No open route between these locations')
              : out.kind === 'traversal' ? `${out.t.order.join(' → ')} (${out.t.visitedCount} reachable)`
              : `Minimum network cost: ${out.m.totalWeightSeconds}s across ${out.m.edges.length} corridors${out.m.connected ? '' : ` (forest of ${out.m.componentCount} components)`}`}</b>
            <span>Complexity</span><b>{COMPLEXITY[algo]}</b></div></Card>
      </>
    }}</Async>
  </>
}
