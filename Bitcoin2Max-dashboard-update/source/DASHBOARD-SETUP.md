# Personal dashboard and community chat

The new `docs/dashboard.html` is linked from the website navigation. It includes
real account registration/login, server-saved workspace data, a portfolio,
address and transaction bookmarks, watchlists, goals, research notes, account
export, password changes, color themes, balance hiding, and four community rooms.

## Run locally

Use Node.js 24 LTS. There are no npm dependencies to install.

```bash
node --version
npm run test:dashboard
npm start
```

Open `http://localhost:3000/dashboard.html` and create an account. Saved data
persists in `data/dashboard/accounts.sqlite`, which is ignored by Git.
Registration never creates a blockchain wallet or requests seed phrases.

For a Codespace, open port 3000 and set the exact forwarded URL as APP_ORIGIN:

```bash
HOST=0.0.0.0 APP_ORIGIN="https://YOUR-CODESPACE-3000.app.github.dev" npm start
```

The URL must match the browser's origin, with no trailing slash or path. Do not
use the example placeholder unchanged. Keep development ports private until
you have reviewed the deployment and moderation configuration.

## Hosting

GitHub Pages serves static files; it cannot run this account server or SQLite.
The dashboard shows an offline account-service message on a static-only host
and does not simulate successful logins or browser-only account persistence.
For live accounts, run the Node service on a server with a persistent data disk
and serve the website and account API together through an HTTPS reverse proxy.

```bash
NODE_ENV=production HOST=127.0.0.1 PORT=3000 \
APP_ORIGIN="https://your-domain.example" \
DASHBOARD_DATABASE="/persistent/bitcoin2max/accounts.sqlite" \
DASHBOARD_MODERATOR_IDS="YOUR-EXISTING-ACCOUNT-ID" npm start
```

Set the real domain. First create the intended moderator account, copy its
account ID from Account settings, and then restart with that exact ID in
DASHBOARD_MODERATOR_IDS. Only those existing account IDs can see reports and
remove other members' messages. Roles are not assigned by unverified email or
display name. Omit this variable until a moderator account exists.

The backend accepts same-origin mutations only. It stores scrypt password
hashes, hashed session tokens, and account-owned records in SQLite. Sessions
use HttpOnly, SameSite=Strict cookies; HTTPS deployments also use Secure and
the __Host- cookie prefix. Password changes revoke other sessions. Authentication
and chat are rate limited; each account can save up to 1,000 workspace records.
The database is not encrypted at rest: protect the host and backups.

This is an initial runnable account service. Before a public production launch,
add verified email registration and password recovery, stronger operational
abuse controls, monitoring, database backups, retention policies and security
review. There is no MFA or passkey login in this implementation. Rate limits
are process-local and reset on restart; they are not a distributed abuse system.

## Portfolio behavior

- Holdings, quantities and average purchase costs are entered by the user; no
  wallet balance or on-chain ownership is inferred from saved addresses.
- BTC/ETH spot quotes are fetched from Coinbase server-side and refreshed on
  the page every minute. Missing quotes stay missing, and quotes older than
  five minutes are excluded from valuation.
- B2MX has no public price feed here. An optional manual price is clearly
  labeled as the user's estimate. It does not establish a market price.
- Unpriced assets are excluded; the total is labeled a priced subtotal when
  any holding has no quote. P/L includes only valued positions.
- Allocation is calculated from priced holdings. There is no fabricated
  performance history, guaranteed return, trading or custody functionality.
- Watchlist targets are checked on the open page; no background notifications
  are sent. Goals compare against the current estimated portfolio value.
- Address checks are structural format checks, not checksum or ownership
  verification. B2MX addresses are placeholders until the native format exists.
- Export produces a private JSON backup; importing backups is not implemented.

## Community rooms

Members can use lounge, bitcoin, ethereum, and builders rooms. The browser
polls every four seconds and displays the latest 100 messages per room. Messages
persist in the database. Members can delete their own messages and report posts;
configured moderators can review reports and remove any message. Chat text is
escaped, and private account emails are not exposed in chat responses.

Reporting saves a report for moderator review; it does not automatically remove
a message. There are no direct messages, file uploads, member bans or automated
content moderation in this version. Existing avatars and network features on
the original website remain separate from the personal dashboard.

## Tests

`npm run test:dashboard` covers account isolation, session handling, password
changes, persisted data, mutation origin checks, validation, chat rooms, spam
limits, reporting, moderator authorization, export, market quotes and private
file access. Use a separate development database for UI testing.
