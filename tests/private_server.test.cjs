'use strict';
const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const http = require('node:http');
const { PrivateService, parseInvitation, pinnedAgent, request } = require('../server/private-service.cjs');

test('private host, TLS pin, invitations, roles, bridge and revocation', async t => {
  const directory = fs.mkdtempSync(path.join(os.tmpdir(), 'emulos-private-'));
  const backend = http.createServer((req, res) => {
    res.setHeader('Content-Type', 'application/json');
    res.end(JSON.stringify({ address: req.headers['x-forwarded-for'], route: req.url, method: req.method }));
  });
  await new Promise(resolve => backend.listen(0, '127.0.0.1', resolve));
  const portServer = http.createServer(); await new Promise(resolve => portServer.listen(0, '127.0.0.1', resolve));
  const port = portServer.address().port; await new Promise(resolve => portServer.close(resolve));
  const host = new PrivateService(path.resolve('server'));
  const client = new PrivateService(path.resolve('server'));
  t.after(async () => { await client.close(); await host.close(); await new Promise(resolve => backend.close(resolve)); fs.rmSync(directory, { recursive: true, force: true }); });
  await host.start({ mode: 'host', data: path.join(directory, 'host'), name: 'Test friends', address: '127.0.0.1', port, testBackend: backend.address().port });
  const agent = pinnedAgent(host.fingerprint);
  t.after(() => agent.destroy());
  const unauthorized = await request(host.endpoint + '/sessions', { agent });
  assert.equal(unauthorized.status, 401);
  const noStats = await request(host.endpoint + '/control/status', { agent });
  assert.equal(noStats.status, 401); assert.equal(JSON.parse(noStats.body).rooms, undefined);
  const wrong = pinnedAgent('0'.repeat(64));
  await assert.rejects(request(host.endpoint + '/control/status', { agent: wrong }), /certificado/); wrong.destroy();
  const invite = await host.remote('/control/invitations', 'POST', { label: '<script>friend</script>', hours: 2 });
  const decoded = parseInvitation(invite.invitation); assert.equal(decoded.endpoint, host.endpoint);
  await client.start({ mode: 'client', data: path.join(directory, 'client'), invitation: invite.invitation, name: 'Friend' });
  const status = await client.remote('/control/status'); assert.equal(status.role, 'client'); assert.equal(status.requests, undefined);
  await assert.rejects(client.remote('/control/invitations', 'POST', { label: 'Intruder' }), /anfitrión/);
  const forwarded = await request(client.bridgeURL + 'whoami');
  assert.equal(forwarded.status, 200); assert.equal(JSON.parse(forwarded.body).address, '127.0.0.1');
  assert.equal(JSON.parse(forwarded.body).route, '/whoami');
  const blocked = await request(new URL(client.bridgeURL).origin + '/whoami'); assert.equal(blocked.status, 403);
  const traversal = await request(host.endpoint + '/title/not-a-title/ports', { agent,
    headers: { Authorization: 'Bearer ' + host.token } }); assert.equal(traversal.status, 400);
  await host.remote('/control/permissions', 'POST', { shareStats: false });
  const hidden = await client.remote('/control/status'); assert.equal(hidden.participants, undefined); assert.equal(hidden.players, undefined);
  await host.remote('/control/revoke', 'POST', { id: invite.id });
  await assert.rejects(client.remote('/control/status'), /Acceso privado/);
  assert.equal((await request(client.bridgeURL + 'sessions')).status, 401);
  const failedLogin = await request(host.endpoint + '/control/join', { agent, method: 'POST' }, JSON.stringify({ id: decoded.id, secret: decoded.secret }));
  assert.equal(failedLogin.status, 401);
  const disk = fs.readFileSync(path.join(directory, 'host/access.json'), 'utf8');
  assert.equal(disk.includes(decoded.secret), false);
});

test('host web panel stays local when invitations advertise a public address', async t => {
  const directory = fs.mkdtempSync(path.join(os.tmpdir(), 'emulos-panel-'));
  const backend = http.createServer((req, res) => res.end('{}'));
  await new Promise(resolve => backend.listen(0, '127.0.0.1', resolve));
  const reservation = http.createServer();
  await new Promise(resolve => reservation.listen(0, '127.0.0.1', resolve));
  const port = reservation.address().port;
  await new Promise(resolve => reservation.close(resolve));
  let ready;
  const host = new PrivateService(path.resolve('server'), event => { if (event.event === 'ready') ready = event; });
  t.after(async () => { await host.close(); await new Promise(resolve => backend.close(resolve)); fs.rmSync(directory, { recursive: true, force: true }); });
  await host.start({ mode: 'host', data: directory, address: '203.0.113.1', port, testBackend: backend.address().port });
  const panel = new URL(ready.adminURL);
  assert.equal(panel.hostname, '127.0.0.1');
  const token = new URLSearchParams(panel.hash.slice(1)).get('token');
  const response = await request(panel.origin + '/control/status', { agent: host.agent, headers: { Authorization: 'Bearer ' + token } });
  assert.equal(response.status, 200);
  assert.equal(JSON.parse(response.body).role, 'host');
  const invitation = await host.remote('/control/invitations', 'POST', { label: 'Friend' });
  assert.equal(parseInvitation(invitation.invitation).endpoint, `https://203.0.113.1:${port}`);
});

test('invitation validation rejects insecure and malformed addresses', () => {
  assert.throws(() => parseInvitation('hello'), /invitación/);
  const value = { endpoint: 'http://example.com', fingerprint: '0'.repeat(64), secret: '1'.repeat(64), id: '2'.repeat(32) };
  assert.throws(() => parseInvitation(Buffer.from(JSON.stringify(value)).toString('base64url')), /inválidas/);
  value.endpoint = 'https://name:password@example.com';
  assert.throws(() => parseInvitation(Buffer.from(JSON.stringify(value)).toString('base64url')), /inválidas/);
});

test('real Xenia backend, profiles, session discovery and QoS', { skip: !process.env.EMULOS_PRIVATE_REAL_SERVER_DIR }, async t => {
  const directory = fs.mkdtempSync(path.join(os.tmpdir(), 'emulos-xenia-'));
  const reservation = http.createServer(); await new Promise(resolve => reservation.listen(0, '127.0.0.1', resolve));
  const port = reservation.address().port; await new Promise(resolve => reservation.close(resolve));
  const host = new PrivateService(process.env.EMULOS_PRIVATE_REAL_SERVER_DIR);
  t.after(async () => { await host.close(); fs.rmSync(directory, { recursive: true, force: true }); });
  await host.start({ mode: 'host', data: directory, name: 'Actual Xenia', address: '127.0.0.1', port });
  const query = async (route, method = 'GET', value) => request(host.bridgeURL + route, {
    method, headers: { 'Content-Type': 'application/json' },
  }, value ? JSON.stringify(value) : undefined);
  const identity = await query('whoami'); assert.equal(identity.status, 200); assert.equal(JSON.parse(identity.body).address, '127.0.0.1');
  const player = await query('players', 'POST', { xuid: '0000000000000001', machineId: '0000000000000002',
    hostAddress: '127.0.0.1', macAddress: '001122334455', gamertag: 'PrivateFriend', settings: {} });
  assert.equal(player.status, 201, player.body.toString());
  const found = await query('players/find', 'POST', { hostAddress: '127.0.0.1' });
  assert.equal(found.status, 201, found.body.toString()); assert.equal(JSON.parse(found.body).gamertag, 'PrivateFriend');
  // Insert a session-shaped fixture into the actual backend database to verify the fast room projection.
  await host.db.collection('sessions').insertOne({ id: '8000000000000001', titleId: '4D5307E6', xuid: '0000000000000001',
    title: 'Gears private test', mediaId: '12345678', version: '1.0', hostAddress: '127.0.0.1', macAddress: '001122334455',
    advertised: true, deleted: false, publicSlotsCount: 4, privateSlotsCount: 0, players: { '0000000000000001': true }, updatedAt: new Date() });
  host.roomCache = null;
  const rooms = await query('sessions'); const data = JSON.parse(rooms.body);
  assert.equal(data.Titles[0].sessions[0].host_gamertag, 'PrivateFriend'); assert.equal(data.Titles[0].sessions[0].total, 4);
  const ports = await query('title/4D5307E6/ports'); assert.equal(ports.status, 200, ports.body.toString());
  const qosRoute = 'title/4D5307E6/sessions/8000000000000001/qos';
  const qos = await query(qosRoute, 'POST', { probe: 'private' }); assert.equal(qos.status, 201, qos.body.toString());
  const read = await query(qosRoute); assert.equal(read.status, 200); assert.deepEqual(JSON.parse(read.body), { probe: 'private' });
  assert.equal(fs.existsSync(path.join(directory, 'qos/4D5307E6/8000000000000001')), true);
});
