import { useState } from 'react'
import { Async, Badge, Card, Empty, PageHeader, SectionTitle, Stat } from '../components/ui'
import { useAlgorithmComparison, useAudit, useOverview, useSortedPatients } from '../hooks/useAnalytics'
import { usePatients } from '../hooks/usePatients'
import { actionLabel, conicFromCounts, downloadCsv, formatDuration, seqLabel, stateLabel, URGENCY_LEVELS } from '../lib/format'
import type { SortAlgorithm, SortField } from '../types/analytics'

const TABS = ['Patient Analytics', 'Reporting', 'Audit Trail', 'Algorithm Performance'] as const
type Tab = (typeof TABS)[number]

const SORT_FIELDS: { value: SortField; label: string }[] = [
  { value: 'billing', label: 'Sort by billing amount' },
  { value: 'urgency', label: 'Sort by urgency' },
  { value: 'arrival', label: 'Sort by arrival time' },
  { value: 'age', label: 'Sort by age' },
  { value: 'waiting', label: 'Sort by waiting duration' },
]
const SORT_ALGOS: { value: SortAlgorithm; label: string }[] = [
  { value: 'merge', label: 'Merge Sort' },
  { value: 'quick', label: 'Quick Sort' },
  { value: 'heap', label: 'Heap Sort' },
]

export function Analytics() {
  const [tab, setTab] = useState<Tab>('Patient Analytics')
  return <>
    <PageHeader title="Analytics & Audit" subtitle="Explore hospital activity, patient trends and system performance."/>
    <div className="analytics-tabs">{TABS.map(t => <button key={t} className={tab === t ? 'active' : ''} onClick={() => setTab(t)}>{t}</button>)}</div>
    {tab === 'Patient Analytics' && <PatientAnalytics/>}
    {tab === 'Reporting' && <Reporting/>}
    {tab === 'Audit Trail' && <AuditTrail/>}
    {tab === 'Algorithm Performance' && <AlgorithmPerformance/>}
  </>
}

function PatientAnalytics() {
  const overview = useOverview()
  const patients = usePatients()
  return <Async q={overview}>{o => {
    const by = o.byState
    const underTreatment = by.UNDER_TREATMENT ?? 0
    const waiting = (by.TRIAGED_WAITING ?? 0) + (by.APPOINTMENT_SCHEDULED ?? 0) + (by.REGISTERED ?? 0)
    const moving = (by.DISPATCH_PENDING ?? 0) + (by.IN_TRANSIT ?? 0)
    const other = (by.DISCHARGED ?? 0) + (by.CANCELLED ?? 0)
    const waits = (patients.data?.patients ?? []).map(p => p.waitingSeconds).filter((w): w is number => w !== null)
    const avgWait = waits.length ? Math.round(waits.reduce((a, b) => a + b, 0) / waits.length) : null
    const max = Math.max(1, ...o.urgencyDistribution.map(u => u.count))
    const pctTreat = o.totalPatients ? Math.round((underTreatment / o.totalPatients) * 100) : 0
    return <>
      <div className="stats-grid compact-stats">
        <Stat label="Registered Patients" value={String(o.totalPatients)} note="All patient records" icon="users"/>
        <Stat label="Under Treatment" value={String(underTreatment)} note={`${pctTreat}% of registered patients`} icon="pulse"/>
        <Stat label="Avg. Wait (sim)" value={avgWait === null ? '—' : formatDuration(avgWait)} note={`${waits.length} waiting patient${waits.length === 1 ? '' : 's'}`} icon="clock" tone="green"/>
        <Stat label="Discharged" value={String(by.DISCHARGED ?? 0)} note="Completed care" icon="check"/>
      </div>
      <div className="analytics-grid">
        <Card><SectionTitle title="Urgency distribution" link="All patients"/><div className="bars">{URGENCY_LEVELS.map((u, i) => { const c = o.urgencyDistribution[i]?.count ?? 0; return <div key={u.n}><span>{u.name}</span><i><b className={`u${u.n}`} style={{ width: `${(c / max) * 100}%` }}/></i><strong>{c}</strong></div> })}</div></Card>
        <Card><SectionTitle title="Patient status" link="Live"/><div className="donut-wrap large"><div className="donut status-donut" style={{ background: conicFromCounts([underTreatment, waiting, moving, other], ['var(--teal)', 'var(--orange)', 'var(--blue)', '#a8b5bb']) }}><span><b>{o.totalPatients}</b>Total</span></div><div className="legend">
          <div><i className="dot teal"/><span>Under treatment</span><b>{underTreatment}</b></div><div><i className="dot orange"/><span>Waiting</span><b>{waiting}</b></div><div><i className="dot blue"/><span>Dispatch / transport</span><b>{moving}</b></div><div><i className="dot gray"/><span>Discharged / cancelled</span><b>{other}</b></div></div></div></Card>
      </div>
    </>
  }}</Async>
}

function Reporting() {
  const [field, setField] = useState<SortField>('billing')
  const [algo, setAlgo] = useState<SortAlgorithm>('merge')
  const q = useSortedPatients(field, algo)
  return <Card className="table-card"><div className="section-head"><div><h2>Patient reporting</h2><p className="muted">Sorted by the project’s own Merge Sort, Quick Sort or Heap Sort implementations.</p></div>
    <div className="filters"><select aria-label="Sort algorithm" value={algo} onChange={e => setAlgo(e.target.value as SortAlgorithm)}>{SORT_ALGOS.map(a => <option key={a.value} value={a.value}>{a.label}</option>)}</select>
      <select aria-label="Sort field" value={field} onChange={e => setField(e.target.value as SortField)}>{SORT_FIELDS.map(f => <option key={f.value} value={f.value}>{f.label}</option>)}</select></div></div>
    <Async q={q}>{d => <>
      <div className="info-grid metrics"><div className="info"><span>Algorithm</span><b>{d.metrics.algorithm}</b></div><div className="info"><span>Comparisons</span><b>{d.metrics.comparisons}</b></div><div className="info"><span>Swaps / moves</span><b>{d.metrics.swaps} / {d.metrics.moves}</b></div><div className="info"><span>Stable · time</span><b>{d.metrics.stable ? 'Stable' : 'Not stable'} · {d.metrics.timeAverage}</b></div></div>
      {d.patients.length === 0 ? <Empty title="No patients to report"/> : <div className="table-scroll"><table><thead><tr><th>Patient</th><th>Age</th><th>Urgency</th><th>Arrival sequence</th><th>Wait (sim)</th><th>Billing amount</th></tr></thead><tbody>
        {d.patients.map(p => <tr key={p.id}><td><b>{p.name}</b><span className="sub">{p.id} · {stateLabel(p.state)}</span></td><td>{p.age}</td><td><Badge level={p.urgency}/></td><td>{seqLabel(p.arrivalSequence)}</td><td>{formatDuration(p.waitingSeconds)}</td><td><b>${p.billing.toLocaleString()}</b></td></tr>)}
      </tbody></table></div>}
    </>}</Async>
  </Card>
}

function AuditTrail() {
  const q = useAudit()
  return <Async q={q}>{d => {
    const events = [...d.events].reverse()
    return <Card><SectionTitle title={`Chronological audit trail (${d.returned} of ${d.total} events, newest first)`} link="Export CSV" onLink={() => downloadCsv('audit-trail.csv', [
      ['index', 'simulated_seconds', 'patient_id', 'patient_name', 'action', 'prior_state', 'new_state', 'urgency', 'location', 'resource', 'details'],
      ...d.events.map(e => [e.index, e.timestampSeconds, e.patientId, e.patientName, e.action, e.priorState, e.newState, e.urgency, e.locationName, e.resourceId, e.details])])}/>
      {events.length === 0 ? <Empty title="No audit events yet"/> : <div className="audit-list">{events.map(e => <div key={e.index}><span className="audit-time">t={e.timestampSeconds}s</span><i className={e.action.startsWith('REJECTED') ? 'rejected' : ''}/><div><b>{actionLabel(e.action)}</b><p>{e.patientId ? `${e.patientId} · ${e.patientName}` : 'Campus network'} — {e.details}</p></div><span>{e.priorState !== e.newState ? `${stateLabel(e.priorState)} → ${stateLabel(e.newState)}` : (e.resourceId || stateLabel(e.newState))}</span></div>)}</div>}
    </Card>
  }}</Async>
}

function AlgorithmPerformance() {
  const q = useAlgorithmComparison()
  return <Async q={q}>{d => {
    const rows = [
      ...d.sorts.map(s => ({ name: s.algorithm, value: s.durationMicroseconds, note: `${s.comparisons} comparisons` })),
      ...d.graph.map(g => ({ name: g.algorithm, value: g.averageMicroseconds, note: `avg of ${g.iterations} runs` })),
    ]
    const max = Math.max(1, ...rows.map(r => r.value))
    const fmt = (v: number) => (v < 1 ? '<1' : v.toFixed(v < 10 ? 1 : 0))
    return <div className="analytics-grid"><Card><SectionTitle title="Measured execution time" link={`n = ${d.patientCount} patients · sorted by ${d.sortBy}`}/>
      <div className="perf-chart">{rows.map(r => <div key={r.name} title={r.note}><span>{r.name}</span><i><b style={{ width: `${Math.max(3, (r.value / max) * 100)}%` }}/></i><strong>{fmt(r.value)}μs</strong></div>)}</div>
      <p className="muted perf-note">Sorting rows are a single run on the current patients; graph rows are the average of repeated runs on the live campus graph. Small inputs run in microseconds, so sort timings are resolution-limited.</p></Card>
      <Card><SectionTitle title="Complexity & operation counts" link="From the C++ implementation"/><div className="complexity"><div><b>Algorithm</b><b>Average time · operations</b></div>
        {d.sorts.map(s => <div key={s.algorithm}><span>{s.algorithm}</span><code>{s.timeAverage} · {s.comparisons} cmp / {s.swaps} swaps / {s.moves} moves</code></div>)}
        {d.graph.map(g => <div key={g.algorithm}><span>{g.algorithm}</span><code>{g.complexity}</code></div>)}</div></Card></div>
  }}</Async>
}
