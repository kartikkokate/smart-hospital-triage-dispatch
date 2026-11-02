import { useEffect, useState, type ReactNode } from 'react'
import { scheduleAppointment, convertAppointment } from '../api/appointments'
import { listNodes } from '../api/campus'
import { errorMessage } from '../api/client'
import { dischargePatient, listPatients, registerPatient, updatePatientUrgency } from '../api/patients'
import { admitToTriage } from '../api/triage'
import { useApp } from '../hooks/AppContext'
import { useAction } from '../hooks/useAction'
import { useAsync } from '../hooks/useAsync'
import { usePatientDetail, usePatients } from '../hooks/usePatients'
import { actionLabel, initials, statusLabel, URGENCY_LEVELS } from '../lib/format'
import type { NewPatientInput, Patient } from '../types/patient'
import { Async, Badge, Button, Empty, Icon, Info } from './ui'

function ModalShell({ eyebrow, title, close, children, actions }: { eyebrow: string; title: string; close: () => void; children: ReactNode; actions: ReactNode }) {
  useEffect(() => {
    const onKey = (e: KeyboardEvent) => e.key === 'Escape' && close()
    window.addEventListener('keydown', onKey)
    return () => window.removeEventListener('keydown', onKey)
  }, [close])
  return <div className="modal-backdrop" onMouseDown={e => e.target === e.currentTarget && close()}><div className="modal" role="dialog" aria-modal="true" aria-label={title}>
    <div className="modal-head"><div><span className="eyebrow">{eyebrow}</span><h2>{title}</h2></div><button className="icon-btn" onClick={close} aria-label="Close"><Icon name="x"/></button></div>
    {children}
    <div className="modal-actions">{actions}</div>
  </div></div>
}

const urgencyOptions = (max = 5) => URGENCY_LEVELS.filter(u => u.n <= max).map(u => <option key={u.n} value={u.n}>Level {u.n} — {u.name}</option>)

// ---- Register ---------------------------------------------------------------------------------------

type Pathway = 'triage' | 'appointment' | 'none'

export function RegisterModal({ close }: { close: () => void }) {
  const { notify, refresh } = useApp()
  const nodes = useAsync(() => listNodes(), [])
  const existing = useAsync(() => listPatients(), [])

  const [f, setF] = useState({ id: '', name: '', age: '', complaint: '', urgency: '1', loc: '0', target: '2', billing: '0', pathway: 'triage' as Pathway })
  const [errors, setErrors] = useState<Record<string, string>>({})
  const [busy, setBusy] = useState(false)
  const set = (k: keyof typeof f, v: string) => setF(s => ({ ...s, [k]: v }))

  // Suggest the next free P-number once the directory is known (editable by the user).
  const [suggested, setSuggested] = useState(false)
  useEffect(() => {
    if (suggested || !existing.data) return
    const nums = existing.data.patients.map(p => /^P(\d+)$/.exec(p.id)).filter((m): m is RegExpExecArray => !!m).map(m => Number(m[1]))
    setF(s => ({ ...s, id: `P${nums.length ? Math.max(...nums) + 1 : 101}` }))
    setSuggested(true)
  }, [existing.data, suggested])

  const validate = () => {
    const e: Record<string, string> = {}
    const id = f.id.trim(), name = f.name.trim(), complaint = f.complaint.trim()
    if (!id) e.id = 'Patient ID is required.'
    else if (id.length > 16) e.id = 'At most 16 characters.'
    else if (/[\s,]/.test(id)) e.id = 'No spaces or commas.'
    else if (existing.data?.patients.some(p => p.id === id)) e.id = 'This ID is already registered.'
    if (!name) e.name = 'Full name is required.'
    else if (name.length > 64) e.name = 'At most 64 characters.'
    else if (!/^[A-Za-z' -]+$/.test(name)) e.name = "Letters, spaces, hyphens and apostrophes only."
    if (f.age.trim() === '' || !/^\d+$/.test(f.age.trim()) || Number(f.age) > 150) e.age = 'Whole number from 0 to 150.'
    if (!complaint) e.complaint = 'Chief complaint is required.'
    else if (complaint.length > 128) e.complaint = 'At most 128 characters.'
    const billing = Number(f.billing)
    if (f.billing.trim() === '' || !Number.isFinite(billing) || billing < 0 || billing > 1000000) e.billing = 'Between 0 and 1,000,000.'
    return e
  }

  const submit = async () => {
    const errs = validate()
    setErrors(errs)
    if (Object.keys(errs).length) return
    const input: NewPatientInput = {
      id: f.id.trim(), name: f.name.trim(), age: Number(f.age), complaint: f.complaint.trim(),
      urgency: Number(f.urgency), locationNode: Number(f.loc), targetNode: Number(f.target), billing: Number(f.billing),
    }
    setBusy(true)
    try {
      await registerPatient(input)
    } catch (err) {
      notify('error', errorMessage(err))
      setBusy(false)
      return
    }
    refresh()
    try {
      if (f.pathway === 'triage') {
        const r = await admitToTriage(input.id, input.urgency)
        notify('success', `${input.id} registered and admitted to emergency triage (queue rank #${r.rank}).`)
      } else if (f.pathway === 'appointment') {
        await scheduleAppointment(input.id)
        notify('success', `${input.id} registered and added to the appointment queue.`)
      } else {
        notify('success', `${input.id} registered.`)
      }
    } catch (err) {
      notify('error', `${input.id} was registered, but the next step failed: ${errorMessage(err)}`)
    }
    refresh()
    setBusy(false)
    close()
  }

  const field = (k: string) => errors[k] ? <span className="field-error">{errors[k]}</span> : null
  const cls = (k: string) => errors[k] ? 'invalid' : ''

  return <ModalShell eyebrow="New record" title="Register New Patient" close={close} actions={<><Button secondary onClick={close}>Cancel</Button><Button onClick={submit} disabled={busy || !nodes.data}>{busy ? 'Registering…' : 'Register Patient'}</Button></>}>
    {nodes.error ? <div className="modal-pad"><div className="state-view error" role="alert"><div><b>Could not load campus locations</b><span>{nodes.error}</span></div><Button secondary onClick={nodes.reload}>Retry</Button></div></div> :
    <div className="form-grid">
      <label>Patient ID<input className={cls('id')} value={f.id} onChange={e => set('id', e.target.value)} maxLength={16}/>{field('id')}</label>
      <label>Full Name<input className={cls('name')} placeholder="Enter patient name" value={f.name} onChange={e => set('name', e.target.value)}/>{field('name')}</label>
      <label>Age<input className={cls('age')} type="number" min={0} max={150} placeholder="Age" value={f.age} onChange={e => set('age', e.target.value)}/>{field('age')}</label>
      <label>Urgency Level<select value={f.urgency} onChange={e => set('urgency', e.target.value)}>{urgencyOptions()}</select></label>
      <label className="wide">Chief Medical Complaint<textarea className={cls('complaint')} placeholder="Describe primary complaint" value={f.complaint} onChange={e => set('complaint', e.target.value)}/>{field('complaint')}</label>
      <label>Current Campus Location<select value={f.loc} onChange={e => set('loc', e.target.value)}>{nodes.data?.nodes.map(n => <option key={n.id} value={n.id}>{n.name}</option>)}</select></label>
      <label>Target Treatment Location<select value={f.target} onChange={e => set('target', e.target.value)}>{nodes.data?.nodes.map(n => <option key={n.id} value={n.id}>{n.name}</option>)}</select></label>
      <label>Initial Billing Amount<input className={cls('billing')} type="number" min={0} step="0.01" placeholder="$0.00" value={f.billing} onChange={e => set('billing', e.target.value)}/>{field('billing')}</label>
      <label>After registration<select value={f.pathway} onChange={e => set('pathway', e.target.value)}><option value="triage">Admit to emergency triage</option><option value="appointment">Schedule outpatient appointment</option><option value="none">Register only</option></select></label>
    </div>}
  </ModalShell>
}

// ---- Patient profile & actions ----------------------------------------------------------------------

export function PatientModal({ id, close }: { id: string; close: () => void }) {
  const detail = usePatientDetail(id)
  const { run, busy } = useAction()
  const [urgency, setUrgency] = useState<string | null>(null)

  return <ModalShell eyebrow="Patient profile" title={detail.data?.patient.name ?? id} close={close} actions={<Button onClick={close}>Close Profile</Button>}>
    <Async q={detail}>{d => {
      if (!d) return null
      const p: Patient = d.patient
      const terminal = p.state === 'DISCHARGED' || p.state === 'CANCELLED'
      const chosen = urgency ?? String(p.urgency)
      const events = d.history.slice(-5)
      return <>
        <div className="profile-banner"><div className="avatar">{initials(p.name)}</div><div><b>{p.id}</b><p>{p.age} years · {p.complaint}</p></div><Badge level={p.urgency}/></div>
        <div className="info-grid profile"><Info label="Current status" value={statusLabel(p)}/><Info label="Current location" value={p.locationName}/><Info label="Treatment destination" value={p.targetName}/><Info label="Assigned resource" value={p.assignedResourceId || 'Not assigned'}/></div>
        <h3 className="timeline-title">Patient lifecycle (from the audit log)</h3>
        {events.length === 0 ? <Empty title="No audit events recorded"/> : <div className="timeline">{events.map(e => <div className="done" key={e.index}><i><Icon name="check" size={12}/></i><span><b>{actionLabel(e.action)}</b><small>t={e.timestampSeconds}s</small></span></div>)}</div>}
        {!terminal && <div className="profile-actions">
          <label>Urgency level<select value={chosen} onChange={e => setUrgency(e.target.value)}>{urgencyOptions()}</select></label>
          <Button secondary disabled={busy || Number(chosen) === p.urgency} onClick={() => run(() => updatePatientUrgency(p.id, Number(chosen)), `${p.id} urgency set to level ${chosen}. Arrival sequence preserved.`)}>Update urgency</Button>
          {p.state === 'REGISTERED' && <>
            <Button disabled={busy} onClick={() => run(() => admitToTriage(p.id, Number(chosen)), r => `${p.id} admitted to triage (rank #${r.rank}).`)}>Admit to triage</Button>
            <Button secondary disabled={busy} onClick={() => run(() => scheduleAppointment(p.id), `${p.id} added to the appointment queue.`)}>Schedule appointment</Button>
          </>}
          {p.state === 'UNDER_TREATMENT' && <Button disabled={busy} onClick={() => run(() => dischargePatient(p.id), `${p.id} discharged.`)}>Discharge patient</Button>}
        </div>}
      </>
    }}</Async>
  </ModalShell>
}

// ---- Appointments -----------------------------------------------------------------------------------

export function ScheduleModal({ close }: { close: () => void }) {
  const registered = usePatients({ state: 'REGISTERED' })
  const { run, busy } = useAction()
  const [chosen, setChosen] = useState('')
  const list = registered.data?.patients ?? []
  const value = chosen || list[0]?.id || ''

  return <ModalShell eyebrow="Outpatient clinic" title="Schedule Appointment" close={close} actions={<><Button secondary onClick={close}>Cancel</Button>
    <Button disabled={busy || !value} onClick={async () => { const r = await run(() => scheduleAppointment(value), `${value} added to the appointment queue.`); if (r) close() }}>Schedule</Button></>}>
    <div className="modal-pad"><Async q={registered}>{d => d.patients.length === 0
      ? <Empty title="No registered patients available" hint="Only patients in the REGISTERED state can be scheduled. Register a new patient first and choose “Schedule outpatient appointment”."/>
      : <div className="form-grid single"><label className="wide">Registered patient<select value={value} onChange={e => setChosen(e.target.value)}>{d.patients.map(p => <option key={p.id} value={p.id}>{p.id} — {p.name} · {p.complaint}</option>)}</select></label></div>}</Async></div>
  </ModalShell>
}

export function ConvertModal({ patient, close }: { patient: Patient; close: () => void }) {
  const { run, busy } = useAction()
  const [urgency, setUrgency] = useState('2')
  return <ModalShell eyebrow="Condition deteriorated" title={`Convert ${patient.name} to emergency`} close={close} actions={<><Button secondary onClick={close}>Cancel</Button>
    <Button disabled={busy} onClick={async () => { const r = await run(() => convertAppointment(patient.id, Number(urgency)), `${patient.id} moved from the appointment queue to emergency triage.`); if (r) close() }}>Convert to emergency</Button></>}>
    <div className="modal-pad"><p className="muted">{patient.id} leaves the FIFO appointment queue and enters the emergency min-heap with the urgency chosen below.</p>
      <div className="form-grid single"><label className="wide">Emergency urgency<select value={urgency} onChange={e => setUrgency(e.target.value)}>{urgencyOptions()}</select></label></div></div>
  </ModalShell>
}
