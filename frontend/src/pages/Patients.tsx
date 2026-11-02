import { useEffect, useState } from 'react'
import { Async, Badge, Button, Card, Empty, Icon, PageHeader, Stat } from '../components/ui'
import { useOverview } from '../hooks/useAnalytics'
import { usePatients } from '../hooks/usePatients'
import { initials, statusLabel, STATE_FILTERS } from '../lib/format'

export function PatientsPage({ openModal, openPatient }: { openModal: () => void; openPatient: (id: string) => void }) {
  const [query, setQuery] = useState('')
  const [debounced, setDebounced] = useState('')
  const [urgency, setUrgency] = useState('')
  const [state, setState] = useState('')

  useEffect(() => {
    const t = window.setTimeout(() => setDebounced(query.trim()), 250)
    return () => window.clearTimeout(t)
  }, [query])

  const overview = useOverview()
  const patients = usePatients({ q: debounced, urgency: urgency ? Number(urgency) : undefined, state })
  const by = overview.data?.byState
  const waiting = by ? (by.TRIAGED_WAITING ?? 0) + (by.APPOINTMENT_SCHEDULED ?? 0) + (by.REGISTERED ?? 0) : undefined
  const show = (n: number | undefined) => (n === undefined ? '…' : String(n))

  return <>
    <PageHeader title="Patient Management" subtitle="Register, search and monitor hospital patients." action={<Button onClick={openModal}><Icon name="plus"/> Register New Patient</Button>}/>
    <div className="stats-grid compact-stats">
      <Stat label="Total Patients" value={show(overview.data?.totalPatients)} note="All registered patients" icon="users"/>
      <Stat label="Waiting" value={show(waiting)} note="Registered, triage or appointment queue" icon="clock" tone="orange"/>
      <Stat label="Under Treatment" value={show(by?.UNDER_TREATMENT ?? (by ? 0 : undefined))} note="Active care pathways" icon="pulse"/>
      <Stat label="Discharged" value={show(by?.DISCHARGED ?? (by ? 0 : undefined))} note="Completed care" icon="check" tone="green"/>
    </div>
    <Card className="table-card">
      <div className="table-tools"><div className="searchbox"><Icon name="search"/><input placeholder="Search patient ID or name..." value={query} onChange={e => setQuery(e.target.value)}/></div><div className="filters"><Icon name="filter"/>
        <select aria-label="Urgency filter" value={urgency} onChange={e => setUrgency(e.target.value)}><option value="">All urgency</option>{[1, 2, 3, 4, 5].map(n => <option key={n} value={n}>Level {n}</option>)}</select>
        <select aria-label="Status filter" value={state} onChange={e => setState(e.target.value)}>{STATE_FILTERS.map(s => <option key={s.value} value={s.value}>{s.label}</option>)}</select></div></div>
      <Async q={patients}>{d => d.patients.length === 0
        ? <Empty title="No patients match" hint={d.total === 0 ? 'No patients are registered yet.' : 'Try a different search or clear the filters.'}/>
        : <>
          <div className="table-scroll"><table><thead><tr><th>Patient</th><th>Age</th><th>Chief complaint</th><th>Urgency</th><th>Current location</th><th>Status</th><th></th></tr></thead><tbody>
            {d.patients.map(p => <tr key={p.id}><td><div className="person"><div className="avatar">{initials(p.name)}</div><div><b>{p.name}</b><span>{p.id}</span></div></div></td><td>{p.age}</td><td><b>{p.complaint}</b><span className="sub">→ {p.targetName}</span></td><td><Badge level={p.urgency}/></td><td>{p.locationName}</td><td><span className="status"><i/>{statusLabel(p)}</span></td><td><button className="more" onClick={() => openPatient(p.id)}>View details</button></td></tr>)}
          </tbody></table></div>
          <div className="table-footer">Showing {d.matched} of {d.total} patients</div>
        </>}</Async>
    </Card>
  </>
}
