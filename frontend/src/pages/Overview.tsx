import { Async, Badge, Button, Card, Empty, Icon, Info, PageHeader, Progress, SectionTitle, Stat } from '../components/ui'
import { useOverview } from '../hooks/useAnalytics'
import { useTriage } from '../hooks/useTriage'
import { actionIcon, actionLabel, conicFromCounts, formatDuration, greeting, initials, seqLabel, statusLabel, URGENCY_LEVELS } from '../lib/format'
import type { Page } from '../App'

const pct = (n: number, total: number) => (total === 0 ? 0 : Math.round((n / total) * 100))
const pad2 = (n: number) => String(n).padStart(2, '0')

export function Overview({ go }: { go: (p: Page) => void }) {
  const overview = useOverview()
  const triage = useTriage()
  return <>
    <PageHeader title={`${greeting()}, Hospital Operations`} subtitle="Monitor patient flow, emergency priorities and transport operations." />
    <Async q={overview}>{o => {
      const top = o.highestPriority
      const level1 = o.urgencyDistribution[0]?.count ?? 0
      const f = o.fleet
      return <>
        <div className="stats-grid">
          <Stat label="Total Patients" value={String(o.totalPatients)} note={`${o.byState.DISCHARGED ?? 0} discharged · ${o.byState.CANCELLED ?? 0} cancelled`} icon="users"/>
          <Stat label="Emergency Triage" value={pad2(o.triageQueueSize)} note={`${level1} resuscitation (L1) patients in total`} icon="pulse" tone="red"/>
          <Stat label="Active Dispatches" value={pad2(o.activeDispatches)} note={`${f.enRoutePickup} pickups · ${f.transporting} transporting`} icon="route" tone="orange"/>
          <Stat label="Available Resources" value={pad2(f.available)} note={`of ${f.total} total resources`} icon="truck" tone="green"/>
        </div>
        <div className="overview-grid">
          <Card className="priority-card">
            <div className="section-head"><div><span className="eyebrow">Emergency overview</span><h2>Highest-priority patient</h2></div>{top && <Badge level={top.urgency}/>}</div>
            {top ? <>
              <div className="patient-hero"><div className="avatar red">{initials(top.name)}</div><div><h3>{top.name}</h3><p>{top.id} · {top.complaint}</p></div></div>
              <div className="info-grid"><Info label="Arrival sequence" value={seqLabel(top.arrivalSequence)}/><Info label="Location" value={top.locationName}/><Info label="Wait time (sim)" value={formatDuration(top.waitingSeconds)}/><Info label="Status" value={statusLabel(top)}/></div>
            </> : <Empty title="No patients waiting in triage" hint="The emergency queue is empty."/>}
            <Button onClick={() => go('Emergency Triage')}>Open triage queue <Icon name="arrow"/></Button>
          </Card>
          <Card>
            <div className="section-head"><div><span className="eyebrow">Live capacity</span><h2>Resource availability</h2></div><span className="live"><i/> Live</span></div>
            <div className="resource-summary"><div><strong>{f.total}</strong><span>Total fleet</span></div><div><strong>{f.available}</strong><span>Available now</span></div></div>
            <div className="progress-stack"><Progress label="Available" value={pct(f.available, f.total)} count={String(f.available)} color="teal"/><Progress label="En-route pickup" value={pct(f.enRoutePickup, f.total)} count={String(f.enRoutePickup)} color="blue"/><Progress label="Transporting" value={pct(f.transporting, f.total)} count={String(f.transporting)} color="orange"/></div>
            <button className="text-link" onClick={() => go('Route Dispatch')}>Manage dispatch operations <Icon name="arrow"/></button>
          </Card>
        </div>
        <div className="dashboard-lower">
          <Card><SectionTitle title="Recent activity" link="View all" onLink={() => go('Analytics & Audit')}/><div className="activity-list">
            {o.recentActivity.length === 0 && <Empty title="No activity yet"/>}
            {o.recentActivity.slice(0, 4).map((e, i) => <div className="activity" key={e.index}><span className={`activity-icon a${i}`}><Icon name={actionIcon(e.action)} size={16}/></span><div><b>{e.patientName || 'Campus network'}</b><p>{actionLabel(e.action)}</p></div><time>t={e.timestampSeconds}s</time></div>)}
          </div></Card>
          <Card><SectionTitle title="Emergency queue preview" link="Full queue" onLink={() => go('Emergency Triage')}/>
            <Async q={triage}>{t => <div className="queue-list">
              {t.queue.length === 0 && <Empty title="Queue is empty"/>}
              {t.queue.slice(0, 4).map(r => <div className="queue-row" key={r.patient.id}><b className="rank">{pad2(r.rank)}</b><div><strong>{r.patient.name}</strong><span>{r.patient.id} · {r.patient.complaint}</span></div><Badge level={r.patient.urgency}/><time>{formatDuration(r.patient.waitingSeconds)}</time></div>)}
            </div>}</Async>
          </Card>
          <Card><SectionTitle title="Patients by urgency" link="All patients"/><div className="donut-wrap"><div className="donut" style={{ background: conicFromCounts(o.urgencyDistribution.map(u => u.count)) }}><span><b>{o.totalPatients}</b>Total</span></div><div className="legend">{URGENCY_LEVELS.map((u, i) => <div key={u.n}><i className={`dot u${u.n}`}/><span>{u.name}</span><b>{o.urgencyDistribution[i]?.count ?? 0}</b></div>)}</div></div></Card>
        </div>
      </>
    }}</Async>
  </>
}
