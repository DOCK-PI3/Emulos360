'use strict';
const element = id => document.getElementById(id);
let token = sessionStorage.getItem('emulos-token') || '';
const fragment = new URLSearchParams(location.hash.slice(1));
if (fragment.has('token')) { token = fragment.get('token'); sessionStorage.setItem('emulos-token', token); history.replaceState(null, '', '/'); }
async function api(route, method = 'GET', data) {
  const result = await fetch(route, { method, headers: { Authorization: 'Bearer ' + token, 'Content-Type': 'application/json' },
    body: data ? JSON.stringify(data) : undefined, cache: 'no-store', signal: AbortSignal.timeout(15000) });
  const value = await result.json(); if (!result.ok) throw new Error(value.error || 'Error de conexión'); return value;
}
function text(id, value) { element(id).textContent = value; }
function action(fn) { return async () => { try { text('error', ''); await fn(); } catch (error) { text('error', error.message); } }; }
async function refresh() {
  const start = performance.now(); const status = await api('/control/status'); const ms = Math.round(performance.now() - start);
  element('login').hidden = true; element('dashboard').hidden = false; element('host').hidden = status.role !== 'host';
  text('serverName', status.name); text('role', status.role === 'host' ? 'Panel del anfitrión' : 'Panel del participante');
  text('connection', `Respuesta del servicio: ${ms} ms · tiempo activo: ${status.uptime} s${status.upnp ? ' · ' + status.upnp : ''}`);
  element('metrics').replaceChildren();
  const metrics = [['Salas', status.rooms], ['Jugadores en salas', status.players]];
  if (status.role === 'host') metrics.push(['Peticiones', status.requests], ['Errores', status.errors], ['Memoria del servicio', status.memoryMB + ' MB']);
  for (const [name, value] of metrics) {
    if (value === undefined) continue;
    const card = document.createElement('div'); card.className = 'card';
    const title = document.createElement('div'); title.textContent = name;
    const number = document.createElement('div'); number.className = 'number'; number.textContent = value;
    card.append(title, number); element('metrics').append(card);
  }
  element('participants').replaceChildren();
  for (const participant of status.participants || []) { const p = document.createElement('p'); p.textContent = participant.name; element('participants').append(p); }
  if (!status.participants) text('participants', 'El anfitrión mantiene esta lista privada.');
  element('stats').hidden = !(status.leaderboards || []).length;
  element('leaderboards').replaceChildren();
  for (const board of status.leaderboards || []) {
    const p = document.createElement('p'); p.textContent = `${board.player} · juego ${board.titleId} · clasificación ${board.board}: ${board.values}`;
    element('leaderboards').append(p);
  }
  if (status.role === 'host') {
    element('share').checked = status.shareStats; element('invitations').replaceChildren();
    for (const invite of status.invitations || []) {
      const row = document.createElement('p'); const label = document.createElement('span');
      label.textContent = `${invite.label} · ${invite.revoked ? 'Revocada' : new Date(invite.expires).toLocaleString()} `;
      const button = document.createElement('button'); button.textContent = 'Revocar'; button.disabled = invite.revoked;
      button.onclick = action(async () => { await api('/control/revoke', 'POST', { id: invite.id }); await refresh(); });
      row.append(label, button); element('invitations').append(row);
    }
    text('events', (status.events || []).map(event => `${event.time} · ${event.message}`).join('\n'));
  }
  const rooms = await api('/sessions'); element('rooms').replaceChildren();
  for (const title of rooms.Titles) for (const session of title.sessions) {
    const row = document.createElement('tr');
    for (const value of [title.name, session.host_gamertag, `${session.players.length}/${session.total}`, session.version]) {
      const cell = document.createElement('td'); cell.textContent = value; row.append(cell);
    }
    element('rooms').append(row);
  }
}
element('join').onclick = action(async () => {
  const encoded = element('invitation').value.trim().replace(/^emulos360:\/\/join\//, '');
  const normalized = encoded.replace(/-/g, '+').replace(/_/g, '/');
  const invite = JSON.parse(new TextDecoder().decode(Uint8Array.from(atob(normalized), c => c.charCodeAt(0))));
  // Credentials are submitted only to the panel the user has opened, never to a URL from pasted text.
  const result = await api('/control/join', 'POST', { id: invite.id, secret: invite.secret, name: element('name').value });
  token = result.token; sessionStorage.setItem('emulos-token', token); element('invitation').value = ''; await refresh();
});
element('create').onclick = action(async () => {
  const result = await api('/control/invitations', 'POST', { label: element('label').value, hours: Number(element('hours').value) });
  element('created').value = result.invitation; element('created').hidden = false; element('copy').hidden = false; await refresh();
});
element('copy').onclick = action(() => navigator.clipboard.writeText(element('created').value));
element('share').onchange = action(async () => { await api('/control/permissions', 'POST', { shareStats: element('share').checked }); await refresh(); });
element('cleanup').onclick = action(async () => { await api('/control/cleanup', 'POST', {}); await refresh(); });
element('refresh').onclick = action(refresh);
element('logout').onclick = action(async () => {
  await api('/control/leave', 'POST', {}); token = ''; sessionStorage.removeItem('emulos-token');
  element('dashboard').hidden = true; element('login').hidden = false;
});
if (token) action(refresh)();
setInterval(() => { if (token && !document.hidden) action(refresh)(); }, 5000);
