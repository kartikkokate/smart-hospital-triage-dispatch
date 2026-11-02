import type { Patient, PatientState } from '../types/patient'

/** Display names for the 1–5 urgency scale defined by the C++ UrgencyLevel enum. */
export const URGENCY_LEVELS = [
  { n: 1, name: 'Resuscitation' },
  { n: 2, name: 'Emergent' },
  { n: 3, name: 'Urgent' },
  { n: 4, name: 'Less Urgent' },
  { n: 5, name: 'Non-Urgent' },
]

export function formatDuration(seconds: number | null | undefined): string {
  if (seconds === null || seconds === undefined) return '—'
  if (seconds < 60) return `${seconds}s`
  const m = Math.floor(seconds / 60)
  const s = seconds % 60
  return s === 0 ? `${m}m` : `${m}m ${s}s`
}

export function formatLongDuration(seconds: number | null | undefined): string {
  if (seconds === null || seconds === undefined) return '—'
  const m = Math.floor(seconds / 60)
  const s = seconds % 60
  return m === 0 ? `${s} sec` : `${m} min ${s} sec`
}

export function initials(name: string): string {
  const parts = name.split(/\s+/).filter(Boolean)
  return parts.map((p) => p[0]).join('').slice(0, 2).toUpperCase() || '?'
}

export const seqLabel = (n: number) => `#${String(n).padStart(4, '0')}`

export function statusLabel(p: Pick<Patient, 'state' | 'assignedResourceId'>): string {
  switch (p.state) {
    case 'REGISTERED': return 'Registered'
    case 'TRIAGED_WAITING': return 'Waiting'
    case 'APPOINTMENT_SCHEDULED': return 'Appointment'
    case 'DISPATCH_PENDING': return p.assignedResourceId ? 'Resource assigned' : 'Awaiting dispatch'
    case 'IN_TRANSIT': return 'Transporting'
    case 'UNDER_TREATMENT': return 'Under treatment'
    case 'DISCHARGED': return 'Discharged'
    case 'CANCELLED': return 'Cancelled'
  }
}

export function stateLabel(state: PatientState): string {
  return statusLabel({ state, assignedResourceId: '' })
}

export const STATE_FILTERS: { value: string; label: string }[] = [
  { value: '', label: 'All statuses' },
  { value: 'REGISTERED', label: 'Registered' },
  { value: 'TRIAGED_WAITING', label: 'Waiting (triage)' },
  { value: 'APPOINTMENT_SCHEDULED', label: 'Appointment' },
  { value: 'DISPATCH_PENDING', label: 'Dispatch pending' },
  { value: 'IN_TRANSIT', label: 'Transporting' },
  { value: 'UNDER_TREATMENT', label: 'Under treatment' },
  { value: 'DISCHARGED', label: 'Discharged' },
  { value: 'CANCELLED', label: 'Cancelled' },
]

const ACTION_LABELS: Record<string, string> = {
  REGISTERED: 'Patient registered',
  TRIAGED: 'Triage admission',
  URGENCY_UPDATED: 'Urgency updated',
  APPOINTMENT_SCHEDULED: 'Appointment scheduled',
  APPOINTMENT_CALLED: 'Called for consultation',
  APPOINTMENT_CANCELLED: 'Appointment cancelled',
  APPOINTMENT_CONVERTED_TO_EMERGENCY: 'Converted to emergency',
  DISPATCH_SELECTED: 'Selected for dispatch',
  DISPATCH_ASSIGNED: 'Dispatch assigned',
  DISPATCH_CANCELLED: 'Dispatch cancelled',
  PICKUP_COMPLETE: 'Pickup complete',
  HANDOVER_COMPLETE: 'Admitted to treatment',
  DISCHARGED: 'Discharged',
  CORRIDOR_BLOCKED: 'Corridor blocked',
  CORRIDOR_REOPENED: 'Corridor reopened',
}

export function actionLabel(action: string): string {
  if (ACTION_LABELS[action]) return ACTION_LABELS[action]
  const words = action.replace(/_/g, ' ').toLowerCase()
  return words.charAt(0).toUpperCase() + words.slice(1)
}

/** Icon used for an audit action in activity lists. */
export function actionIcon(action: string): string {
  if (action.startsWith('REJECTED')) return 'x'
  if (action.startsWith('DISPATCH') || action.startsWith('PICKUP') || action.startsWith('HANDOVER')) return 'route'
  if (action.startsWith('CORRIDOR')) return 'map'
  if (action.startsWith('APPOINTMENT')) return 'calendar'
  if (action === 'TRIAGED' || action === 'URGENCY_UPDATED') return 'pulse'
  if (action === 'DISCHARGED') return 'check'
  return 'users'
}

const URGENCY_COLORS = ['var(--red)', 'var(--orange)', 'var(--yellow)', 'var(--blue)', 'var(--green)']

/** Builds a conic-gradient for the donut charts from real counts. */
export function conicFromCounts(counts: number[], colors: string[] = URGENCY_COLORS): string {
  const total = counts.reduce((a, b) => a + b, 0)
  if (total === 0) return 'conic-gradient(#e6ecee 0 100%)'
  let acc = 0
  const stops: string[] = []
  counts.forEach((c, i) => {
    if (c === 0) return
    const start = (acc / total) * 100
    acc += c
    const end = (acc / total) * 100
    stops.push(`${colors[i % colors.length]} ${start}% ${end}%`)
  })
  return `conic-gradient(${stops.join(', ')})`
}

export const edgeKey = (a: number, b: number) => (a < b ? `${a}-${b}` : `${b}-${a}`)

export function greeting(): string {
  const h = new Date().getHours()
  if (h < 12) return 'Good Morning'
  if (h < 18) return 'Good Afternoon'
  return 'Good Evening'
}

export function downloadCsv(filename: string, rows: (string | number)[][]) {
  const esc = (v: string | number) => `"${String(v).replace(/"/g, '""')}"`
  const blob = new Blob([rows.map((r) => r.map(esc).join(',')).join('\n')], { type: 'text/csv;charset=utf-8' })
  const url = URL.createObjectURL(blob)
  const a = document.createElement('a')
  a.href = url
  a.download = filename
  a.click()
  URL.revokeObjectURL(url)
}
