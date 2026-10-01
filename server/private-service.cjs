'use strict';
const fs = require('node:fs');
const path = require('node:path');
const http = require('node:http');
const https = require('node:https');
const tls = require('node:tls');
const net = require('node:net');
const dns = require('node:dns').promises;
const os = require('node:os');
const crypto = require('node:crypto');
const { spawn } = require('node:child_process');
const { performance, monitorEventLoopDelay } = require('node:perf_hooks');

const random = () => crypto.randomBytes(32).toString('hex');
const hash = value => crypto.createHash('sha256').update(value).digest('hex');
const MAX_BODY = 2 * 1024 * 1024;
function save(file, data) {
  fs.mkdirSync(path.dirname(file), { recursive: true, mode: 0o700 });
  fs.writeFileSync(file + '.tmp', JSON.stringify(data), { mode: 0o600 });
  fs.renameSync(file + '.tmp', file);
}
function json(res, status, value) {
  if (res.destroyed || res.writableEnded) return;
  res.writeHead(status, { 'Content-Type': 'application/json', 'Cache-Control': 'no-store',
    'X-Content-Type-Options': 'nosniff', 'Referrer-Policy': 'no-referrer' });
  res.end(JSON.stringify(value));
}
async function body(req) {
  let length = 0;
  const chunks = [];
  for await (const chunk of req) {
    length += chunk.length;
    if (length > MAX_BODY) throw new Error('Petición demasiado grande');
    chunks.push(chunk);
  }
  return Buffer.concat(chunks);
}
async function freePort() {
  const server = net.createServer();
  await new Promise((resolve, reject) => { server.once('error', reject); server.listen(0, '127.0.0.1', resolve); });
  const port = server.address().port;
  await new Promise(resolve => server.close(resolve));
  return port;
}
function parseInvitation(value) {
  const encoded = String(value).trim().replace(/^emulos360:\/\/join\//, '');
  if (encoded.length > 8192) throw new Error('Invitación demasiado grande');
  let data;
  try { data = JSON.parse(Buffer.from(encoded, 'base64url').toString()); }
  catch { throw new Error('Pega una invitación válida de Emulos360'); }
  const url = new URL(data.endpoint);
  if (url.protocol !== 'https:' || !url.hostname || url.username || url.password || url.search || url.hash ||
      (url.pathname !== '/' && url.pathname !== '') || !/^[a-f0-9]{64}$/.test(data.fingerprint) ||
      !/^[a-f0-9]{64}$/.test(data.secret) || !/^[a-f0-9]{32}$/.test(data.id))
    throw new Error('Dirección o credenciales de invitación inválidas');
  return data;
}
function pinnedAgent(fingerprint) {
  const agent = new https.Agent({ keepAlive: true, maxSockets: 16 });
  agent.createConnection = (options, callback) => {
    let completed = false;
    const finish = (error, socket) => {
      if (completed) return;
      completed = true;
      callback(error, socket);
    };
    const socket = tls.connect({ ...options, rejectUnauthorized: false }, () => {
      const cert = socket.getPeerCertificate();
      if (!cert.raw || hash(cert.raw) !== fingerprint) {
        const error = new Error('El certificado no coincide con la invitación');
        finish(error); socket.destroy(); return;
      }
      finish(null, socket);
    });
    socket.once('error', error => finish(error));
    socket.setTimeout(15000, () => socket.destroy(new Error('Tiempo de conexión agotado')));
  };
  return agent;
}
function request(url, options = {}, payload) {
  return new Promise((resolve, reject) => {
    const transport = new URL(url).protocol === 'https:' ? https : http;
    const req = transport.request(url, { ...options, timeout: 15000 }, res => {
      let size = 0; const chunks = [];
      res.on('data', chunk => { size += chunk.length; if (size > 8 * 1024 * 1024) res.destroy(new Error('Respuesta excesiva')); else chunks.push(chunk); });
      res.on('error', reject);
      res.on('end', () => resolve({ status: res.statusCode, headers: res.headers, body: Buffer.concat(chunks) }));
    });
    req.on('timeout', () => req.destroy(new Error('Tiempo de espera agotado')));
    req.on('error', reject);
    req.end(payload);
  });
}

class PrivateService {
  constructor(directory, emit = () => {}) {
    this.directory = path.resolve(directory);
    this.emit = emit;
    this.tokens = new Map(); this.members = new Map(); this.failures = new Map();
    this.children = []; this.events = []; this.samples = [];
    this.requests = 0; this.errors = 0; this.started = Date.now(); this.stopping = false;
    this.delay = monitorEventLoopDelay({ resolution: 20 }); this.delay.enable();
    this.upnpStatus = 'Apertura manual del puerto TCP del servidor';
  }
  event(message) {
    this.events.push({ time: new Date().toISOString(), message });
    this.events = this.events.slice(-100);
  }
  async start(config) {
    this.config = config;
    if (!config.data || !['host', 'client'].includes(config.mode)) throw new Error('Configuración inválida');
    fs.mkdirSync(config.data, { recursive: true, mode: 0o700 });
    if (config.mode === 'host') await this.startHost(); else await this.startClient();
    await this.startBridge();
    this.emit({ event: 'ready', hosting: config.mode === 'host', endpoint: this.endpoint,
      bridge: this.bridgeURL, fingerprint: this.fingerprint, name: this.serverName,
      adminURL: config.mode === 'host' ? this.connectEndpoint + '/#token=' + this.adminToken : '' });
    await this.publish();
    this.interval = setInterval(() => this.publish().catch(() => {}), 5000);
  }
  launch(executable, args, options = {}) {
    const child = spawn(executable, args, { windowsHide: true, stdio: ['ignore', 'pipe', 'pipe'], ...options });
    this.children.push(child);
    child.stdout.on('data', () => {});
    // Engine API logs stay local and never enter the private panel or IPC.
    child.stderr.on('data', chunk => {
      if (this.config?.data) fs.appendFileSync(path.join(this.config.data, 'service-errors.log'), chunk, { mode: 0o600 });
    });
    child.once('error', error => { this.childFailure = error.message; });
    child.once('exit', code => {
      if (!this.stopping) {
        this.childFailure = `Un componente del servidor terminó (${code})`;
        this.emit({ event: 'error', message: this.childFailure });
        this.close().catch(() => {});
      }
    });
    return child;
  }
  async startHost() {
    const config = this.config;
    if (!Number.isInteger(config.port) || config.port < 1024 || config.port > 65535) throw new Error('Usa un puerto entre 1024 y 65535');
    this.address = String(config.address || '127.0.0.1').trim();
    if (!/^[a-zA-Z0-9.-]{1,253}$/.test(this.address)) throw new Error('Introduce una IP IPv4 o un dominio válido');
    this.advertisedIP = net.isIPv4(this.address) ? this.address : (await dns.lookup(this.address, { family: 4 })).address;
    this.serverName = String(config.name || 'Partidas de amigos').slice(0, 80);
    this.stateFile = path.join(config.data, 'access.json');
    this.state = fs.existsSync(this.stateFile) ? JSON.parse(fs.readFileSync(this.stateFile)) : {
      admin: random(), invitations: [], shareStats: true,
    };
    this.adminToken = this.state.admin; save(this.stateFile, this.state);
    const certificateFile = path.join(config.data, 'certificate.json');
    let pair;
    if (fs.existsSync(certificateFile)) pair = JSON.parse(fs.readFileSync(certificateFile));
    else {
      pair = require('selfsigned').generate([{ name: 'commonName', value: 'Emulos360 Private Netplay' }],
        { keySize: 2048, days: 3650, algorithm: 'sha256' });
      save(certificateFile, { private: pair.private, cert: pair.cert });
    }
    this.fingerprint = hash(new crypto.X509Certificate(pair.cert).raw);
    if (!config.testBackend) {
      let dbPort = await freePort();
      while (dbPort === config.port) dbPort = await freePort();
      this.backendPort = await freePort();
      while (this.backendPort === dbPort || this.backendPort === config.port) this.backendPort = await freePort();
      const dbPath = path.join(config.data, 'database'); fs.mkdirSync(dbPath, { recursive: true });
      const executable = path.join(this.directory, 'mongodb', 'bin', process.platform === 'win32' ? 'mongod.exe' : 'mongod');
      if (!fs.existsSync(executable)) throw new Error('Falta MongoDB portable. Ejecuta el instalador de dependencias MultiP');
      this.launch(executable, ['--dbpath', dbPath, '--bind_ip', '127.0.0.1', '--port', String(dbPort),
        '--logpath', path.join(config.data, 'mongodb.log'), '--logappend', '--quiet', '--wiredTigerCacheSizeGB', '0.25']);
      const { MongoClient } = require(path.join(this.directory, 'upstream/node_modules/mongodb'));
      this.mongo = new MongoClient(`mongodb://127.0.0.1:${dbPort}/emulos360`, { serverSelectionTimeoutMS: 20000 });
      await this.mongo.connect(); this.db = this.mongo.db('emulos360');
      if (this.stopping) throw new Error('Inicio cancelado');
      await this.db.collection('sessions').createIndex({ titleId: 1, advertised: 1, deleted: 1 });
      this.launch(process.execPath, [path.join(this.directory, 'upstream/dist/main.js')], {
        cwd: path.join(this.directory, 'upstream'), env: { ...process.env, API_PORT: String(this.backendPort),
          PORT: String(this.backendPort), EMULOS_BIND_ADDRESS: '127.0.0.1', MONGO_URI: `mongodb://127.0.0.1:${dbPort}/emulos360`,
          SWAGGER_API: 'false', SSL: 'false', nginx: 'true', heroku_nginx: 'false', xstorage: 'false', EMULOS_STATE_DIR: config.data },
      });
      const deadline = Date.now() + 30000;
      for (;;) {
        if (this.stopping) throw new Error('Inicio cancelado');
        if (this.childFailure) throw new Error(this.childFailure);
        try { const res = await request(`http://127.0.0.1:${this.backendPort}/whoami`); if (res.status === 200) break; } catch {}
        if (Date.now() > deadline) throw new Error('El servicio de Xenia no terminó de iniciar');
        await new Promise(resolve => setTimeout(resolve, 250));
      }
    } else this.backendPort = config.testBackend;
    this.gateway = https.createServer({ key: pair.private, cert: pair.cert, minVersion: 'TLSv1.2' },
      (req, res) => this.handle(req, res).catch(error => { this.errors++; json(res, 500, { error: error.message }); }));
    this.gateway.requestTimeout = 20000; this.gateway.headersTimeout = 10000;
    await new Promise((resolve, reject) => { this.gateway.once('error', reject); this.gateway.listen(config.port, '0.0.0.0', resolve); });
    this.endpoint = `https://${this.address}:${config.port}`;
    this.connectEndpoint = `https://127.0.0.1:${config.port}`;
    this.agent = pinnedAgent(this.fingerprint); this.token = this.adminToken;
    if (config.upnp) await this.mapPort();
    this.event('Servidor privado iniciado');
  }
  async mapPort() {
    this.upnp = require('nat-upnp').createClient({ timeout: 3000 });
    try {
      const mappings = await new Promise((resolve, reject) => this.upnp.getMappings((err, list) => err ? reject(err) : resolve(list)));
      if (mappings.some(entry => Number(entry.public.port) === this.config.port && entry.protocol === 'TCP'))
        throw new Error('El puerto ya tiene una asignación en el router');
      await new Promise((resolve, reject) => this.upnp.portMapping({ public: this.config.port,
        private: this.config.port, protocol: 'TCP', ttl: 3600, description: 'Emulos360 MultiP' }, err => err ? reject(err) : resolve()));
      this.mapped = true; this.upnpStatus = 'Puerto TCP asignado por UPnP';
      this.renew = setInterval(() => this.upnp.portMapping({ public: this.config.port, private: this.config.port,
        protocol: 'TCP', ttl: 3600, description: 'Emulos360 MultiP' }, error => {
          if (error) this.upnpStatus = 'Falló la renovación UPnP: comprueba el router';
        }), 1800000);
    } catch (error) { this.upnpStatus = 'UPnP no disponible: ' + error.message; }
  }
  async startClient() {
    const invite = parseInvitation(this.config.invitation);
    if (this.config.address) {
      const host = String(this.config.address).trim();
      if (!/^[a-zA-Z0-9.-]{1,253}$/.test(host) || !Number.isInteger(this.config.port) || this.config.port < 1 || this.config.port > 65535)
        throw new Error('IP o puerto inválido');
      invite.endpoint = `https://${host}:${this.config.port}`;
    }
    this.endpoint = invite.endpoint; this.fingerprint = invite.fingerprint; this.agent = pinnedAgent(this.fingerprint);
    this.connectEndpoint = this.endpoint;
    const response = await request(this.endpoint + '/control/join', { method: 'POST', agent: this.agent,
      headers: { 'Content-Type': 'application/json' } }, JSON.stringify({ id: invite.id, secret: invite.secret, name: this.config.name }));
    const result = JSON.parse(response.body);
    if (response.status !== 200) throw new Error(result.error || 'No se pudo entrar al servidor');
    this.token = result.token; this.serverName = result.name;
  }
  authenticate(req) {
    const token = String(req.headers.authorization || '').replace(/^Bearer /, '');
    if (token.length === 64 && crypto.timingSafeEqual(Buffer.from(hash(token)), Buffer.from(hash(this.adminToken))))
      return { role: 'host', id: 'host', name: 'Anfitrión', lastSeen: Date.now() };
    const member = this.tokens.get(hash(token));
    if (!member) return null;
    const invite = this.state.invitations.find(item => item.id === member.inviteId);
    if (!invite || invite.revoked || invite.expires < Date.now()) return null;
    member.lastSeen = Date.now(); return member;
  }
  async handle(req, res) {
    this.requests++;
    const url = new URL(req.url, 'https://local'); const route = url.pathname;
    if (/%(?:2f|5c|2e|00)/i.test(route) || route.includes('\\') ||
        (route.startsWith('/title/') && !/^\/title\/[a-f0-9]{8}(?:\/|$)/i.test(route))) {
      json(res, 400, { error: 'Ruta inválida' }); return;
    }
    const ip = req.socket.remoteAddress.replace(/^::ffff:/, '');
    if (req.method === 'GET' && (route === '/' || route === '/panel.js')) {
      res.writeHead(200, { 'Content-Type': route === '/' ? 'text/html; charset=utf-8' : 'text/javascript; charset=utf-8',
        'Content-Security-Policy': "default-src 'self'; script-src 'self'; style-src 'self' 'unsafe-inline'; frame-ancestors 'none'; base-uri 'none'",
        'X-Content-Type-Options': 'nosniff', 'Cache-Control': 'no-store', 'Referrer-Policy': 'no-referrer' });
      res.end(fs.readFileSync(path.join(this.directory, route === '/' ? 'panel.html' : 'panel.js'))); return;
    }
    if (route === '/control/join' && req.method === 'POST') {
      const key = ip; const attempts = this.failures.get(key) || { count: 0, until: Date.now() + 60000 };
      if (attempts.until < Date.now()) { attempts.count = 0; attempts.until = Date.now() + 60000; }
      if (attempts.count >= 10) { json(res, 429, { error: 'Demasiados intentos. Espera un minuto' }); return; }
      const credentials = JSON.parse((await body(req)).toString());
      if (credentials.secret === this.adminToken && req.socket.remoteAddress === '127.0.0.1') {
        json(res, 200, { token: this.adminToken, name: this.serverName, role: 'host' }); return;
      }
      const invite = this.state.invitations.find(item => item.id === credentials.id && !item.revoked && item.expires > Date.now());
      if (!invite || typeof credentials.secret !== 'string' || hash(credentials.secret) !== invite.hash) {
        attempts.count++; this.failures.set(key, attempts); this.errors++; json(res, 401, { error: 'Invitación incorrecta, caducada o revocada' }); return;
      }
      for (const [digest, entry] of this.tokens) {
        if (Date.now() - entry.lastSeen > 86400000) { this.tokens.delete(digest); this.members.delete(entry.id); }
      }
      if (this.members.size >= 128) { json(res, 503, { error: 'Servidor lleno' }); return; }
      const token = random(); const id = crypto.randomBytes(16).toString('hex');
      const member = { id, inviteId: invite.id, name: String(credentials.name || invite.label).slice(0, 64),
        role: 'client', lastSeen: Date.now(), ip };
      this.tokens.set(hash(token), member); this.members.set(id, member);
      this.event('Entró ' + member.name); json(res, 200, { token, name: this.serverName, role: 'client' }); return;
    }
    const member = this.authenticate(req);
    if (!member) { this.errors++; json(res, 401, { error: 'Acceso privado: necesitas una invitación válida' }); return; }
    if (route.startsWith('/control/')) {
      if (route === '/control/ping' && req.method === 'GET') { json(res, 200, { time: Date.now(), address: ip }); return; }
      if (route === '/control/status' && req.method === 'GET') { json(res, 200, await this.status(member)); return; }
      if (route === '/control/leave' && req.method === 'POST') {
        for (const [token, entry] of this.tokens) if (entry.id === member.id) this.tokens.delete(token);
        this.members.delete(member.id); json(res, 200, { ok: true }); return;
      }
      if (member.role !== 'host') { json(res, 403, { error: 'Esta opción pertenece al anfitrión' }); return; }
      if (route === '/control/invitations' && req.method === 'POST') {
        if (this.state.invitations.filter(item => !item.revoked && item.expires > Date.now()).length >= 100)
          { json(res, 409, { error: 'Límite de 100 invitaciones activas' }); return; }
        const data = JSON.parse((await body(req)).toString()); const secret = random();
        const invite = { id: crypto.randomBytes(16).toString('hex'), hash: hash(secret), label: String(data.label || 'Amigo').slice(0, 64),
          expires: Date.now() + Math.max(1, Math.min(720, Number(data.hours) || 168)) * 3600000, revoked: false };
        this.state.invitations.push(invite); save(this.stateFile, this.state);
        const invitation = 'emulos360://join/' + Buffer.from(JSON.stringify({ endpoint: this.endpoint,
          fingerprint: this.fingerprint, id: invite.id, secret })).toString('base64url');
        this.event('Invitación creada para ' + invite.label); json(res, 200, { invitation, id: invite.id }); return;
      }
      if (route === '/control/revoke' && req.method === 'POST') {
        const data = JSON.parse((await body(req)).toString());
        const invite = this.state.invitations.find(item => item.id === data.id);
        if (!invite) { json(res, 404, { error: 'Invitación desconocida' }); return; }
        invite.revoked = true;
        for (const [token, entry] of this.tokens) if (entry.inviteId === invite.id) { this.tokens.delete(token); this.members.delete(entry.id); }
        save(this.stateFile, this.state); this.event('Acceso revocado: ' + invite.label); json(res, 200, { ok: true }); return;
      }
      if (route === '/control/permissions' && req.method === 'POST') {
        const data = JSON.parse((await body(req)).toString()); this.state.shareStats = data.shareStats === true;
        save(this.stateFile, this.state); json(res, 200, { ok: true }); return;
      }
      if (route === '/control/cleanup' && req.method === 'POST') {
        const result = this.db ? await this.db.collection('sessions').deleteMany({ $or: [{ deleted: true },
          { updatedAt: { $lt: new Date(Date.now() - 3600000) } }] }) : { deletedCount: 0 };
        this.roomCache = null; this.event('Limpieza de sesiones caducadas: ' + result.deletedCount);
        json(res, 200, { removed: result.deletedCount }); return;
      }
      json(res, 404, { error: 'Operación desconocida' }); return;
    }
    if (route === '/sessions' && req.method === 'GET') { json(res, 200, await this.rooms()); return; }
    // Only the emulator REST surface is forwarded; no Swagger, filesystem or administration routes.
    if (!/^\/(players(?:\/|$)|title\/|whoami$|DeleteSessions(?:\/|$)|xstorage\/)/.test(route) ||
        !['GET', 'POST', 'DELETE', 'PUT', 'HEAD'].includes(req.method)) {
      json(res, 404, { error: 'Ruta no disponible' }); return;
    }
    // Preserve the real address through the loopback backend, stripping user supplied forwarding headers.
    const realIP = ip === '127.0.0.1' ? this.advertisedIP : ip;
    if (route.startsWith('/DeleteSessions')) url.searchParams.set('hostAddress', realIP);
    const data = await body(req);
    const result = await request(`http://127.0.0.1:${this.backendPort}${route}${url.search}`, {
      method: req.method, headers: { 'Content-Type': req.headers['content-type'] || 'application/json',
        'X-Forwarded-For': realIP, 'X-Real-IP': realIP },
    }, data);
    if (req.method !== 'GET') this.roomCache = null;
    if (result.status >= 500) this.errors++;
    res.writeHead(result.status, { 'Content-Type': result.headers['content-type'] || 'application/json', 'Cache-Control': 'no-store' });
    res.end(result.body);
  }
  async rooms() {
    if (this.roomCache && Date.now() - this.roomCache.time < 1000) return this.roomCache.value;
    if (!this.db) return { Titles: [] };
    const sessions = await this.db.collection('sessions').find({ deleted: false, advertised: true }).limit(1000).toArray();
    const xuids = [...new Set(sessions.flatMap(session => [session.xuid, ...Object.keys(session.players || {})]))];
    const players = await this.db.collection('players').find({ xuid: { $in: xuids } }).project({ xuid: 1, gamertag: 1, richPresence: 1 }).toArray();
    const byXuid = new Map(players.map(player => [player.xuid, player])); const titles = new Map();
    for (const session of sessions) {
      const titleId = session.titleId.toUpperCase().padStart(8, '0');
      if (!titles.has(titleId)) titles.set(titleId, { titleId, name: session.title || titleId, sessions: [] });
      titles.get(titleId).sessions.push({ host_gamertag: byXuid.get(session.xuid)?.gamertag || session.xuid,
        host_presence: byXuid.get(session.xuid)?.richPresence || '', mediaId: session.mediaId, version: session.version,
        total: session.publicSlotsCount + session.privateSlotsCount,
        players: Object.entries(session.players || {}).filter(([, active]) => active).map(([xuid]) => ({ gamertag: byXuid.get(xuid)?.gamertag || xuid })) });
    }
    const value = { Titles: [...titles.values()] }; this.roomCache = { time: Date.now(), value }; return value;
  }
  async status(member) {
    const rooms = await this.rooms(); const sessions = rooms.Titles.flatMap(title => title.sessions);
    const common = { name: this.serverName, role: member.role, endpoint: this.endpoint,
      uptime: Math.floor((Date.now() - this.started) / 1000), rooms: sessions.length,
      shareStats: this.state.shareStats, participants: [...this.members.values()].filter(entry => Date.now() - entry.lastSeen < 30000)
        .map(entry => ({ id: entry.id, name: entry.name, online: true })),
      players: sessions.reduce((sum, session) => sum + session.players.length, 0) };
    if (member.role === 'host' || this.state.shareStats) {
      const records = this.db ? await this.db.collection('leaderboards').find({}).project({ _id: 0 }).limit(100).toArray() : [];
      const owners = this.db && records.length ? await this.db.collection('players').find({ xuid: { $in: records.map(record => record.player) } }).project({ xuid: 1, gamertag: 1 }).toArray() : [];
      common.leaderboards = records.map(record => {
        let definitions = {};
        if (/^[a-f0-9]{8}$/i.test(record.titleId)) {
          const file = path.join(this.directory, 'upstream/src/titles', record.titleId.toUpperCase(), 'stats.json');
          try { definitions = JSON.parse(fs.readFileSync(file)).properties || {}; } catch {}
        }
        const values = Object.entries(record.stats || {}).map(([id, stat]) => {
          const definition = Object.values(definitions).find(item => String(item.statId) === id);
          return `${definition?.info || 'Estadística ' + id}: ${stat?.value ?? stat}`;
        }).join(' · ');
        return { titleId: record.titleId, board: record.id,
          player: owners.find(owner => owner.xuid === record.player)?.gamertag || record.player, values };
      });
    }
    if (member.role === 'host') Object.assign(common, { requests: this.requests, errors: this.errors,
      memoryMB: Math.round(process.memoryUsage().rss / 1048576), eventLoopMs: Number.isFinite(this.delay.mean) ? Math.round(this.delay.mean / 1e6) : 0,
      upnp: this.upnpStatus, events: this.events, invitations: this.state.invitations.map(({ hash: ignored, ...item }) => item) });
    else if (!this.state.shareStats) { delete common.players; delete common.participants; }
    return common;
  }
  async remote(route, method = 'GET', data) {
    const result = await request(this.connectEndpoint + route, { method, agent: this.agent,
      headers: { Authorization: 'Bearer ' + this.token, 'Content-Type': 'application/json' } }, data ? JSON.stringify(data) : undefined);
    let value; try { value = JSON.parse(result.body); } catch { throw new Error('Respuesta incompatible del servidor'); }
    if (result.status !== 200) throw new Error(value.error || `Error HTTP ${result.status}`);
    return value;
  }
  async startBridge() {
    const secret = random();
    this.bridge = http.createServer((req, res) => {
      if (!req.url.startsWith('/' + secret + '/')) { json(res, 403, { error: 'Acceso local protegido' }); return; }
      const route = req.url.slice(secret.length + 1);
      const target = new URL(this.connectEndpoint + route);
      const proxy = https.request(target, { method: req.method, agent: this.agent, timeout: 20000,
        headers: { Authorization: 'Bearer ' + this.token, 'Content-Type': req.headers['content-type'] || 'application/json' } }, response => {
        res.writeHead(response.statusCode, { 'Content-Type': response.headers['content-type'] || 'application/json' }); response.pipe(res);
      });
      let size = 0;
      req.on('data', chunk => { size += chunk.length; if (size > MAX_BODY) { proxy.destroy(); json(res, 413, { error: 'Petición excesiva' }); } });
      proxy.on('error', () => json(res, 502, { error: 'Servidor privado inaccesible' }));
      proxy.on('timeout', () => proxy.destroy(new Error('Timeout')));
      req.on('aborted', () => proxy.destroy()); req.pipe(proxy);
    });
    this.bridge.requestTimeout = 25000; this.bridge.headersTimeout = 10000;
    await new Promise((resolve, reject) => { this.bridge.once('error', reject); this.bridge.listen(0, '127.0.0.1', resolve); });
    this.bridgeURL = `http://127.0.0.1:${this.bridge.address().port}/${secret}/`;
  }
  async publish() {
    if (this.publishing || this.stopping) return;
    this.publishing = true;
    try {
      const start = performance.now(); await this.remote('/control/ping');
      this.samples.push(performance.now() - start); this.samples = this.samples.slice(-20);
      const status = await this.remote('/control/status');
      const latency = this.samples.reduce((sum, value) => sum + value, 0) / this.samples.length;
      status.latencyMs = Math.round(latency);
      status.jitterMs = Math.round(this.samples.reduce((sum, value) => sum + Math.abs(value - latency), 0) / this.samples.length);
      status.connected = true; this.emit({ event: 'status', status });
    } catch (error) { this.emit({ event: 'disconnected', message: error.message }); }
    finally { this.publishing = false; }
  }
  async command(command) {
    if (command.action === 'invite') {
      const value = await this.remote('/control/invitations', 'POST', { label: command.label, hours: command.hours });
      this.emit({ event: 'invitation', ...value });
    } else if (command.action === 'revoke') await this.remote('/control/revoke', 'POST', { id: command.id });
    else if (command.action === 'permissions') await this.remote('/control/permissions', 'POST', { shareStats: command.shareStats });
    else if (command.action === 'cleanup') await this.remote('/control/cleanup', 'POST', {});
    else if (command.action === 'public-ip') {
      const result = await request('https://api.ipify.org?format=json');
      const address = JSON.parse(result.body).ip;
      if (!net.isIPv4(address)) throw new Error('No se recibió una dirección IPv4');
      this.emit({ event: 'public-ip', address });
    }
    await this.publish();
  }
  async close() {
    if (this.stopping) return;
    this.stopping = true; clearInterval(this.interval); clearInterval(this.renew); this.delay.disable();
    if (this.config?.mode === 'client' && this.token) await this.remote('/control/leave', 'POST', {}).catch(() => {});
    if (this.mapped) await new Promise(resolve => this.upnp.portUnmapping({ public: this.config.port, protocol: 'TCP' }, () => resolve()));
    this.upnp?.close(); this.agent?.destroy();
    for (const server of [this.gateway, this.bridge]) if (server) { server.close(); server.closeAllConnections(); }
    if (this.mongo) {
      await this.mongo.db('admin').command({ shutdown: 1 }).catch(() => {});
      await this.mongo.close().catch(() => {});
    }
    for (const child of this.children) if (child.exitCode === null) child.kill();
    this.emit({ event: 'stopped' });
  }
}

if (require.main === module) {
  const readline = require('node:readline');
  const service = new PrivateService(__dirname, value => process.stdout.write(JSON.stringify(value) + '\n'));
  let chain = Promise.resolve();
  const input = readline.createInterface({ input: process.stdin, terminal: false });
  input.on('line', line => {
    chain = chain.then(async () => {
      if (line.length > 16384) throw new Error('Orden demasiado grande');
      const command = JSON.parse(line);
      if (command.action === 'start') await service.start(command);
      else if (command.action === 'stop') { await service.close(); process.exit(0); }
      else await service.command(command);
    }).catch(async error => {
      service.emit({ event: 'error', message: error.message });
      if (!service.bridge) { await service.close(); process.exit(1); }
    });
  });
  input.on('close', () => service.close().finally(() => process.exit(0)));
  process.on('SIGTERM', () => service.close().finally(() => process.exit(0)));
  process.on('SIGINT', () => service.close().finally(() => process.exit(0)));
  process.on('uncaughtException', error => { service.emit({ event: 'error', message: error.message }); service.close().finally(() => process.exit(1)); });
}
module.exports = { PrivateService, parseInvitation, pinnedAgent, request };
