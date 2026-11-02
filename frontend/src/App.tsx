import { useMemo, useState } from 'react'
import { ConvertModal, PatientModal, RegisterModal, ScheduleModal } from './components/Modals'
import { Icon } from './components/ui'
import { useApp } from './hooks/AppContext'
import { Analytics } from './pages/Analytics'
import { Appointments } from './pages/Appointments'
import { Campus } from './pages/Campus'
import { DispatchPage } from './pages/Dispatch'
import { Overview } from './pages/Overview'
import { PatientsPage } from './pages/Patients'
import { Triage } from './pages/Triage'
import type { Patient } from './types/patient'

export type Page = 'Overview' | 'Patients' | 'Emergency Triage' | 'Route Dispatch' | 'Campus Map' | 'Appointments' | 'Analytics & Audit'

const nav: { label: Page; icon: string }[] = [
  { label: 'Overview', icon: 'grid' }, { label: 'Patients', icon: 'users' }, { label: 'Emergency Triage', icon: 'pulse' },
  { label: 'Route Dispatch', icon: 'route' }, { label: 'Campus Map', icon: 'map' }, { label: 'Appointments', icon: 'calendar' },
  { label: 'Analytics & Audit', icon: 'chart' },
]

type ModalState =
  | { type: 'register' }
  | { type: 'patient'; id: string }
  | { type: 'schedule' }
  | { type: 'convert'; patient: Patient }
  | null

export default function App() {
  const { notices, dismiss, backend } = useApp()
  const [page, setPage] = useState<Page>('Overview')
  const [modal, setModal] = useState<ModalState>(null)
  const title = useMemo(() => page === 'Overview' ? 'Command Center' : page, [page])
  const close = () => setModal(null)
  const openPatient = (id: string) => setModal({ type: 'patient', id })

  const status = backend.online === null ? 'connecting' : backend.online ? 'online' : 'offline'

  return <div className="app-shell">
    <aside className="sidebar"><div className="brand"><div className="logo"><span>+</span><i/></div><div><b>MediRoute</b><small>Emergency Operations</small></div></div>
      <nav>{nav.map(n => <button key={n.label} className={page === n.label ? 'active' : ''} onClick={() => setPage(n.label)}><Icon name={n.icon}/><span>{n.label}</span>{page === n.label && <i/>}</button>)}</nav>
      <div className="sidebar-bottom"><div className={`system-card ${status}`}><span><i/> {status === 'online' ? 'C++ backend connected' : status === 'offline' ? 'Backend unreachable' : 'Connecting to backend…'}</span><small>{backend.health ? `Simulation clock · t=${backend.health.simulationTimeSeconds}s` : 'Hospital Operations'}</small></div><div className="side-user"><div className="avatar">DR</div><div><b>Dr. Elena Reed</b><small>Operations Lead</small></div><Icon name="settings"/></div></div></aside>
    <div className="main-shell"><header className="topbar"><div><span>Hospital Operations</span><Icon name="arrow" size={14}/><b>{title}</b></div><div className="top-actions"><button className="icon-btn" aria-label="Search"><Icon name="search"/></button><button className="icon-btn bell" aria-label="Notifications"><Icon name="bell"/><i/></button><span className={`operational ${status}`}><i/> {status === 'online' ? 'Operational' : status === 'offline' ? 'Offline' : 'Connecting'}</span><div className="avatar">DR</div></div></header>
      <main>
        {page === 'Overview' && <Overview go={setPage}/>}
        {page === 'Patients' && <PatientsPage openModal={() => setModal({ type: 'register' })} openPatient={openPatient}/>}
        {page === 'Emergency Triage' && <Triage openPatient={openPatient}/>}
        {page === 'Route Dispatch' && <DispatchPage/>}
        {page === 'Campus Map' && <Campus/>}
        {page === 'Appointments' && <Appointments openSchedule={() => setModal({ type: 'schedule' })} openConvert={patient => setModal({ type: 'convert', patient })}/>}
        {page === 'Analytics & Audit' && <Analytics/>}
      </main></div>
    {modal?.type === 'register' && <RegisterModal close={close}/>}
    {modal?.type === 'patient' && <PatientModal id={modal.id} close={close}/>}
    {modal?.type === 'schedule' && <ScheduleModal close={close}/>}
    {modal?.type === 'convert' && <ConvertModal patient={modal.patient} close={close}/>}
    <div className="toast-stack" aria-live="polite">{notices.map(n => <div key={n.id} className={`notice ${n.kind}`}><Icon name={n.kind === 'error' ? 'x' : 'check'}/>{n.text}<button onClick={() => dismiss(n.id)}>Dismiss</button></div>)}</div>
  </div>
}
