(() => {
  'use strict';
  const $ = id => document.getElementById(id);
  const names = { BTC: 'Bitcoin', ETH: 'Ethereum', B2MX: 'Bitcoin2.0Max' };
  const glyphs = { BTC: '₿', ETH: 'Ξ', B2MX: 'M' };
  const colors = { BTC: '#ffb769', ETH: '#ac9aff', B2MX: '#57e5ce' };
  const types = ['holdings', 'addresses', 'transactions', 'watchlist', 'goals', 'notes'];
  const titles = { overview: ['THE BIG PICTURE', 'Your universe, at a glance.', 'A little clarity. A lot of possibility.'],
    holdings: ['YOUR ASSETS', 'Build your bigger picture.', 'Track holdings you enter. This dashboard does not move funds.'],
    addresses: ['YOUR CONNECTIONS', 'An address for everything.', 'Organize public addresses. Never save private keys or seed phrases.'],
    transactions: ['YOUR TRAIL', 'Keep the important moments.', 'Bookmark transaction hashes and open them in a network explorer.'],
    watchlist: ['ON YOUR RADAR', 'Watch what matters.', 'Price targets are checked while this dashboard is open.'],
    goals: ['YOUR NEXT CHAPTER', 'Give your ambition a number.', 'Set portfolio value goals and see your progress.'],
    notes: ['YOUR IDEAS', 'A little space to think.', 'Keep research, reminders and plans in one place.'],
    chat: ['BETTER TOGETHER', 'Find your people.', 'Four rooms. Fresh perspectives. One community.'],
    settings: ['MAKE IT YOURS', 'Your space. Your settings.', 'Manage your account and take your saved workspace with you.'] };
  const singular = { holdings: 'holding', addresses: 'address', transactions: 'transaction', watchlist: 'watch', goals: 'goal', notes: 'note' };
  let user = null, data = {}, quotes = {}, view = 'overview', authMode = 'login', editing = null;
  let room = 'lounge', chatTimer = null, marketTimer = null, chatBusy = false, search = '', chatSignature = '', toastTimer;
  const usd = value => new Intl.NumberFormat('en-US', { style: 'currency', currency: 'USD', maximumFractionDigits: 2 }).format(value);
  const quantity = value => new Intl.NumberFormat('en-US', { maximumFractionDigits: 8 }).format(Number(value));
  const escape = value => String(value ?? '').replace(/[&<>"']/g, c => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[c]));
  // All values interpolated into markup are escaped; generated URLs are fixed-origin explorer URLs.
  const sensitive = value => `<span class="sensitive">${escape(value)}</span>`;
  const coin = symbol => `<span class="coin ${symbol}">${glyphs[symbol]}</span>`;
  const symbolOptions = '<option value="BTC">Bitcoin · BTC</option><option value="ETH">Ethereum · ETH</option><option value="B2MX">Bitcoin2.0Max · B2MX</option>';
  async function api(path, { method = 'GET', value } = {}) {
    const response = await fetch(`/api/account/${path}`, {
      method, credentials: 'same-origin', headers: { 'Content-Type': 'application/json', 'X-B2MX-Request': 'dashboard' },
      body: value === undefined ? undefined : JSON.stringify(value)
    });
    let result;
    try { result = await response.json(); } catch { throw new Error('Account service is unavailable. Open the dashboard on its account server.'); }
    if (!response.ok) {
      if (response.status === 401 && user && path !== 'password') showAuth();
      throw new Error(result.error ?? 'The request could not be completed.');
    }
    return result;
  }
  function toast(message) {
    clearTimeout(toastTimer); $('toast').textContent = message; $('toast').hidden = false;
    toastTimer = setTimeout(() => { $('toast').hidden = true; }, 4500);
  }
  function price(item) {
    if (item.manualPrice != null && item.manualPrice !== '') return Number(item.manualPrice);
    const quote = quotes[item.symbol];
    return quote && Date.now() - quote.at < 5 * 60000 ? quote.price : null;
  }
  function portfolio() {
    let value = 0, cost = 0, valuedCost = 0, missing = 0;
    const allocation = {};
    for (const holding of data.holdings ?? []) {
      const basis = Number(holding.quantity) * Number(holding.averageCost);
      cost += basis;
      const quote = price(holding);
      if (quote == null) { ++missing; continue; }
      const worth = Number(holding.quantity) * quote;
      value += worth; valuedCost += basis;
      allocation[holding.symbol] = (allocation[holding.symbol] ?? 0) + worth;
    }
    return { value, cost, valuedCost, missing, allocation, gain: value - valuedCost };
  }
  function explorer(type, item) {
    if (item.network === 'BTC') return `https://mempool.space/${type === 'addresses' ? 'address' : 'tx'}/${encodeURIComponent(item.value)}`;
    if (item.network === 'ETH') return `https://etherscan.io/${type === 'addresses' ? 'address' : 'tx'}/${encodeURIComponent(item.value)}`;
    return null;
  }
  function actions(type, item, extra = '') {
    return `<div class="row-actions">${extra}<button data-edit="${escape(item.id)}" data-type="${type}">Edit</button><button data-delete="${escape(item.id)}" data-type="${type}" aria-label="Delete ${escape(item.label ?? item.symbol)}">Delete</button></div>`;
  }
  function empty(title, description, icon = '◈') {
    return `<div class="empty"><div class="empty-icon">${icon}</div><strong>${escape(title)}</strong><span>${escape(description)}</span></div>`;
  }
  function holdingsTable(items) {
    if (!items.length) return empty('Your portfolio starts here.', 'Add your first holding to build a clearer picture of your assets.');
    return `<div class="table-wrap"><table><thead><tr><th>ASSET</th><th>HOLDINGS</th><th>PRICE / UNIT</th><th>VALUE</th><th>UNREALIZED P/L</th><th></th></tr></thead><tbody>${items.map(item => {
      const p = price(item), worth = p == null ? null : Number(item.quantity) * p;
      const gain = worth == null ? null : worth - Number(item.quantity) * Number(item.averageCost);
      return `<tr><td>${coin(item.symbol)}<span class="asset-name">${names[item.symbol]}<small>${item.symbol}</small></span></td><td>${sensitive(quantity(item.quantity))}</td><td>${p == null ? '—' : usd(p)}<small>${item.manualPrice != null ? 'Your manual estimate' : p == null ? 'Price unavailable' : 'Coinbase spot'}</small></td><td>${worth == null ? '—' : sensitive(usd(worth))}</td><td class="${gain == null ? '' : gain >= 0 ? 'positive' : 'negative'}">${gain == null ? '—' : sensitive(`${gain >= 0 ? '+' : ''}${usd(gain)}`)}</td><td>${actions('holdings', item)}</td></tr>`;
    }).join('')}</tbody></table></div>`;
  }
  function overview() {
    const p = portfolio();
    const allocation = Object.entries(p.allocation).filter(([, value]) => value > 0);
    return `<div class="grid-overview"><section class="card balance-card"><div class="card-label">${p.missing ? 'Priced portfolio subtotal' : 'Estimated portfolio value'}</div><div class="big-value">${sensitive(usd(p.value))}</div><div class="value-foot">${p.missing ? `${p.missing} holding(s) have no current price and are excluded.` : 'Based on your saved holdings and available prices.'}</div><div class="balance-metrics"><div><small>SAVED COST BASIS</small><b>${sensitive(usd(p.cost))}</b></div><div><small>P/L · PRICED HOLDINGS</small><b class="${p.gain >= 0 ? 'positive' : 'negative'}">${sensitive(`${p.gain >= 0 ? '+' : ''}${usd(p.gain)}`)}</b></div></div></section><section class="card"><div class="card-heading"><h2>Asset allocation</h2><span class="badge">BY VALUE</span></div><div class="allocation-wrap"><div id="allocationRing" class="allocation-ring" role="img" aria-label="Allocation by estimated holding value"><span><strong>${allocation.length}</strong>assets priced</span></div><div class="allocation-legend">${allocation.length ? allocation.map(([symbol, value]) => `<div class="legend-line"><span><i class="dot" data-color="${symbol}"></i>${symbol}</span><b>${sensitive(`${(value / p.value * 100).toFixed(1)}%`)}</b></div>`).join('') : '<div class="legend-line">Add holdings to see your allocation.</div>'}</div></div><p class="form-help">Manual estimates are included. Unpriced assets are excluded.</p></section></div><div class="stats-row"><div class="stat-card"><span class="stat-icon">▤</span><div><b>${data.addresses.length}</b><small>SAVED ADDRESSES</small></div></div><div class="stat-card"><span class="stat-icon">↗</span><div><b>${data.transactions.length}</b><small>BOOKMARKED TRANSACTIONS</small></div></div><div class="stat-card"><span class="stat-icon">☆</span><div><b>${data.watchlist.length}</b><small>ASSETS ON YOUR RADAR</small></div></div></div><section class="card"><div class="card-heading"><h2>Your portfolio</h2><button data-go="holdings">View all ↗</button></div>${holdingsTable(data.holdings.slice(0, 5))}</section><section class="card community-banner"><div><span class="eyebrow">BETTER TOGETHER</span><h2>Your next idea could start with a conversation.</h2><p>Meet the community in the lounge, Bitcoin, Ethereum, or builders room.</p></div><button data-go="chat" class="button secondary">Join the conversation ↗</button></section>`;
  }
  function itemGrid(type, items) {
    if (!items.length) return empty(`No ${type === 'watchlist' ? 'watched assets' : type} yet.`, 'Use the add button to make this space yours.');
    const p = portfolio();
    return `<div class="item-grid">${items.map(item => {
      let content;
      if (type === 'addresses' || type === 'transactions') {
        const link = explorer(type, item);
        content = `<div class="item-title"><h3>${escape(item.label)}</h3><span class="badge">${item.network}</span></div><div class="mono">${escape(item.value)}</div><p>${escape(item.note)}</p>${actions(type, item, `<button data-copy="${escape(item.value)}">Copy</button>${link ? `<a class="small-action" href="${escape(link)}" target="_blank" rel="noopener noreferrer">Explorer ↗</a>` : '<span class="small-action">Explorer not available</span>'}`)}`;
      } else if (type === 'watchlist') {
        const quote = quotes[item.symbol], available = quote && Date.now() - quote.at < 5 * 60000;
        const met = available && item.target != null && (item.direction === 'above' ? quote.price >= Number(item.target) : quote.price <= Number(item.target));
        content = `<div class="item-title"><h3>${coin(item.symbol)}${names[item.symbol]}</h3><span class="badge">${met ? 'TARGET REACHED' : 'ON YOUR RADAR'}</span></div><div class="big-value">${available ? usd(quote.price) : '—'}</div><p>${item.target == null ? 'No price target set.' : `Target ${item.direction} ${usd(Number(item.target))}.`}</p><p>${available ? 'Coinbase spot · checked on this page' : 'Market price unavailable.'}</p>${actions(type, item)}`;
      } else if (type === 'goals') {
        const ratio = Number(item.target) > 0 ? Math.min(p.value / Number(item.target), 1) : 0;
        content = `<h3>${escape(item.label)}</h3><div class="big-value">${sensitive(usd(Number(item.target)))}</div><div class="progress-track"><div class="progress-fill" data-progress="${ratio * 100}"></div></div><p>${sensitive(`${(ratio * 100).toFixed(1)}%`)} of goal · ${sensitive(usd(p.value))} ${p.missing ? 'priced subtotal' : 'estimated portfolio value'}</p>${actions(type, item)}`;
      } else content = `<h3>${escape(item.label)}</h3><p>${escape(item.body)}</p>${actions(type, item)}`;
      return `<article class="card item-card">${content}</article>`;
    }).join('')}</div>`;
  }
  function chatView() {
    return `<div class="chat-shell"><aside class="rooms"><div class="sidebar-label">COMMUNITY ROOMS</div>${['lounge', 'bitcoin', 'ethereum', 'builders'].map(name => `<button class="room-button ${name === room ? 'active' : ''}" data-room="${name}"># ${name}</button>`).join('')}<p class="room-rules">Be kind. Share ideas.<br>Never post private keys, seed phrases or personal financial information.<br><br>Members can report messages.</p></aside><section class="chat-main"><header class="chat-header"><h2># ${room}</h2><span id="chatStatus"><span class="pulse"></span>Updates every 4 seconds</span></header><div id="messages" class="messages" role="log" aria-label="Community messages"></div><form id="chatForm" class="composer"><div class="composer-box"><textarea id="messageInput" aria-label="Message" placeholder="Bring a thought. Start a conversation…" maxlength="1000" required></textarea><button class="button primary" type="submit">Send ↗</button></div><small>Messages are shared with signed-in members. Keep the conversation respectful.</small></form></section></div>`;
  }
  function settingsView() {
    return `<div class="settings-grid"><section class="card"><span class="eyebrow">YOUR ACCOUNT</span><h2>Good to have you here.</h2><div class="settings-profile">${escape(user.name)}<span>${escape(user.email)}</span><span class="mono">Account ID: ${escape(user.id)}</span></div><p>Your email identifies this account. Email verification and password recovery are not configured yet.</p><h2>Take your workspace with you.</h2><p>Download a JSON backup of your holdings, saved addresses, transaction bookmarks, goals and notes.</p><button id="exportData" class="button secondary">↓ Export my workspace</button></section><section class="card"><span class="eyebrow">ACCOUNT SECURITY</span><h2>Update your password.</h2><form id="passwordForm"><label>Current password<input name="currentPassword" type="password" autocomplete="current-password" minlength="12" maxlength="128" required></label><label>New password<input name="newPassword" type="password" autocomplete="new-password" minlength="12" maxlength="128" required></label><button type="submit" class="button primary">Update password ↗</button></form><p>Changing your password signs out your other sessions.</p></section>${user.moderator ? '<section class="card"><h2>Community reports</h2><div id="moderationReports"></div></section>' : ''}</div>`;
  }
  function render() {
    if (!user) return;
    const t = titles[view];
    $('pageEyebrow').textContent = t[0]; $('pageTitle').textContent = t[1]; $('pageSubtitle').textContent = t[2];
    $('breadcrumb').textContent = view === 'holdings' ? 'Portfolio' : view[0].toUpperCase() + view.slice(1);
    $('addButton').hidden = view === 'chat' || view === 'settings';
    $('addButton').textContent = `+ Add ${singular[view === 'overview' ? 'holdings' : view]}`;
    document.querySelectorAll('[data-view]').forEach(button => button.classList.toggle('active', button.dataset.view === view));
    if (view === 'overview') $('viewContent').innerHTML = overview();
    else if (view === 'chat') $('viewContent').innerHTML = chatView();
    else if (view === 'settings') $('viewContent').innerHTML = settingsView();
    else {
      const items = (data[view] ?? []).filter(item => JSON.stringify(item).toLowerCase().includes(search.toLowerCase()));
      $('viewContent').innerHTML = `<div class="toolbar"><input id="itemSearch" type="search" aria-label="Search saved items" placeholder="Search your ${view}…" value="${escape(search)}"><small>${items.length} saved item(s)</small></div>${view === 'holdings' ? `<section class="card">${holdingsTable(items)}</section>` : itemGrid(view, items)}`;
      $('itemSearch').addEventListener('input', event => {
        const start = event.target.selectionStart; search = event.target.value; render();
        $('itemSearch').focus(); $('itemSearch').setSelectionRange(start, start);
      });
    }
    document.querySelectorAll('[data-color]').forEach(node => { node.style.backgroundColor = colors[node.dataset.color]; });
    document.querySelectorAll('[data-progress]').forEach(node => { node.style.width = `${node.dataset.progress}%`; });
    const ring = $('allocationRing');
    if (ring) {
      const p = portfolio(); let position = 0;
      const stops = Object.entries(p.allocation).filter(([, value]) => value > 0).map(([symbol, value]) => {
        const start = position; position += value / p.value * 100; return `${colors[symbol]} ${start}% ${position}%`;
      });
      if (stops.length) ring.style.background = `conic-gradient(${stops.join(',')})`;
    }
    if (view === 'chat') { chatSignature = ''; loadChat(); $('chatForm').addEventListener('submit', sendMessage); }
    if (view === 'settings') {
      $('exportData').addEventListener('click', exportData);
      $('passwordForm').addEventListener('submit', changePassword);
      if (user.moderator) loadReports();
    }
  }
  function setView(next) {
    if (!titles[next]) return;
    view = next; search = ''; clearInterval(chatTimer); chatTimer = null;
    $('sidebar').classList.remove('open'); $('mobileNav').setAttribute('aria-expanded', 'false');
    render();
    if (view === 'chat') chatTimer = setInterval(loadChat, 4000);
  }
  async function loadData() { data = await api('items'); render(); }
  async function loadMarket() {
    try {
      const result = await api('market'); quotes = result.quotes;
      $('marketStrip').innerHTML = `${['BTC', 'ETH'].map(symbol => `<span class="quote"><b>${symbol}</b>${quotes[symbol] ? usd(quotes[symbol].price) : 'Unavailable'}</span>`).join('')}<span class="quote"><b>B2MX</b>No public market quote</span><span class="market-note">Coinbase spot · ${Object.keys(quotes).length ? `checked ${new Date(result.checked).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' })}` : 'price service unavailable'}</span>`;
      if (view !== 'chat' && view !== 'settings' && !document.activeElement?.matches('#itemSearch')) render();
    } catch (error) { if (user) toast(error.message); }
  }
  async function enter(account) {
    user = account; $('authScreen').hidden = true; $('app').hidden = false;
    $('userInitial').textContent = user.name.slice(0, 1).toUpperCase();
    $('authForm').reset(); $('authError').textContent = '';
    try { await loadData(); await loadMarket(); } catch (error) { toast(error.message); }
    clearInterval(marketTimer); marketTimer = setInterval(() => { if (!document.hidden && user) loadMarket(); }, 60000);
  }
  function showAuth() {
    user = null; data = {}; quotes = {}; clearInterval(chatTimer); clearInterval(marketTimer);
    view = 'overview'; room = 'lounge'; search = ''; chatSignature = '';
    $('itemDialog').close(); $('app').hidden = true; $('authScreen').hidden = false;
  }
  function setAuthMode(mode) {
    authMode = mode; const register = mode === 'register';
    $('nameField').hidden = !register; $('authForm').elements.name.required = register;
    $('registerNotice').hidden = !register;
    $('authForm').elements.password.autocomplete = register ? 'new-password' : 'current-password';
    $('authTitle').textContent = register ? 'Make room for your future.' : 'Good to see you.';
    $('authSubtitle').textContent = register ? 'Your personal workspace starts here.' : 'Sign in and pick up where you left off.';
    $('authSubmit').textContent = register ? 'Create my command center ↗' : 'Enter command center ↗';
    $('loginTab').classList.toggle('active', !register); $('registerTab').classList.toggle('active', register);
    $('authError').textContent = '';
  }
  const schemas = {
    addresses: [['label', 'Name', 'text'], ['network', 'Network', 'symbol'], ['value', 'Public address', 'text'], ['note', 'Note (optional)', 'textarea', false]],
    transactions: [['label', 'Name', 'text'], ['network', 'Network', 'symbol'], ['value', 'Transaction hash', 'text'], ['note', 'Note (optional)', 'textarea', false]],
    holdings: [['symbol', 'Asset', 'symbol'], ['quantity', 'Quantity held', 'decimal'], ['averageCost', 'Average purchase cost per coin (USD)', 'decimal'], ['manualPrice', 'Manual price per coin (USD, optional)', 'decimal', false]],
    watchlist: [['symbol', 'Asset', 'symbol'], ['target', 'Target price (USD, optional)', 'decimal', false], ['direction', 'Target direction', 'direction']],
    goals: [['label', 'Goal name', 'text'], ['target', 'Target portfolio value (USD)', 'decimal']],
    notes: [['label', 'Title', 'text'], ['body', 'Your research or reminder', 'textarea']]
  };
  function openItem(type, item = null) {
    editing = { type, id: item?.id };
    $('dialogTitle').textContent = `${item ? 'Edit' : 'Add'} ${singular[type]}`;
    $('itemError').textContent = '';
    $('itemFields').innerHTML = schemas[type].map(([key, label, kind, required = true]) => {
      const value = item?.[key] ?? '';
      const flags = `${required ? 'required' : ''} name="${key}"`;
      const field = kind === 'symbol' ? `<select ${flags}>${symbolOptions}</select>` :
        kind === 'direction' ? `<select ${flags}><option value="above">At or above target</option><option value="below">At or below target</option></select>` :
        kind === 'textarea' ? `<textarea ${flags} maxlength="${type === 'notes' ? 2000 : 500}">${escape(value)}</textarea>` :
        `<input ${flags} type="text" ${kind === 'decimal' ? 'inputmode="decimal"' : ''} maxlength="${key === 'label' ? 80 : 128}" value="${escape(value)}">`;
      return `<label>${label}${field}</label>`;
    }).join('') + (type === 'holdings' ? '<p class="form-help">Leave manual price blank to use available BTC or ETH spot prices. B2MX has no public market quote; a manual price is your estimate.</p>' : type === 'addresses' ? '<p class="form-help">Saved addresses are bookmarks. Address format is checked, but ownership and checksum are not verified.</p>' : '');
    if (item) for (const [key] of schemas[type]) $('itemForm').elements[key].value = item[key] ?? '';
    $('itemDialog').showModal();
  }
  async function saveItem(event) {
    event.preventDefault(); $('saveItem').disabled = true; $('itemError').textContent = '';
    try {
      const item = Object.fromEntries(new FormData($('itemForm')));
      await api(editing.id ? `items/${editing.id}` : 'items', { method: editing.id ? 'PUT' : 'POST', value: { type: editing.type, item } });
      $('itemDialog').close(); await loadData(); toast('Saved to your workspace.');
    } catch (error) { $('itemError').textContent = error.message; }
    finally { $('saveItem').disabled = false; }
  }
  async function loadChat() {
    if (!user || view !== 'chat' || chatBusy || document.hidden) return;
    chatBusy = true; const requestedRoom = room;
    try {
      const result = await api(`chat?room=${room}`);
      if (view !== 'chat' || room !== requestedRoom || !user) return;
      const signature = JSON.stringify(result.messages);
      if (signature === chatSignature) return;
      const box = $('messages'), nearBottom = box.scrollHeight - box.scrollTop - box.clientHeight < 100;
      const initial = !chatSignature; chatSignature = signature;
      box.innerHTML = result.messages.length ? result.messages.map(message => `<article class="message"><span class="avatar">${escape(message.author[0].toUpperCase())}</span><div class="message-content"><div class="message-meta"><b>${escape(message.author)}</b><time datetime="${new Date(message.created).toISOString()}">${new Date(message.created).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' })}</time><button class="message-action" ${message.authorId === user.id || user.moderator ? `data-message-delete="${message.id}"` : `data-report="${message.id}"`}>${message.authorId === user.id || user.moderator ? 'Delete' : 'Report'}</button></div><p>${escape(message.body)}</p></div></article>`).join('') : empty('A fresh conversation starts here.', 'Say hello. Share an idea. Make a connection.', '◎');
      if (initial || nearBottom) box.scrollTop = box.scrollHeight;
      $('chatStatus').textContent = 'Connected · updates every 4 seconds';
    } catch (error) { if ($('chatStatus')) $('chatStatus').textContent = error.message; }
    finally { chatBusy = false; }
  }
  async function sendMessage(event) {
    event.preventDefault(); const form = event.currentTarget, button = form.querySelector('button');
    const input = $('messageInput'), message = input.value.trim(); if (!message) return;
    button.disabled = true;
    try { await api('chat', { method: 'POST', value: { room, body: message } }); input.value = ''; chatSignature = ''; await loadChat(); input.focus(); }
    catch (error) { toast(error.message); }
    finally { button.disabled = false; }
  }
  async function loadReports() {
    try {
      const result = await api('reports');
      if (!$('moderationReports')) return;
      $('moderationReports').innerHTML = result.reports.length ? result.reports.map(report => `<article class="item-card"><h3># ${escape(report.room)} · ${escape(report.author)}</h3><p>${escape(report.body)}</p><small>${report.count} report(s)</small><button class="small-action" data-message-delete="${report.id}">Remove message</button></article>`).join('') : '<p>No reported messages.</p>';
    } catch (error) { toast(error.message); }
  }
  async function exportData() {
    try {
      const result = await api('export'); const blob = new Blob([JSON.stringify(result, null, 2)], { type: 'application/json' });
      const url = URL.createObjectURL(blob), anchor = document.createElement('a');
      anchor.href = url; anchor.download = 'bitcoin2max-my-workspace.json'; anchor.click(); setTimeout(() => URL.revokeObjectURL(url), 1000);
      toast('Workspace backup downloaded. Keep it private.');
    } catch (error) { toast(error.message); }
  }
  async function changePassword(event) {
    event.preventDefault(); const form = event.currentTarget, button = form.querySelector('button'); button.disabled = true;
    try { await api('password', { method: 'POST', value: Object.fromEntries(new FormData(form)) }); form.reset(); toast('Password changed. Your other sessions have been signed out.'); }
    catch (error) { toast(error.message); }
    finally { button.disabled = false; }
  }
  document.addEventListener('click', async event => {
    const button = event.target.closest('button'); if (!button) return;
    try {
      if (button.dataset.view || button.dataset.go) return setView(button.dataset.view ?? button.dataset.go);
      if (button.dataset.edit) return openItem(button.dataset.type, data[button.dataset.type].find(item => item.id === button.dataset.edit));
      if (button.dataset.delete) { await api(`items/${button.dataset.delete}`, { method: 'DELETE' }); await loadData(); return toast('Item removed.'); }
      if (button.dataset.copy) { await navigator.clipboard.writeText(button.dataset.copy); return toast('Copied.'); }
      if (button.dataset.room) { room = button.dataset.room; return setView('chat'); }
      if (button.dataset.messageDelete) { await api(`chat/${button.dataset.messageDelete}`, { method: 'DELETE' }); chatSignature = ''; if (view === 'chat') await loadChat(); else await loadReports(); return toast('Message removed.'); }
      if (button.dataset.report) { await api(`chat/${button.dataset.report}/report`, { method: 'POST', value: {} }); return toast('Report saved for moderator review.'); }
    } catch (error) { toast(error.message); }
  });
  $('loginTab').addEventListener('click', () => setAuthMode('login'));
  $('registerTab').addEventListener('click', () => setAuthMode('register'));
  $('authForm').addEventListener('submit', async event => {
    event.preventDefault(); $('authSubmit').disabled = true; $('authError').textContent = '';
    try { const result = await api(authMode, { method: 'POST', value: Object.fromEntries(new FormData($('authForm'))) }); await enter(result.user); }
    catch (error) { $('authError').textContent = error.message; }
    finally { $('authSubmit').disabled = false; }
  });
  $('addButton').addEventListener('click', () => openItem(view === 'overview' ? 'holdings' : view));
  $('itemForm').addEventListener('submit', saveItem);
  $('closeDialog').addEventListener('click', () => $('itemDialog').close());
  $('cancelDialog').addEventListener('click', () => $('itemDialog').close());
  $('logout').addEventListener('click', async () => { try { await api('logout', { method: 'POST', value: {} }); showAuth(); } catch (error) { toast(error.message); } });
  $('themeToggle').addEventListener('click', () => { document.body.classList.toggle('light'); try { localStorage.setItem('b2mx-theme', document.body.classList.contains('light') ? 'light' : 'dark'); } catch {} });
  $('privacyToggle').addEventListener('click', () => { document.body.classList.toggle('privacy'); const hidden = document.body.classList.contains('privacy'); $('privacyToggle').setAttribute('aria-pressed', String(hidden)); $('privacyToggle').setAttribute('aria-label', hidden ? 'Show portfolio values' : 'Hide portfolio values'); });
  $('mobileNav').addEventListener('click', () => { const open = $('sidebar').classList.toggle('open'); $('mobileNav').setAttribute('aria-expanded', String(open)); });
  try { document.body.classList.toggle('light', localStorage.getItem('b2mx-theme') === 'light'); } catch {}
  document.addEventListener('visibilitychange', () => { if (!document.hidden && user) { if (view === 'chat') loadChat(); loadMarket(); } });
  (async () => {
    try {
      await api('health'); $('serviceStatus').textContent = 'Account service connected';
      try { const result = await api('me'); await enter(result.user); } catch (error) { if (!error.message.includes('sign in')) $('authError').textContent = error.message; }
    } catch (error) { $('serviceStatus').textContent = 'Account service offline. Run the dashboard on its account server to sign in.'; $('authSubmit').disabled = true; }
  })();
})();
