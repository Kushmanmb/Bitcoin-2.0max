import { test, before, after } from 'node:test';
import assert from 'node:assert/strict';
import { mkdtempSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { createApp } from './server.mjs';

const directory = mkdtempSync(join(tmpdir(), 'b2mx-dashboard-test-'));
const database = join(directory, 'accounts.sqlite');
const fakeMarket = async url => ({ ok: true, json: async () => ({ data: { currency: 'USD', amount: url.includes('BTC') ? '60000' : '3000' } }) });
let server, origin, alice, bob;
const password = 'Correct-horse-orbit-42';
const moderatorIds = [];
async function start() {
  server = createApp({ database, marketFetch: fakeMarket, moderatorIds });
  await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
  origin = `http://127.0.0.1:${server.address().port}`;
}
async function stop() { await new Promise(resolve => server.close(resolve)); }
async function request(path, { method = 'GET', value, cookie, extraHeaders = {} } = {}) {
  const result = await fetch(`${origin}/api/account/${path}`, {
    method, headers: { Origin: origin, 'Content-Type': 'application/json', 'X-B2MX-Request': 'dashboard',
      ...(cookie ? { Cookie: cookie } : {}), ...extraHeaders }, body: value == null ? undefined : JSON.stringify(value)
  });
  return { status: result.status, body: await result.json(), cookie: result.headers.get('set-cookie')?.split(';')[0], headers: result.headers };
}
before(async () => {
  await start();
  const a = await request('register', { method: 'POST', value: { name: 'Alice', email: 'alice@example.com', password } });
  assert.equal(a.status, 201); alice = a.cookie;
  moderatorIds.push(a.body.user.id);
  assert.match(a.headers.get('set-cookie'), /HttpOnly/); assert.match(a.headers.get('set-cookie'), /SameSite=Strict/);
  const b = await request('register', { method: 'POST', value: { name: 'Bob', email: 'bob@example.com', password } });
  assert.equal(b.status, 201); bob = b.cookie;
});
after(async () => { await stop(); rmSync(directory, { recursive: true, force: true }); });

test('Sessions protect every private collection and do not expose hashes', async () => {
  assert.equal((await request('items')).status, 401);
  const me = await request('me', { cookie: alice }); assert.equal(me.status, 200);
  assert.equal(me.body.user.name, 'Alice'); assert.equal(me.body.user.moderator, true);
  assert.equal(me.body.user.password, undefined); assert.equal(me.body.user.salt, undefined);
  assert.equal((await request('me', { cookie: 'b2mx_session=not-a-session' })).status, 401);
});
test('Mutations require both a same-origin request and the request header', async () => {
  assert.equal((await request('items', { method: 'POST', cookie: alice, value: {}, extraHeaders: { Origin: 'https://evil.example' } })).status, 403);
  assert.equal((await request('items', { method: 'POST', cookie: alice, value: {}, extraHeaders: { 'X-B2MX-Request': '' } })).status, 403);
});
test('Accounts can save, edit, export and delete only their own records', async () => {
  const saved = await request('items', { method: 'POST', cookie: alice, value: {
    type: 'addresses', item: { label: 'My ETH', network: 'ETH', value: '0x' + 'a'.repeat(40), note: 'Personal bookmark' }
  } });
  assert.equal(saved.status, 201);
  assert.equal((await request('items', { cookie: bob })).body.addresses.length, 0);
  assert.equal((await request(`items/${saved.body.id}`, { method: 'DELETE', cookie: bob })).status, 404);
  assert.equal((await request(`items/${saved.body.id}`, { method: 'PUT', cookie: bob, value: { item: {} } })).status, 404);
  const edited = await request(`items/${saved.body.id}`, { method: 'PUT', cookie: alice, value: { item: { ...saved.body, label: 'Updated ETH' } } });
  assert.equal(edited.status, 200);
  const exported = await request('export', { cookie: alice });
  assert.equal(exported.body.data.addresses[0].label, 'Updated ETH');
  assert.equal(exported.body.user.password, undefined);
  assert.equal((await request(`items/${saved.body.id}`, { method: 'DELETE', cookie: alice })).status, 200);
});
test('All six workspace features persist and reject malformed asset data', async () => {
  const fixtures = {
    holdings: { symbol: 'BTC', quantity: '0.25', averageCost: '40000', manualPrice: '' },
    transactions: { label: 'A transaction', network: 'BTC', value: 'b'.repeat(64), note: '' },
    watchlist: { symbol: 'ETH', target: '3500', direction: 'above' },
    goals: { label: 'My first milestone', target: '25000' },
    notes: { label: '<script>research</script>', body: 'Notes stay text, even <img src=x onerror=alert(1)>' }
  };
  for (const [type, item] of Object.entries(fixtures))
    assert.equal((await request('items', { method: 'POST', cookie: bob, value: { type, item } })).status, 201);
  const rows = (await request('items', { cookie: bob })).body;
  assert.equal(rows.holdings[0].quantity, '0.25'); assert.equal(rows.holdings[0].manualPrice, null);
  assert.equal(rows.notes[0].label, '<script>research</script>');
  assert.equal((await request('items', { method: 'POST', cookie: bob, value: { type: 'holdings', item: { symbol: 'BTC', quantity: '-1', averageCost: '0' } } })).status, 400);
  assert.equal((await request('items', { method: 'POST', cookie: bob, value: { type: 'transactions', item: { label: 'bad', network: 'ETH', value: 'x', note: '' } } })).status, 400);
});
test('Chat rooms, spam limits, reporting and moderator ownership rules work', async () => {
  assert.equal((await request('chat?room=lounge')).status, 401);
  const sent = await request('chat', { method: 'POST', cookie: bob, value: { room: 'builders', body: 'Hello, builders! <script>not HTML</script>' } });
  assert.equal(sent.status, 201); assert.equal(sent.body.author, 'Bob');
  assert.equal((await request('chat?room=lounge', { cookie: alice })).body.messages.length, 0);
  const room = (await request('chat?room=builders', { cookie: alice })).body;
  assert.equal(room.messages[0].body, sent.body.body); assert.equal(room.messages[0].email, undefined);
  assert.equal((await request('chat', { method: 'POST', cookie: bob, value: { room: 'builders', body: 'Too soon' } })).status, 429);
  assert.equal((await request(`chat/${sent.body.id}/report`, { method: 'POST', cookie: alice, value: {} })).status, 200);
  assert.equal((await request('reports', { cookie: bob })).status, 403);
  assert.equal((await request('reports', { cookie: alice })).body.reports.length, 1);
  assert.equal((await request(`chat/${sent.body.id}`, { method: 'DELETE', cookie: alice })).status, 200);
  const aliceMessage = await request('chat', { method: 'POST', cookie: alice, value: { room: 'lounge', body: 'A moderator message' } });
  assert.equal(aliceMessage.status, 201);
  assert.equal((await request(`chat/${aliceMessage.body.id}`, { method: 'DELETE', cookie: bob })).status, 403);
  assert.equal((await request(`chat/${aliceMessage.body.id}`, { method: 'DELETE', cookie: alice })).status, 200);
});
test('Market returns only valid upstream quotes and never invents a B2MX price', async () => {
  const market = await request('market', { cookie: alice });
  assert.equal(market.body.quotes.BTC.price, 60000); assert.equal(market.body.quotes.ETH.price, 3000);
  assert.equal(market.body.quotes.B2MX, undefined);
});
test('Passwords change with current-password proof and revoke other sessions', async () => {
  const second = await request('login', { method: 'POST', value: { email: 'bob@example.com', password } });
  assert.equal(second.status, 200);
  assert.equal((await request('password', { method: 'POST', cookie: bob, value: { currentPassword: 'wrong-password-42', newPassword: 'Different-strong-password-42' } })).status, 401);
  const changed = await request('password', { method: 'POST', cookie: bob, value: { currentPassword: password, newPassword: 'Different-strong-password-42' } });
  assert.equal(changed.status, 200); assert.equal((await request('me', { cookie: second.cookie })).status, 401);
  assert.equal((await request('me', { cookie: bob })).status, 401); bob = changed.cookie;
  assert.equal((await request('login', { method: 'POST', value: { email: 'bob@example.com', password } })).status, 401);
});
test('Data and sessions survive a service restart', async () => {
  await stop(); await start();
  const saved = await request('items', { cookie: bob }); assert.equal(saved.status, 200); assert.equal(saved.body.holdings.length, 1);
});
test('Private database and code cannot be served; logout invalidates session', async () => {
  for (const path of ['/data/dashboard/accounts.sqlite', '/dashboard-server/server.mjs', '/.git/config']) {
    const result = await fetch(origin + path); assert.equal(result.status, 404);
  }
  const page = await fetch(origin + '/dashboard.html'); assert.equal(page.status, 200);
  assert.match(page.headers.get('content-security-policy'), /script-src 'self'/);
  assert.equal((await request('logout', { method: 'POST', cookie: bob, value: {} })).status, 200);
  assert.equal((await request('me', { cookie: bob })).status, 401);
});
