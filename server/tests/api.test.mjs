// Integration tests for the Smart Hospital REST API.
// Spawns the real C++ server and drives it over HTTP. No dependencies (node:test + fetch).
//
//   node --test server/tests/api.test.mjs
//
// Env: HOSPITAL_SERVER_EXE (default bin/HospitalServer.exe), TEST_PORT (default 8097)

import { test, before, after, beforeEach } from 'node:test'
import assert from 'node:assert/strict'
import { spawn } from 'node:child_process'
import net from 'node:net'
import path from 'node:path'
import { fileURLToPath } from 'node:url'

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', '..')
const exe = process.env.HOSPITAL_SERVER_EXE || path.join(root, 'bin', 'HospitalServer.exe')
const port = Number(process.env.TEST_PORT || 8097)
const base = `http://127.0.0.1:${port}/api`
const ALLOWED_ORIGIN = 'http://localhost:8443'

let server

async function call(method, url, body, headers = {}) {
  const res = await fetch(base + url, {
    method,
    headers: { 'Content-Type': 'application/json', ...headers },
    body: body === undefined ? undefined : typeof body === 'string' ? body : JSON.stringify(body),
  })
  const text = await res.text()
  let json = null
  try { json = text ? JSON.parse(text) : null } catch { /* non-JSON */ }
  return { status: res.status, json, headers: res.headers, text }
}
const get = (u) => call('GET', u)
const post = (u, b) => call('POST', u, b ?? {})
const put = (u, b) => call('PUT', u, b)
const del = (u) => call('DELETE', u)

async function waitForServer() {
  for (let i = 0; i < 50; i++) {
    try { if ((await fetch(base + '/health')).ok) return } catch { /* not up yet */ }
    await new Promise((r) => setTimeout(r, 100))
  }
  throw new Error('server did not start')
}

before(async () => {
  server = spawn(exe, ['--port', String(port), '--data-dir', 'data/', '--allow-origin', ALLOWED_ORIGIN], { cwd: root, stdio: 'ignore' })
  await waitForServer()
})
after(() => { server?.kill() })
beforeEach(async () => { assert.equal((await post('/admin/reset')).status, 200) })

const newPatient = (id, extra = {}) => ({
  id, name: 'Test Patient', age: 40, complaint: 'Synthetic complaint', urgency: 3,
  locationNode: 1, targetNode: 2, billing: 100, ...extra,
})
const queueIds = async () => (await get('/triage/queue')).json.queue.map((r) => r.patient.id)

// ---------------------------------------------------------------------------
test('health reports the seeded system', async () => {
  const r = await get('/health')
  assert.equal(r.status, 200)
  assert.equal(r.json.status, 'ok')
  assert.equal(r.json.patients, 12)
})

test('seed patients are exposed with consistent fields', async () => {
  const r = await get('/patients')
  assert.equal(r.json.total, 12)
  const p104 = r.json.patients.find((p) => p.id === 'P104')
  assert.equal(p104.name, 'Marcus Brody')
  assert.equal(p104.urgency, 1)
  assert.equal(p104.state, 'TRIAGED_WAITING')
  assert.equal(p104.locationName, 'ER Triage & Main Waiting Hall')
})

test('patient search and filters', async () => {
  assert.equal((await get('/patients?q=brody')).json.matched, 1)
  assert.equal((await get('/patients?urgency=1')).json.matched, 3)
  assert.equal((await get('/patients?state=APPOINTMENT_SCHEDULED')).json.matched, 3)
  assert.equal((await get('/patients?urgency=9')).status, 400)
})

test('unknown patient returns 404; unknown route 404; wrong method 405', async () => {
  assert.equal((await get('/patients/NOPE')).status, 404)
  assert.equal((await get('/nothing')).status, 404)
  assert.equal((await call('DELETE', '/patients')).status, 405)
})

// ---------------------------------------------------------------------------
test('registration validation and duplicate detection', async () => {
  assert.equal((await post('/patients', '{not json')).status, 400)
  assert.equal((await post('/patients', newPatient('P900', { urgency: 9 }))).status, 400)
  assert.equal((await post('/patients', newPatient('P900', { urgency: 0 }))).status, 400)
  assert.equal((await post('/patients', newPatient('P900', { age: 151 }))).status, 400)
  assert.equal((await post('/patients', newPatient('P900', { age: '40' }))).status, 400)
  assert.equal((await post('/patients', newPatient('P900', { name: 'R2D2' }))).status, 422)
  assert.equal((await post('/patients', newPatient('P900', { locationNode: 99 }))).status, 422)
  assert.equal((await post('/patients', newPatient('P900', { billing: -5 }))).status, 422)
  assert.equal((await post('/patients', newPatient('P 900'))).status, 422)
  assert.equal((await post('/patients', newPatient('P101'))).status, 422) // duplicate seed id
  const { id, ...missing } = newPatient('P900')
  assert.equal((await post('/patients', missing)).status, 400)
  assert.equal((await get('/patients')).json.total, 12) // nothing was created
})

test('registered patient appears in the directory in REGISTERED state', async () => {
  const r = await post('/patients', newPatient('P901'))
  assert.equal(r.status, 201)
  assert.equal(r.json.patient.state, 'REGISTERED')
  const dir = await get('/patients?q=P901')
  assert.equal(dir.json.matched, 1)
  const detail = await get('/patients/P901')
  assert.equal(detail.json.history[0].action, 'REGISTERED')
})

test('patient PUT only accepts urgency', async () => {
  assert.equal((await put('/patients/P101', { age: 5 })).status, 400)
  const ok = await put('/patients/P101', { urgency: 2 })
  assert.equal(ok.status, 200)
  assert.equal(ok.json.patient.urgency, 2)
})

// ---------------------------------------------------------------------------
test('triage queue follows min-heap policy: urgency, then arrival sequence', async () => {
  const ids = await queueIds()
  assert.deepEqual(ids, ['P104', 'P107', 'P112', 'P103', 'P105', 'P109', 'P108', 'P110', 'P101'])
  const next = await get('/triage/next')
  assert.equal(next.json.patient.id, 'P104')
  assert.equal(next.json.queueSize, 9)
})

test('admit a new emergency patient and verify its heap rank', async () => {
  await post('/patients', newPatient('P902', { urgency: 1 }))
  const admit = await post('/triage/admit', { patientId: 'P902', urgency: 1 })
  assert.equal(admit.status, 201)
  assert.equal(admit.json.rank, 4) // behind P104, P107, P112 (same urgency, earlier arrival)
  assert.equal(admit.json.patient.state, 'TRIAGED_WAITING')
  assert.equal(admit.json.queueSize, 10)
  // Cannot admit twice
  assert.equal((await post('/triage/admit', { patientId: 'P902', urgency: 1 })).status, 422)
  assert.equal((await post('/triage/admit', { patientId: 'P902', urgency: 7 })).status, 400)
})

test('changing urgency reorders the heap and preserves arrival sequence', async () => {
  await post('/patients', newPatient('P903', { urgency: 5 }))
  await post('/triage/admit', { patientId: 'P903', urgency: 5 })
  assert.equal((await queueIds()).at(-1), 'P903')
  const before = (await get('/patients/P903')).json.patient.arrivalSequence
  const upd = await put('/triage/P903/urgency', { urgency: 1 })
  assert.equal(upd.status, 200)
  assert.equal(upd.json.rank, 4)
  assert.equal(upd.json.patient.arrivalSequence, before)
  assert.deepEqual((await queueIds()).slice(0, 4), ['P104', 'P107', 'P112', 'P903'])
  assert.equal((await put('/triage/P903/urgency', { urgency: 6 })).status, 400)
  assert.equal((await put('/triage/NOPE/urgency', { urgency: 1 })).status, 404)
})

test('heapArray is the real heap storage order', async () => {
  const q = (await get('/triage/queue')).json
  assert.equal(q.heapArray[0].patientId, 'P104') // root = highest priority
  assert.equal(q.heapArray.length, q.size)
})

// ---------------------------------------------------------------------------
test('full dispatch lifecycle: select, assign, pickup, complete, discharge', async () => {
  const sel = await post('/dispatch/select-next')
  assert.equal(sel.status, 200)
  assert.equal(sel.json.patient.id, 'P104')
  assert.equal(sel.json.source, 'TRIAGE_QUEUE')
  assert.equal(sel.json.patient.state, 'DISPATCH_PENDING')
  assert.equal(sel.json.triageQueueSize, 8)
  assert.ok(!(await queueIds()).includes('P104'))

  const pending = await get('/dispatch/pending')
  assert.equal(pending.json.dispatches.length, 1)
  assert.equal(pending.json.dispatches[0].status, 'AWAITING_ASSIGNMENT')

  const assign = await post('/dispatch/assign', { patientId: 'P104' })
  assert.equal(assign.status, 200)
  const d = assign.json.dispatch
  // GURNEY-03 starts at node 1 (patient's location): cheapest Leg 1 (0s)
  assert.equal(d.resource.id, 'GURNEY-03')
  assert.equal(d.status, 'RESOURCE_ASSIGNED')
  assert.equal(d.leg1.costSeconds, 0)
  assert.deepEqual(d.leg2.path, [1, 2])
  assert.equal(d.leg2.costSeconds, 100)
  assert.equal(d.totalCostSeconds, 100)
  assert.equal((await get('/dispatch/resources')).json.summary.available, 2) // availability changed
  assert.equal((await post('/dispatch/assign', { patientId: 'P104' })).status, 422) // duplicate assignment

  const pick = await post('/dispatch/pickup', { patientId: 'P104' })
  assert.equal(pick.json.dispatch.status, 'IN_TRANSIT')
  assert.equal((await get('/patients/P104')).json.patient.state, 'IN_TRANSIT')
  assert.equal((await get('/dispatch/resources')).json.summary.transporting, 1)

  const done = await post('/dispatch/complete', { patientId: 'P104' })
  assert.equal(done.json.patient.state, 'UNDER_TREATMENT')
  assert.equal(done.json.patient.locationNode, 2)
  const res = (await get('/dispatch/resources')).json
  assert.equal(res.summary.available, 3)
  assert.equal(res.resources.find((r) => r.id === 'GURNEY-03').nodeId, 2) // resource moved to destination

  const dis = await post('/patients/P104/discharge')
  assert.equal(dis.json.patient.state, 'DISCHARGED')
  assert.equal((await post('/patients/P104/discharge')).status, 422)
})

test('dispatch cancel releases the resource', async () => {
  await post('/dispatch/select-next')
  await post('/dispatch/assign', { patientId: 'P104' })
  assert.equal((await get('/dispatch/resources')).json.summary.enRoutePickup, 1)
  const c = await post('/dispatch/cancel', { patientId: 'P104' })
  assert.equal(c.json.patient.state, 'CANCELLED')
  assert.equal((await get('/dispatch/resources')).json.summary.available, 3)
})

test('dispatch rejects invalid state transitions', async () => {
  assert.equal((await post('/dispatch/assign', { patientId: 'P101' })).status, 422) // still waiting in triage
  assert.equal((await post('/dispatch/pickup', { patientId: 'P101' })).status, 422)
  assert.equal((await post('/dispatch/complete', { patientId: 'P101' })).status, 422)
  assert.equal((await post('/dispatch/assign', { patientId: 'NOPE' })).status, 404)
  assert.equal((await post('/dispatch/assign', {})).status, 400)
})

test('no resource available is reported, patient stays pending', async () => {
  // Occupy all three gurneys with three patients.
  for (let i = 0; i < 3; i++) {
    const sel = await post('/dispatch/select-next')
    assert.equal((await post('/dispatch/assign', { patientId: sel.json.patient.id })).status, 200)
  }
  const sel4 = await post('/dispatch/select-next')
  const r = await post('/dispatch/assign', { patientId: sel4.json.patient.id })
  assert.equal(r.status, 422)
  assert.match(r.json.error.message, /No available/)
  const pending = (await get('/dispatch/pending')).json.dispatches
  assert.equal(pending.find((d) => d.patient.id === sel4.json.patient.id).status, 'AWAITING_ASSIGNMENT')
})

// ---------------------------------------------------------------------------
test('campus graph is the real 12 node / 15 corridor topology', async () => {
  assert.equal((await get('/campus/nodes')).json.nodes.length, 12)
  const edges = (await get('/campus/edges')).json
  assert.equal(edges.edges.length, 15)
  assert.equal(edges.blockedCount, 0)
})

test('Dijkstra route matches the original system (0 -> 3 = 270s via 0,1,5,3)', async () => {
  const r = (await get('/campus/route?from=0&to=3')).json
  assert.deepEqual(r.path, [0, 1, 5, 3])
  assert.equal(r.costSeconds, 270)
  assert.equal((await get('/campus/route?from=0&to=99')).status, 404)
  assert.equal((await get('/campus/route?from=a&to=1')).status, 400)
  assert.equal((await get('/campus/route?from=0')).status, 400)
})

test('blocking a corridor changes subsequent route calculations', async () => {
  assert.equal((await get('/campus/route?from=1&to=2')).json.costSeconds, 100)
  assert.equal((await post('/campus/corridor/block', { u: 1, v: 2 })).status, 200)
  const detour = (await get('/campus/route?from=1&to=2')).json
  assert.equal(detour.costSeconds, 140) // 1 -> 0 -> 2
  assert.deepEqual(detour.path, [1, 0, 2])
  assert.equal((await get('/campus/edges')).json.blockedCount, 1)
  assert.equal((await post('/campus/corridor/reopen', { u: 1, v: 2 })).status, 200)
  assert.equal((await get('/campus/route?from=1&to=2')).json.costSeconds, 100)
})

test('unreachable destination is reported, not faked', async () => {
  await post('/campus/corridor/block', { u: 10, v: 11 })
  const r = (await get('/campus/route?from=0&to=11')).json
  assert.equal(r.reachable, false)
  assert.equal(r.costSeconds, null)
  assert.deepEqual(r.path, [])
  assert.equal((await post('/campus/corridor/block', { u: 0, v: 11 })).status, 422) // no such corridor
  assert.equal((await post('/campus/corridor/block', { u: 0 })).status, 400)
})

test('closed corridor blocks dispatch assignment to unreachable destination', async () => {
  await post('/campus/corridor/block', { u: 10, v: 11 })
  await post('/patients', newPatient('P904', { urgency: 1, targetNode: 11 }))
  await post('/triage/admit', { patientId: 'P904', urgency: 1 })
  // Drain higher priority patients first so P904 is selected.
  let id = ''
  for (let i = 0; i < 10 && id !== 'P904'; i++) {
    id = (await post('/dispatch/select-next')).json.patient.id
    if (id !== 'P904') await post('/dispatch/cancel', { patientId: id })
  }
  assert.equal(id, 'P904')
  const r = await post('/dispatch/assign', { patientId: 'P904' })
  assert.equal(r.status, 422)
  assert.match(r.json.error.message, /unreachable/)
})

test('traversals and MST', async () => {
  const bfs = (await get('/campus/traversal?algorithm=bfs&start=0')).json
  assert.equal(bfs.order[0], 0)
  assert.equal(bfs.visitedCount, 12)
  const dfs = (await get('/campus/traversal?algorithm=dfs&start=0')).json
  assert.equal(dfs.visitedCount, 12)
  assert.notDeepEqual(bfs.order, dfs.order)
  assert.equal((await get('/campus/traversal?algorithm=xyz&start=0')).status, 400)
  const prim = (await get('/campus/mst?algorithm=prim')).json
  const kruskal = (await get('/campus/mst?algorithm=kruskal')).json
  assert.equal(prim.totalWeightSeconds, 1280)
  assert.equal(kruskal.totalWeightSeconds, 1280)
  assert.equal(kruskal.edges.length, 11)
  assert.equal((await get('/campus/mst?algorithm=none')).status, 400)
})

// ---------------------------------------------------------------------------
test('appointments preserve FIFO order', async () => {
  const q0 = (await get('/appointments')).json
  assert.deepEqual(q0.queue.map((r) => r.patient.id), ['P102', 'P106', 'P111'])
  await post('/patients', newPatient('P905', { urgency: 5 }))
  assert.equal((await post('/appointments', { patientId: 'P905' })).status, 201)
  assert.equal((await post('/appointments', { patientId: 'P905' })).status, 422) // no double booking
  assert.deepEqual((await get('/appointments')).json.queue.map((r) => r.patient.id), ['P102', 'P106', 'P111', 'P905'])

  const next = await post('/appointments/next')
  assert.equal(next.json.patient.id, 'P102') // first in, first out
  assert.equal(next.json.patient.state, 'UNDER_TREATMENT')

  assert.equal((await del('/appointments/P111')).status, 200)
  assert.deepEqual((await get('/appointments')).json.queue.map((r) => r.patient.id), ['P106', 'P905'])
  assert.equal((await del('/appointments/P111')).status, 422)
  assert.equal((await del('/appointments/NOPE')).status, 404)

  const s = (await get('/appointments')).json.stats
  assert.equal(s.called, 1)
  assert.equal(s.cancelled, 1)
})

test('empty appointment queue cannot be called', async () => {
  for (let i = 0; i < 3; i++) assert.equal((await post('/appointments/next')).status, 200)
  assert.equal((await post('/appointments/next')).status, 422)
})

test('appointment can be converted to an emergency and enters the heap', async () => {
  const r = await post('/appointments/P111/convert', { urgency: 1 })
  assert.equal(r.status, 200)
  assert.equal(r.json.patient.state, 'TRIAGED_WAITING')
  const ids = await queueIds()
  assert.deepEqual(ids.slice(0, 4), ['P104', 'P107', 'P111', 'P112']) // urgency 1, arrival 11 sits before P112 (12)
  assert.ok(!(await get('/appointments')).json.queue.some((x) => x.patient.id === 'P111'))
  assert.equal((await post('/appointments/P111/convert', { urgency: 1 })).status, 422)
})

// ---------------------------------------------------------------------------
test('audit trail records operations in chronological order', async () => {
  await post('/patients', newPatient('P906', { urgency: 2 }))
  await post('/triage/admit', { patientId: 'P906', urgency: 2 })
  await put('/triage/P906/urgency', { urgency: 1 })
  await post('/campus/corridor/block', { u: 0, v: 6 })
  await post('/triage/admit', { patientId: 'P906', urgency: 2 }) // rejected, but audited
  const a = (await get('/audit')).json
  const actions = a.events.map((e) => e.action)
  for (const x of ['REGISTERED', 'TRIAGED', 'URGENCY_UPDATED', 'CORRIDOR_BLOCKED', 'REJECTED_TRIAGE']) {
    assert.ok(actions.includes(x), `missing ${x}`)
  }
  const idx = a.events.map((e) => e.index)
  assert.deepEqual(idx, [...idx].sort((x, y) => x - y))
  const mine = (await get('/audit?patientId=P906')).json
  assert.ok(mine.events.every((e) => e.patientId === 'P906'))
  assert.equal((await get('/audit?limit=0')).status, 400)
  assert.equal((await get('/audit?limit=1')).json.returned, 1)
})

test('analytics overview reflects live backend state', async () => {
  const o = (await get('/analytics/overview')).json
  assert.equal(o.totalPatients, 12)
  assert.equal(o.triageQueueSize, 9)
  assert.equal(o.appointmentQueueSize, 3)
  assert.equal(o.fleet.total, 3)
  assert.equal(o.highestPriority.id, 'P104')
  assert.equal(o.urgencyDistribution.reduce((s, d) => s + d.count, 0), 12)
  await post('/dispatch/select-next')
  const o2 = (await get('/analytics/overview')).json
  assert.equal(o2.triageQueueSize, 8)
  assert.equal(o2.activeDispatches, 1)
  assert.equal(o2.highestPriority.id, 'P107')
})

test('sorted reports use the custom sorts and agree across algorithms', async () => {
  const merge = (await get('/analytics/patients?sortBy=billing&algorithm=merge')).json
  const quick = (await get('/analytics/patients?sortBy=billing&algorithm=quick')).json
  const heap = (await get('/analytics/patients?sortBy=billing&algorithm=heap')).json
  const order = (r) => r.patients.map((p) => p.id)
  assert.deepEqual(order(quick), order(merge))
  assert.deepEqual(order(heap), order(merge))
  const billing = merge.patients.map((p) => p.billing)
  assert.deepEqual(billing, [...billing].sort((a, b) => a - b))
  assert.equal(merge.metrics.stable, true)
  assert.ok(merge.metrics.comparisons > 0)
  assert.equal((await get('/analytics/patients?sortBy=nope')).status, 400)
  assert.equal((await get('/analytics/patients?algorithm=bubble')).status, 400)
})

test('algorithm comparison returns real metrics', async () => {
  const r = (await get('/analytics/algorithms')).json
  assert.equal(r.sorts.length, 3)
  assert.equal(r.graph.length, 5)
  assert.ok(r.graph.every((g) => g.averageMicroseconds >= 0 && g.iterations > 0))
})

// ---------------------------------------------------------------------------
test('CORS: allowed origin is echoed, foreign origin cannot change state', async () => {
  const ok = await call('GET', '/health', undefined, { Origin: ALLOWED_ORIGIN })
  assert.equal(ok.headers.get('access-control-allow-origin'), ALLOWED_ORIGIN)
  const foreignRead = await call('GET', '/health', undefined, { Origin: 'http://evil.example' })
  assert.equal(foreignRead.headers.get('access-control-allow-origin'), null)
  const foreignWrite = await call('POST', '/admin/reset', {}, { Origin: 'http://evil.example' })
  assert.equal(foreignWrite.status, 403)
  const pre = await call('OPTIONS', '/patients', undefined, { Origin: ALLOWED_ORIGIN })
  assert.equal(pre.status, 204)
  assert.match(pre.headers.get('access-control-allow-methods'), /PUT/)
})

test('oversized bodies, chunked bodies and garbage requests are rejected', async () => {
  const big = await post('/patients', 'x'.repeat(70 * 1024))
  assert.equal(big.status, 413)
  const raw = (payload) => new Promise((resolve) => {
    const s = net.connect(port, '127.0.0.1', () => s.write(payload))
    let data = ''
    s.on('data', (d) => { data += d })
    s.on('close', () => resolve(data))
    setTimeout(() => s.destroy(), 3000)
  })
  assert.match(await raw('NOT HTTP\r\n\r\n'), /^HTTP\/1.1 400/)
  assert.match(await raw('POST /api/patients HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n'), /^HTTP\/1.1 501/)
  // An idle connection must not block other clients.
  const idle = net.connect(port, '127.0.0.1')
  const r = await get('/health')
  assert.equal(r.status, 200)
  idle.destroy()
})

test('reset restores seed state', async () => {
  await post('/patients', newPatient('P907'))
  assert.equal((await get('/patients')).json.total, 13)
  await post('/admin/reset')
  assert.equal((await get('/patients')).json.total, 12)
})
