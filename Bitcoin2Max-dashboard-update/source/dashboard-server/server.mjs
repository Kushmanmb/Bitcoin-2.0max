import http from 'node:http';
import { DatabaseSync } from 'node:sqlite';
import { randomBytes, randomUUID, scrypt, timingSafeEqual, createHash } from 'node:crypto';
import { promisify } from 'node:util';
import { mkdirSync, chmodSync, readFileSync, realpathSync } from 'node:fs';
import { resolve, dirname, extname, sep } from 'node:path';
import { fileURLToPath } from 'node:url';

const root = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const derive = promisify(scrypt);
const SESSION_AGE = 7 * 24 * 60 * 60 * 1000;
const rooms = ['lounge', 'bitcoin', 'ethereum', 'builders'];
const symbols = ['BTC', 'ETH', 'B2MX'];
const types = ['addresses', 'transactions', 'holdings', 'watchlist', 'goals', 'notes'];
const hashToken = token => createHash('sha256').update(token).digest('hex');

class ApiError extends Error {
  constructor(status, message) { super(message); this.status = status; }
}
function text(value, name, max = 120, required = true) {
  if (typeof value !== 'string' || value.length > max || /[\u0000-\u0008\u000b\u000c\u000e-\u001f]/.test(value))
    throw new ApiError(400, `Invalid ${name}.`);
  const cleaned = value.trim();
  if (required && !cleaned) throw new ApiError(400, `${name} is required.`);
  return cleaned;
}
function amount(value, name, optional = false) {
  if (optional && (value === '' || value == null)) return null;
  const cleaned = text(String(value ?? ''), name, 45);
  if (!/^\d+(\.\d{1,18})?$/.test(cleaned) || !Number.isFinite(Number(cleaned)) || Number(cleaned) > 1e12)
    throw new ApiError(400, `${name} must be a nonnegative number, up to 18 decimal places.`);
  return cleaned;
}
function choice(value, allowed, name) {
  if (!allowed.includes(value)) throw new ApiError(400, `Invalid ${name}.`);
  return value;
}
function itemPayload(type, raw) {
  if (!raw || typeof raw !== 'object' || Array.isArray(raw)) throw new ApiError(400, 'Invalid item.');
  const label = () => text(raw.label, 'Label', 80);
  if (type === 'addresses' || type === 'transactions') {
    const network = choice(raw.network, symbols, 'network');
    const value = text(raw.value, type === 'addresses' ? 'Address' : 'Transaction hash', 128);
    if (type === 'transactions' && !(network === 'ETH' ? /^0x[0-9a-fA-F]{64}$/ : /^[0-9a-fA-F]{64}$/).test(value))
      throw new ApiError(400, 'Enter a complete transaction hash for this network.');
    if (type === 'addresses') {
      const valid = network === 'ETH' ? /^0x[0-9a-fA-F]{40}$/.test(value) :
        network === 'BTC' ? /^(bc1[a-zA-HJ-NP-Z0-9]{11,87}|[13][a-km-zA-HJ-NP-Z1-9]{25,34})$/.test(value) :
        /^[a-zA-Z0-9]{8,128}$/.test(value);
      if (!valid) throw new ApiError(400, 'Address format does not match the selected network.');
    }
    return { label: label(), network, value, note: text(raw.note ?? '', 'Note', 500, false) };
  }
  if (type === 'holdings') return {
    symbol: choice(raw.symbol, symbols, 'asset'),
    quantity: amount(raw.quantity, 'Quantity'),
    averageCost: amount(raw.averageCost, 'Average cost'),
    manualPrice: amount(raw.manualPrice, 'Manual price', true)
  };
  if (type === 'watchlist') return {
    symbol: choice(raw.symbol, symbols, 'asset'),
    target: amount(raw.target, 'Target price', true),
    direction: choice(raw.direction ?? 'above', ['above', 'below'], 'target direction')
  };
  if (type === 'goals') return { label: label(), target: amount(raw.target, 'Goal amount') };
  if (type === 'notes') return { label: label(), body: text(raw.body, 'Note', 2000) };
  throw new ApiError(400, 'Invalid item type.');
}
async function passwordHash(password, salt) {
  // OWASP scrypt baseline: N=2^17, r=8, p=1; never store plaintext passwords.
  return Buffer.from(await derive(password, salt, 64, { N: 131072, r: 8, p: 1, maxmem: 192 * 1024 * 1024 }));
}
function passwordInput(value) {
  if (typeof value !== 'string' || value.length < 12 || value.length > 128)
    throw new ApiError(400, 'Use a password between 12 and 128 characters.');
  return value;
}
async function body(request) {
  if (!/^application\/json(?:;|$)/i.test(request.headers['content-type'] ?? ''))
    throw new ApiError(415, 'Send JSON data.');
  let bytes = 0;
  const chunks = [];
  for await (const chunk of request) {
    bytes += chunk.length;
    if (bytes > 65536) throw new ApiError(413, 'Request is too large.');
    chunks.push(chunk);
  }
  try {
    const parsed = JSON.parse(Buffer.concat(chunks).toString('utf8'));
    if (!parsed || typeof parsed !== 'object' || Array.isArray(parsed)) throw new Error();
    return parsed;
  } catch { throw new ApiError(400, 'Invalid JSON.'); }
}

export function createApp({ database = resolve(root, 'data/dashboard/accounts.sqlite'), origin,
  docs = resolve(root, 'docs'), marketFetch = fetch,
  moderatorIds = (process.env.DASHBOARD_MODERATOR_IDS ?? '').split(',').map(v => v.trim()).filter(Boolean) } = {}) {
  if (origin && (new URL(origin).origin !== origin || !/^https?:/.test(origin))) throw new Error('APP_ORIGIN must be an exact http(s) origin without a trailing slash.');
  if (database !== ':memory:') {
    mkdirSync(dirname(database), { recursive: true, mode: 0o700 });
    chmodSync(dirname(database), 0o700);
  }
  const db = new DatabaseSync(database);
  if (database !== ':memory:') chmodSync(database, 0o600);
  db.exec(`PRAGMA foreign_keys = ON; PRAGMA journal_mode = WAL;
    CREATE TABLE IF NOT EXISTS users (id TEXT PRIMARY KEY, email TEXT UNIQUE NOT NULL, name TEXT NOT NULL,
      salt TEXT NOT NULL, password BLOB NOT NULL, created INTEGER NOT NULL);
    CREATE TABLE IF NOT EXISTS sessions (token TEXT PRIMARY KEY, user_id TEXT NOT NULL REFERENCES users(id) ON DELETE CASCADE, expires INTEGER NOT NULL);
    CREATE TABLE IF NOT EXISTS items (id TEXT PRIMARY KEY, user_id TEXT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
      type TEXT NOT NULL, payload TEXT NOT NULL, created INTEGER NOT NULL);
    CREATE INDEX IF NOT EXISTS items_owner ON items(user_id, type);
    CREATE TABLE IF NOT EXISTS messages (id INTEGER PRIMARY KEY AUTOINCREMENT, user_id TEXT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
      room TEXT NOT NULL, body TEXT NOT NULL, created INTEGER NOT NULL);
    CREATE INDEX IF NOT EXISTS messages_room ON messages(room, id);
    CREATE TABLE IF NOT EXISTS reports (id INTEGER PRIMARY KEY, message_id INTEGER NOT NULL REFERENCES messages(id) ON DELETE CASCADE,
      user_id TEXT NOT NULL REFERENCES users(id) ON DELETE CASCADE, created INTEGER NOT NULL, UNIQUE(message_id, user_id));`);
  const limits = new Map();
  const secure = origin?.startsWith('https://') ?? false;
  const cookieName = secure ? '__Host-b2mx_session' : 'b2mx_session';
  let activeHashes = 0, priceCache = null, pricePromise = null;
  const dummySalt = randomBytes(16).toString('hex');
  function throttle(key, maximum, interval) {
    const now = Date.now();
    // Bound memory used by rate-limit bookkeeping; expirations are opportunistic.
    if (limits.size > 10000) for (const [k, v] of limits) if (v.until <= now) limits.delete(k);
    let bucket = limits.get(key);
    if (!bucket || bucket.until <= now) { bucket = { count: 0, until: now + interval }; limits.set(key, bucket); }
    if (++bucket.count > maximum) throw new ApiError(429, 'Too many requests. Please try again shortly.');
  }
  async function deriveBounded(password, salt) {
    if (activeHashes >= 2) throw new ApiError(503, 'Account service is busy. Please try again.');
    ++activeHashes;
    try { return await passwordHash(password, salt); } finally { --activeHashes; }
  }
  const json = (response, status, payload) => {
    response.writeHead(status, { 'Content-Type': 'application/json; charset=utf-8', 'Cache-Control': 'no-store' });
    response.end(JSON.stringify(payload));
  };
  function cookie(response, token, clear = false) {
    response.setHeader('Set-Cookie', `${cookieName}=${token}; Path=/; HttpOnly; SameSite=Strict; Max-Age=${clear ? 0 : SESSION_AGE / 1000}${secure ? '; Secure' : ''}`);
  }
  function tokenFrom(request) {
    const raw = (request.headers.cookie ?? '').split(';').map(v => v.trim()).find(v => v.startsWith(`${cookieName}=`));
    const token = raw?.slice(cookieName.length + 1) ?? '';
    return /^[a-f0-9]{64}$/.test(token) ? token : '';
  }
  function userFrom(request) {
    const token = tokenFrom(request);
    if (!token) throw new ApiError(401, 'Please sign in.');
    const user = db.prepare('SELECT u.* FROM users u JOIN sessions s ON s.user_id=u.id WHERE s.token=? AND s.expires>?').get(hashToken(token), Date.now());
    if (!user) throw new ApiError(401, 'Your session expired. Please sign in again.');
    return user;
  }
  function publicUser(user) { return { id: user.id, name: user.name, email: user.email, moderator: moderatorIds.includes(user.id) }; }
  function newSession(response, user) {
    db.prepare('DELETE FROM sessions WHERE expires<=?').run(Date.now());
    const token = randomBytes(32).toString('hex');
    db.prepare('INSERT INTO sessions VALUES(?,?,?)').run(hashToken(token), user.id, Date.now() + SESSION_AGE);
    cookie(response, token);
  }
  function allItems(user) {
    const data = Object.fromEntries(types.map(type => [type, []]));
    for (const row of db.prepare('SELECT id,type,payload,created FROM items WHERE user_id=? ORDER BY created DESC').all(user.id))
      data[row.type].push({ id: row.id, ...JSON.parse(row.payload), created: row.created });
    return data;
  }
  async function market() {
    if (priceCache && Date.now() - priceCache.checked < 60000) return priceCache;
    if (pricePromise) return pricePromise;
    pricePromise = (async () => {
      const quotes = {};
      await Promise.all(['BTC', 'ETH'].map(async symbol => {
        try {
          const response = await marketFetch(`https://api.coinbase.com/v2/prices/${symbol}-USD/spot`, { signal: AbortSignal.timeout(5000) });
          if (!response.ok) return;
          const data = await response.json();
          const value = Number(data.data?.amount);
          if (data.data?.currency === 'USD' && Number.isFinite(value) && value > 0)
            quotes[symbol] = { price: value, source: 'Coinbase spot', at: Date.now() };
        } catch { /* Missing prices stay missing; never invent a market quote. */ }
      }));
      priceCache = { quotes, checked: Date.now() };
      return priceCache;
    })().finally(() => { pricePromise = null; });
    return pricePromise;
  }
  const server = http.createServer(async (request, response) => {
    response.setHeader('X-Content-Type-Options', 'nosniff');
    response.setHeader('Referrer-Policy', 'same-origin');
    response.setHeader('X-Frame-Options', 'DENY');
    try {
      const url = new URL(request.url, 'http://localhost');
      if (url.pathname.startsWith('/api/account/')) {
        throttle(`api:${request.socket.remoteAddress}`, 300, 60000);
        if (!['GET', 'HEAD'].includes(request.method)) {
          const expected = origin ?? `http://${request.headers.host}`;
          if (request.headers.origin !== expected || request.headers['x-b2mx-request'] !== 'dashboard')
            throw new ApiError(403, 'Request origin is not allowed.');
        }
        const path = url.pathname.slice('/api/account/'.length);
        if (path === 'health' && request.method === 'GET') return json(response, 200, { ready: true, rooms, emailVerification: false });
        if (['register', 'login'].includes(path) && request.method === 'POST') {
          throttle(`auth:${request.socket.remoteAddress}`, 12, 15 * 60000);
          const input = await body(request);
          const email = text(input.email, 'Email', 254).toLowerCase();
          if (!/^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(email)) throw new ApiError(400, 'Enter a valid email address.');
          const password = passwordInput(input.password);
          let user;
          if (path === 'register') {
            const name = text(input.name, 'Display name', 40);
            const salt = randomBytes(16).toString('hex');
            const derived = await deriveBounded(password, salt);
            user = { id: randomUUID(), email, name, salt, password: derived, created: Date.now() };
            try { db.prepare('INSERT INTO users VALUES(?,?,?,?,?,?)').run(user.id, email, name, salt, derived, user.created); }
            catch (error) {
              if (error.code === 'ERR_SQLITE_ERROR' && db.prepare('SELECT id FROM users WHERE email=?').get(email))
                throw new ApiError(409, 'Unable to create this account. Try signing in.');
              throw error;
            }
          } else {
            user = db.prepare('SELECT * FROM users WHERE email=?').get(email);
            const derived = await deriveBounded(password, user?.salt ?? dummySalt);
            if (!user || !timingSafeEqual(derived, Buffer.from(user.password))) throw new ApiError(401, 'Email or password is incorrect.');
          }
          newSession(response, user);
          return json(response, path === 'register' ? 201 : 200, { user: publicUser(user) });
        }
        const user = userFrom(request);
        if (path === 'me' && request.method === 'GET') return json(response, 200, { user: publicUser(user) });
        if (path === 'logout' && request.method === 'POST') {
          db.prepare('DELETE FROM sessions WHERE token=?').run(hashToken(tokenFrom(request)));
          cookie(response, '', true); return json(response, 200, { ok: true });
        }
        if (path === 'password' && request.method === 'POST') {
          throttle(`password:${user.id}`, 5, 15 * 60000);
          const input = await body(request);
          const current = await deriveBounded(passwordInput(input.currentPassword), user.salt);
          if (!timingSafeEqual(current, Buffer.from(user.password))) throw new ApiError(401, 'Current password is incorrect.');
          const salt = randomBytes(16).toString('hex');
          const derived = await deriveBounded(passwordInput(input.newPassword), salt);
          db.prepare('UPDATE users SET salt=?,password=? WHERE id=?').run(salt, derived, user.id);
          db.prepare('DELETE FROM sessions WHERE user_id=?').run(user.id);
          newSession(response, user); return json(response, 200, { ok: true });
        }
        if (path === 'items' && request.method === 'GET') return json(response, 200, allItems(user));
        if (path === 'items' && request.method === 'POST') {
          const input = await body(request);
          const type = choice(input.type, types, 'item type');
          if (db.prepare('SELECT COUNT(*) AS count FROM items WHERE user_id=?').get(user.id).count >= 1000)
            throw new ApiError(400, 'Your account has reached its 1,000-item limit.');
          const payload = itemPayload(type, input.item);
          const id = randomUUID(), created = Date.now();
          db.prepare('INSERT INTO items VALUES(?,?,?,?,?)').run(id, user.id, type, JSON.stringify(payload), created);
          return json(response, 201, { id, ...payload, created });
        }
        const itemMatch = /^items\/([a-f0-9-]{36})$/.exec(path);
        if (itemMatch && ['PUT', 'DELETE'].includes(request.method)) {
          const row = db.prepare('SELECT * FROM items WHERE id=? AND user_id=?').get(itemMatch[1], user.id);
          if (!row) throw new ApiError(404, 'Item not found.');
          if (request.method === 'DELETE') db.prepare('DELETE FROM items WHERE id=? AND user_id=?').run(row.id, user.id);
          else {
            const input = await body(request), payload = itemPayload(row.type, input.item);
            db.prepare('UPDATE items SET payload=? WHERE id=? AND user_id=?').run(JSON.stringify(payload), row.id, user.id);
          }
          return json(response, 200, { ok: true });
        }
        if (path === 'export' && request.method === 'GET')
          return json(response, 200, { version: 1, exportedAt: new Date().toISOString(), user: publicUser(user), data: allItems(user) });
        if (path === 'market' && request.method === 'GET') return json(response, 200, await market());
        if (path === 'chat' && request.method === 'GET') {
          const room = choice(url.searchParams.get('room') ?? 'lounge', rooms, 'room');
          const messages = db.prepare(`SELECT m.id,m.room,m.body,m.created,u.id AS authorId,u.name AS author
            FROM messages m JOIN users u ON u.id=m.user_id WHERE m.room=? ORDER BY m.id DESC LIMIT 100`).all(room).reverse();
          return json(response, 200, { room, messages });
        }
        if (path === 'chat' && request.method === 'POST') {
          throttle(`chat:${user.id}`, 1, 3000);
          const input = await body(request);
          const room = choice(input.room, rooms, 'room'), message = text(input.body, 'Message', 1000);
          const created = Date.now();
          const result = db.prepare('INSERT INTO messages(user_id,room,body,created) VALUES(?,?,?,?)').run(user.id, room, message, created);
          return json(response, 201, { id: Number(result.lastInsertRowid), room, body: message, created, author: user.name, authorId: user.id });
        }
        const messageMatch = /^chat\/(\d+)(\/report)?$/.exec(path);
        if (messageMatch) {
          const row = db.prepare('SELECT * FROM messages WHERE id=?').get(messageMatch[1]);
          if (!row) throw new ApiError(404, 'Message not found.');
          if (messageMatch[2] && request.method === 'POST') {
            throttle(`reports:${user.id}`, 20, 60000);
            db.prepare('INSERT OR IGNORE INTO reports(message_id,user_id,created) VALUES(?,?,?)').run(row.id, user.id, Date.now());
            return json(response, 200, { ok: true });
          }
          if (!messageMatch[2] && request.method === 'DELETE') {
            if (row.user_id !== user.id && !moderatorIds.includes(user.id)) throw new ApiError(403, 'You can only delete your own messages.');
            db.prepare('DELETE FROM messages WHERE id=?').run(row.id); return json(response, 200, { ok: true });
          }
        }
        if (path === 'reports' && request.method === 'GET') {
          if (!moderatorIds.includes(user.id)) throw new ApiError(403, 'Moderator access required.');
          return json(response, 200, { reports: db.prepare(`SELECT m.id,m.room,m.body,u.name AS author,COUNT(r.id) AS count
            FROM reports r JOIN messages m ON m.id=r.message_id JOIN users u ON u.id=m.user_id
            GROUP BY m.id ORDER BY count DESC LIMIT 100`).all() });
        }
        throw new ApiError(404, 'Account endpoint not found.');
      }
      if (!['GET', 'HEAD'].includes(request.method)) throw new ApiError(405, 'Method not allowed.');
      // Only files beneath docs are public. DB, server code and dotfiles are never served.
      const decoded = decodeURIComponent(url.pathname);
      if (decoded.split('/').some(v => v.startsWith('.')) || decoded.includes('\\')) throw new ApiError(404, 'Not found.');
      const base = realpathSync(docs);
      const requested = resolve(base, `.${decoded === '/' ? '/index.html' : decoded}`);
      if (!requested.startsWith(base + sep)) throw new ApiError(404, 'Not found.');
      let file;
      try { file = realpathSync(requested); } catch { throw new ApiError(404, 'Not found.'); }
      if (!file.startsWith(base + sep)) throw new ApiError(404, 'Not found.');
      const mime = { '.html': 'text/html', '.css': 'text/css', '.js': 'text/javascript', '.png': 'image/png', '.jpg': 'image/jpeg', '.svg': 'image/svg+xml', '.json': 'application/json' }[extname(file).toLowerCase()] ?? 'application/octet-stream';
      if (file.endsWith('dashboard.html')) response.setHeader('Content-Security-Policy', "default-src 'self'; script-src 'self'; style-src 'self'; img-src 'self' data:; connect-src 'self'; frame-ancestors 'none'; base-uri 'self'; form-action 'self'");
      response.writeHead(200, { 'Content-Type': mime, 'Cache-Control': 'no-cache' });
      response.end(request.method === 'HEAD' ? undefined : readFileSync(file));
    } catch (error) {
      if (!response.headersSent) json(response, error.status ?? 500, { error: error.status ? error.message : 'Server error. Please try again.' });
      else response.end();
      if (!error.status) console.error('Dashboard request failed:', error.message);
    }
  });
  server.on('close', () => db.close());
  return server;
}

if (process.argv[1] && resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  const port = Number(process.env.PORT ?? 3000);
  const origin = process.env.APP_ORIGIN ?? `http://localhost:${port}`;
  if (process.env.NODE_ENV === 'production' && !origin.startsWith('https://')) throw new Error('Production requires an HTTPS APP_ORIGIN.');
  const app = createApp({ origin, database: process.env.DASHBOARD_DATABASE });
  app.listen(port, process.env.HOST ?? '127.0.0.1', () => console.log(`Bitcoin2Max dashboard: ${origin}/dashboard.html`));
  const shutdown = () => app.close(() => process.exit(0));
  process.on('SIGTERM', shutdown); process.on('SIGINT', shutdown);
}
