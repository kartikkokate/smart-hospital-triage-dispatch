import type { ReactNode } from 'react'
import { URGENCY_LEVELS } from '../lib/format'
import type { AsyncState } from '../hooks/useAsync'

const paths: Record<string, ReactNode> = {
  grid: <><rect x="3" y="3" width="7" height="7" rx="1"/><rect x="14" y="3" width="7" height="7" rx="1"/><rect x="3" y="14" width="7" height="7" rx="1"/><rect x="14" y="14" width="7" height="7" rx="1"/></>,
  users: <><path d="M16 21v-2a4 4 0 0 0-4-4H6a4 4 0 0 0-4 4v2"/><circle cx="9" cy="7" r="4"/><path d="M22 21v-2a4 4 0 0 0-3-3.87M16 3.13a4 4 0 0 1 0 7.75"/></>,
  pulse: <path d="M3 12h4l2-7 4 14 2-7h6"/>, route: <><circle cx="6" cy="19" r="3"/><circle cx="18" cy="5" r="3"/><path d="M6 16V9a4 4 0 0 1 4-4h5M9 19h5a4 4 0 0 0 4-4V8"/></>,
  map: <><polygon points="3 6 9 3 15 6 21 3 21 18 15 21 9 18 3 21 3 6"/><path d="M9 3v15M15 6v15"/></>,
  calendar: <><rect x="3" y="4" width="18" height="17" rx="2"/><path d="M16 2v4M8 2v4M3 10h18"/></>,
  chart: <><path d="M3 3v18h18"/><path d="M7 16v-5M12 16V8M17 16V5"/></>,
  search: <><circle cx="11" cy="11" r="7"/><path d="m20 20-4-4"/></>, bell: <><path d="M18 8a6 6 0 0 0-12 0c0 7-3 7-3 9h18c0-2-3-2-3-9"/><path d="M10 21h4"/></>,
  plus: <><path d="M12 5v14M5 12h14"/></>, arrow: <><path d="m9 18 6-6-6-6"/></>, clock: <><circle cx="12" cy="12" r="9"/><path d="M12 7v5l3 2"/></>,
  check: <path d="m5 12 4 4L19 6"/>, truck: <><rect x="2" y="7" width="12" height="10" rx="2"/><path d="M14 10h4l4 4v3h-8"/><circle cx="6" cy="18" r="2"/><circle cx="18" cy="18" r="2"/></>,
  filter: <path d="M4 5h16M7 12h10M10 19h4"/>, x: <><path d="m6 6 12 12M18 6 6 18"/></>, settings: <><circle cx="12" cy="12" r="3"/><path d="M19.4 15a1.7 1.7 0 0 0 .3 1.9l.1.1-2.8 2.8-.1-.1a1.7 1.7 0 0 0-1.9-.3 1.7 1.7 0 0 0-1 1.6v.2h-4V21a1.7 1.7 0 0 0-1-1.6 1.7 1.7 0 0 0-1.9.3l-.1.1L4.2 17l.1-.1a1.7 1.7 0 0 0 .3-1.9A1.7 1.7 0 0 0 3 14H3v-4h.1a1.7 1.7 0 0 0 1.6-1 1.7 1.7 0 0 0-.3-1.9L4.2 7 7 4.2l.1.1A1.7 1.7 0 0 0 9 4.6a1.7 1.7 0 0 0 1-1.6V3h4v.1a1.7 1.7 0 0 0 1 1.6 1.7 1.7 0 0 0 1.9-.3l.1-.1L19.8 7l-.1.1a1.7 1.7 0 0 0-.3 1.9 1.7 1.7 0 0 0 1.6 1h.2v4H21a1.7 1.7 0 0 0-1.6 1Z"/></>,
}

export function Icon({ name, size = 18 }: { name: string; size?: number }) {
  return <svg width={size} height={size} viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.8" strokeLinecap="round" strokeLinejoin="round" aria-hidden>{paths[name]}</svg>
}

export function Badge({ level, children }: { level?: number; children?: ReactNode }) {
  const u = level ? URGENCY_LEVELS[level - 1] : null
  return <span className={level ? `badge u${level}` : 'badge neutral'}>{u ? `L${level} · ${u.name}` : children}</span>
}

export function Card({ children, className = '' }: { children: ReactNode; className?: string }) {
  return <section className={`card ${className}`}>{children}</section>
}

export function Stat({ label, value, note, icon, tone = '' }: { label: string; value: string; note: string; icon: string; tone?: string }) {
  return <Card className="stat"><div className={`stat-icon ${tone}`}><Icon name={icon}/></div><div><p>{label}</p><strong>{value}</strong><small>{note}</small></div></Card>
}

export function PageHeader({ title, subtitle, action }: { title: string; subtitle: string; action?: ReactNode }) {
  return <div className="page-header"><div><h1>{title}</h1><p>{subtitle}</p></div>{action}</div>
}

export function Button({ children, secondary = false, danger = false, onClick, disabled = false, type = 'button' }: {
  children: ReactNode; secondary?: boolean; danger?: boolean; onClick?: () => void; disabled?: boolean; type?: 'button' | 'submit'
}) {
  return <button type={type} disabled={disabled} className={`btn ${secondary ? 'secondary' : ''} ${danger ? 'danger' : ''}`} onClick={onClick}>{children}</button>
}

export function SectionTitle({ title, link, onLink }: { title: string; link: string; onLink?: () => void }) {
  return <div className="section-head compact"><h2>{title}</h2><button className="text-link" onClick={onLink}>{link} <Icon name="arrow" size={14}/></button></div>
}

export function Info({ label, value }: { label: string; value: string }) {
  return <div className="info"><span>{label}</span><b>{value}</b></div>
}

export function Progress({ label, value, count, color }: { label: string; value: number; count: string; color: string }) {
  return <div><div className="progress-label"><span><i className={`dot ${color}`}/>{label}</span><b>{count}</b></div><div className="progress"><i className={color} style={{ width: `${value}%` }}/></div></div>
}

// ---- Loading / error / empty states -------------------------------------------------------------

export function Loading({ label = 'Loading from the hospital backend…' }: { label?: string }) {
  return <div className="state-view" role="status"><i className="spinner"/><span>{label}</span></div>
}

export function ErrorState({ message, onRetry }: { message: string; onRetry?: () => void }) {
  return <div className="state-view error" role="alert"><Icon name="x" size={20}/><div><b>Could not load data</b><span>{message}</span></div>{onRetry && <Button secondary onClick={onRetry}>Retry</Button>}</div>
}

export function Empty({ title, hint, action }: { title: string; hint?: string; action?: ReactNode }) {
  return <div className="state-view empty"><div><b>{title}</b>{hint && <span>{hint}</span>}</div>{action}</div>
}

/** Renders loading and error states for a query; `children` only ever receives real backend data. */
export function Async<T>({ q, children }: { q: AsyncState<T>; children: (data: T) => ReactNode }) {
  if (q.error) return <ErrorState message={q.error} onRetry={q.reload}/>
  if (q.data === undefined) return <Loading/>
  return <>{children(q.data)}</>
}
