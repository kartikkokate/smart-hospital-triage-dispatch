import { callNextAppointment, cancelAppointment } from '../api/appointments'
import { Async, Button, Card, Empty, Icon, PageHeader, SectionTitle, Stat } from '../components/ui'
import { useAction } from '../hooks/useAction'
import { useAppointments } from '../hooks/useAppointments'
import { initials, seqLabel } from '../lib/format'
import type { Patient } from '../types/patient'

const pad2 = (n: number) => String(n).padStart(2, '0')

export function Appointments({ openSchedule, openConvert }: { openSchedule: () => void; openConvert: (p: Patient) => void }) {
  const q = useAppointments()
  const { run, busy } = useAction()

  return <>
    <PageHeader title="Appointment Management" subtitle="Manage outpatient consultation schedules. Patients are served strictly first-in, first-out." action={<Button onClick={openSchedule}><Icon name="plus"/> Schedule Appointment</Button>}/>
    <Async q={q}>{d => {
      const s = d.stats
      const cancelledPct = s.scheduledTotal === 0 ? 0 : Math.round((s.cancelled / s.scheduledTotal) * 100)
      const preview = d.queue.slice(0, 7)
      return <>
        <div className="stats-grid compact-stats">
          <Stat label="Total Scheduled" value={pad2(s.scheduledTotal)} note="Since the system started" icon="calendar"/>
          <Stat label="Waiting" value={pad2(d.size)} note="In the FIFO queue" icon="clock" tone="orange"/>
          <Stat label="Called" value={pad2(s.called)} note="Sent to consultation" icon="check" tone="green"/>
          <Stat label="Cancelled" value={pad2(s.cancelled)} note={`${cancelledPct}% of scheduled · ${s.convertedToEmergency} escalated`} icon="x"/>
        </div>
        <Card className="fifo-card"><div><span className="eyebrow">FIFO processing</span><h2>Consultation queue</h2><p>Patients are called in the order appointments were scheduled.</p></div>
          {d.queue.length === 0 ? <Empty title="Queue is empty" hint="Schedule an appointment for a registered patient."/> : <div className="fifo-line">{preview.map((r, i) => <div key={r.patient.id} className={i === 0 ? 'next' : ''}><span>#{r.position}</span><b>{r.patient.name.split(' ')[0]}</b><small>{r.patient.id}</small></div>)}{d.queue.length > preview.length && <div><span>+{d.queue.length - preview.length}</span><b>more</b></div>}<Icon name="arrow"/></div>}
          <Button disabled={busy || d.size === 0} onClick={() => run(callNextAppointment, r => `${r.patient.name} (${r.patient.id}) called for consultation.`)}>Call Next Patient</Button></Card>
        <Card className="table-card"><SectionTitle title="Appointment queue" link="First in, first out"/>
          {d.queue.length === 0 ? <Empty title="No scheduled appointments"/> : <div className="table-scroll"><table><thead><tr><th>Queue</th><th>Patient</th><th>Complaint</th><th>Arrival</th><th>Status</th><th></th></tr></thead><tbody>
            {d.queue.map((r, i) => <tr key={r.patient.id}><td><b className="rank">#{r.position}</b></td><td><div className="person"><div className="avatar">{initials(r.patient.name)}</div><div><b>{r.patient.name}</b><span>{r.patient.id}</span></div></div></td><td>{r.patient.complaint}</td><td>{seqLabel(r.patient.arrivalSequence)}</td><td><span className="status"><i/>{i === 0 ? 'Next to be called' : 'Waiting'}</span></td>
              <td className="row-actions"><button className="more" disabled={busy} onClick={() => run(() => cancelAppointment(r.patient.id), `Appointment for ${r.patient.id} cancelled.`)}>Cancel</button><button className="more" disabled={busy} onClick={() => openConvert(r.patient)}>Escalate to emergency</button></td></tr>)}
          </tbody></table></div>}
        </Card>
      </>
    }}</Async>
  </>
}
