import { useState } from 'react'
import { assignResource, cancelDispatch, completeTransport, runPickup, selectNextForDispatch } from '../api/dispatch'
import { CampusDiagram } from '../components/CampusDiagram'
import { Async, Badge, Button, Card, Empty, Icon, Info, PageHeader, SectionTitle, Stat } from '../components/ui'
import { useAction } from '../hooks/useAction'
import { useCampus } from '../hooks/useCampus'
import { useDispatch } from '../hooks/useDispatch'
import { formatLongDuration, initials } from '../lib/format'
import type { Dispatch, DispatchStatus } from '../types/dispatch'

const STATUS_TEXT: Record<DispatchStatus, string> = {
  AWAITING_ASSIGNMENT: 'Awaiting assignment',
  RESOURCE_ASSIGNED: 'Resource assigned',
  IN_TRANSIT: 'Patient in transit',
}

const pad2 = (n: number) => String(n).padStart(2, '0')

export function DispatchPage() {
  const data = useDispatch()
  const campus = useCampus()
  const { run, busy } = useAction()
  const [selectedId, setSelectedId] = useState<string>('')

  return <>
    <PageHeader title="Route Dispatch" subtitle="Assign the optimal available resource and calculate the fastest campus route (Dijkstra, two legs)."/>
    <Async q={data}>{d => {
      const selected: Dispatch | undefined = d.dispatches.find(x => x.patient.id === selectedId) ?? d.dispatches[0]
      const s = d.summary
      const legEdges = (dsp?: Dispatch): [number, number][] => [dsp?.leg1, dsp?.leg2].flatMap(l => (l?.segments ?? []).map(g => [g.u, g.v] as [number, number]))
      const legNodes = (dsp?: Dispatch) => [...(dsp?.leg1?.path ?? []), ...(dsp?.leg2?.path ?? [])]
      const leg1 = selected?.leg1, leg2 = selected?.leg2
      const primary = !selected ? null
        : selected.status === 'AWAITING_ASSIGNMENT' ? { label: 'Assign Resource', act: () => run(() => assignResource(selected.patient.id), r => `${r.dispatch.resource?.id ?? 'Resource'} assigned to ${selected.patient.id}.`) }
        : selected.status === 'RESOURCE_ASSIGNED' ? { label: 'Run Pickup (Leg 1)', act: () => run(() => runPickup(selected.patient.id), `Pickup complete — ${selected.patient.id} is now in transit.`) }
        : { label: 'Complete Transport', act: () => run(() => completeTransport(selected.patient.id), `Handover complete — ${selected.patient.id} is now under treatment.`) }

      return <>
        <div className="stats-grid compact-stats">
          <Stat label="Available Resources" value={pad2(s.available)} note="Ready for assignment" icon="truck" tone="green"/>
          <Stat label="En-route Pickup" value={pad2(s.enRoutePickup)} note="Resources heading to patients" icon="route"/>
          <Stat label="Transporting" value={pad2(s.transporting)} note="Patients onboard" icon="users" tone="orange"/>
          <Stat label="Active Dispatches" value={pad2(d.dispatches.length)} note="Pending, assigned or in transit" icon="pulse"/>
        </div>
        <div className="dispatch-grid"><Card className="dispatch-list"><SectionTitle title="Pending dispatch" link={`${d.dispatches.length} patient${d.dispatches.length === 1 ? '' : 's'}`}/><p className="muted">Select a patient to see their route and resource.</p>
          {d.dispatches.length === 0 && <Empty title="No patients in dispatch" hint="Pull the highest-priority patient from the emergency triage queue to begin."/>}
          {d.dispatches.map(x => <button className={`dispatch-patient ${selected?.patient.id === x.patient.id ? 'active' : ''}`} onClick={() => setSelectedId(x.patient.id)} key={x.patient.id}><div className="avatar">{initials(x.patient.name)}</div><div><b>{x.patient.name}</b><span>{x.patient.id} · {x.patient.complaint}</span><small>{x.patient.locationName} → {x.patient.targetName} · {STATUS_TEXT[x.status]}</small></div><Badge level={x.patient.urgency}/></button>)}
          <Button disabled={busy || selected?.status !== 'AWAITING_ASSIGNMENT'} onClick={() => selected && run(() => assignResource(selected.patient.id), r => `${r.dispatch.resource?.id ?? 'Resource'} assigned to ${selected.patient.id} (total route ${r.dispatch.totalCostSeconds ?? '—'}s).`)}><Icon name="route"/> Assign Optimal Resource</Button>
          <div className="spacer"/>
          <Button secondary disabled={busy} onClick={() => run(selectNextForDispatch, r => r.source === 'TRIAGE_QUEUE' ? `${r.patient.id} pulled from the triage queue into the dispatch pool.` : `${r.patient.id} is already pending dispatch.`)}>Pull next from triage queue</Button>
        </Card>
          <Card className="map-card"><div className="section-head"><div><span className="eyebrow">Live campus topology</span><h2>Optimal route visualization</h2></div><span className="simulated">Dijkstra · simulated movement</span></div>
            <Async q={campus}>{c => <CampusDiagram nodes={c.nodes} edges={c.edges} highlightEdges={legEdges(selected)} highlightNodes={legNodes(selected)} pickupNode={selected?.pickupNode} destinationNode={selected?.destinationNode} vehicleNode={selected?.resource?.nodeId} showKey/>}</Async>
            <div className="route-legs">
              <div><span>LEG 1</span><b>{selected?.resource ? `${selected.resource.id} → ${selected.patient.locationName}` : 'Resource → pickup'}</b><small>{!selected ? 'Select a patient' : selected.status === 'IN_TRANSIT' ? 'Completed — resource reached the patient' : leg1 ? (leg1.reachable ? `Resource to pickup · ${leg1.costSeconds} sec · ${leg1.pathNames.length} stops` : 'No open route to the pickup point') : 'Assign a resource to calculate'}</small></div>
              <Icon name="arrow"/>
              <div><span>LEG 2</span><b>{selected ? `${selected.patient.locationName} → ${selected.patient.targetName}` : 'Pickup → destination'}</b><small>{!selected ? '—' : leg2 ? (leg2.reachable ? `Patient transfer · ${leg2.costSeconds} sec · ${leg2.pathNames.length} stops` : 'Destination unreachable (corridor closed)') : '—'}</small></div>
            </div></Card>
        </div>
        <Card className="dispatch-bar"><div><span className="resource-tag"><Icon name="truck"/> {selected?.resource?.id ?? 'No resource yet'}</span><b>{selected?.resource?.name ?? (selected ? 'Assigned when you press Assign' : 'Select a patient')}</b></div>
          <Info label="Total route cost" value={selected?.totalCostSeconds != null ? `${selected.totalCostSeconds} seconds` : '—'}/>
          <Info label="Estimated transit" value={formatLongDuration(selected?.totalCostSeconds)}/>
          <Info label="Dispatch status" value={selected ? STATUS_TEXT[selected.status] : '—'}/>
          <div className="button-group"><Button secondary disabled={busy || !selected || selected.status === 'IN_TRANSIT'} onClick={() => selected && run(() => cancelDispatch(selected.patient.id), `Dispatch for ${selected.patient.id} cancelled: resource released and patient marked CANCELLED.`)}>Cancel</Button><Button disabled={busy || !primary} onClick={() => primary?.act()}>{primary?.label ?? 'Assign Resource'}</Button></div></Card>
      </>
    }}</Async>
  </>
}
