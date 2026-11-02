import { selectNextForDispatch } from '../api/dispatch'
import { Async, Badge, Button, Card, Empty, Icon, Info, PageHeader, SectionTitle, Stat } from '../components/ui'
import { useAction } from '../hooks/useAction'
import { useTriage } from '../hooks/useTriage'
import { formatDuration, initials, seqLabel, statusLabel } from '../lib/format'

export function Triage({ openPatient }: { openPatient: (id: string) => void }) {
  const triage = useTriage()
  const { run, busy } = useAction()
  const extract = () => run(selectNextForDispatch, r =>
    r.source === 'TRIAGE_QUEUE'
      ? `${r.patient.id} extracted from the priority queue and moved to the dispatch pool.`
      : `${r.patient.id} was already pending dispatch with higher priority than the queue root; nothing extracted from the queue.`)

  return <>
    <PageHeader title="Emergency Triage" subtitle="Prioritize patients based on clinical urgency and arrival sequence. Ordering is computed by the C++ min-heap, not the browser." action={<Button onClick={extract} disabled={busy || (triage.data?.size === 0)}><Icon name="pulse"/> Extract Highest Priority</Button>}/>
    <Async q={triage}>{t => {
      const top = t.next
      const urgent = t.queue.filter(r => r.patient.urgency <= 2).length
      const sameLevel = top ? t.queue.filter(r => r.patient.urgency === top.urgency).length : 0
      const slots = ['n1', 'n2', 'n3', 'n4', 'n5']
      const nodes = t.heapArray.slice(0, 5)
      return <>
        <div className="stats-grid compact-stats">
          <Stat label="Patients Waiting" value={String(t.size)} note="In the emergency min-heap" icon="users"/>
          <Stat label="Highest Priority" value={top ? `Level ${top.urgency}` : '—'} note={top ? `${sameLevel} patient${sameLevel === 1 ? '' : 's'} at this level` : 'Queue is empty'} icon="pulse" tone="red"/>
          <Stat label="Urgent Cases" value={String(urgent).padStart(2, '0')} note="Levels 1–2 in queue" icon="clock" tone="orange"/>
          <Stat label="Urgency Updated" value={String(t.urgencyUpdates).padStart(2, '0')} note="Since the system started" icon="chart"/>
        </div>
        {top ? <Card className="triage-feature"><div className="severity-stripe"/><div><span className="eyebrow red-text">Immediate clinical attention</span><div className="patient-hero"><div className="avatar red">{initials(top.name)}</div><div><h2>{top.name} <small>{top.id}</small></h2><p>{top.complaint} · Arrival {seqLabel(top.arrivalSequence)}</p></div></div></div><div className="feature-data"><Info label="Priority" value="#1"/><Info label="Arrival sequence" value={String(top.arrivalSequence).padStart(4, '0')}/><Info label="Current location" value={top.locationName}/><Info label="State" value={statusLabel(top)}/></div><Badge level={top.urgency}/></Card>
          : <Card className="triage-feature"><Empty title="No patients in the triage queue" hint="Register a patient and admit them to emergency triage to see them here."/></Card>}
        <div className="triage-layout"><Card className="table-card"><SectionTitle title="Priority queue" link="Min-heap order"/>
          {t.queue.length === 0 ? <Empty title="Queue is empty"/> : <div className="table-scroll"><table><thead><tr><th>Rank</th><th>Patient</th><th>Chief complaint</th><th>Urgency</th><th>Arrival</th><th>Wait (sim)</th><th></th></tr></thead><tbody>
            {t.queue.map(r => <tr key={r.patient.id}><td><b className={r.rank === 1 ? 'rank top' : 'rank'}>#{r.rank}</b></td><td><b>{r.patient.name}</b><span className="sub">{r.patient.id}</span></td><td>{r.patient.complaint}</td><td><Badge level={r.patient.urgency}/></td><td>{seqLabel(r.patient.arrivalSequence)}</td><td>{formatDuration(r.patient.waitingSeconds)}</td><td><button className="more" onClick={() => openPatient(r.patient.id)}>Review</button></td></tr>)}
          </tbody></table></div>}
        </Card><Card className="heap-card"><span className="eyebrow">DSA policy</span><h2>How priority works</h2><p>Lower urgency number is prioritized. Equal levels are resolved by earlier arrival sequence. Showing heap array positions 0–{Math.max(nodes.length - 1, 0)} of {t.heapArray.length}.</p>
          <div className="heap">{nodes.map((n, i) => <span key={n.patientId} className={`heap-node ${slots[i]}`} title={`Heap index ${n.index} · arrival ${n.arrivalSequence}`}>{n.urgency}<small>{n.patientId}</small></span>)}
            {nodes.length > 1 && <i className="line l1"/>}{nodes.length > 2 && <i className="line l2"/>}{nodes.length > 3 && <i className="line l3"/>}{nodes.length > 4 && <i className="line l4"/>}</div>
          <div className="policy"><b>Min-Heap</b><span>O(log n) insertion & extraction</span></div></Card></div>
      </>
    }}</Async>
  </>
}
