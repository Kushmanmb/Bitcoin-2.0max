# Install the dashboard and community chat

Upload this ZIP into your existing Bitcoin-2.0max repository, then run:

```bash
git switch -c feature/user-dashboard
unzip Bitcoin2Max-dashboard.zip
bash Bitcoin2Max-dashboard-update/apply.sh
npm run test:dashboard
npm start
```

Requires Node.js 24. No npm dependencies are needed.
Open http://localhost:3000/dashboard.html and create an account.
For Codespaces and live HTTPS hosting, read DASHBOARD-SETUP.md.
GitHub Pages alone cannot run the account and chat server.

The installer checks the patch before applying it and leaves existing conflicting
files alone. If it reports a patch conflict, resolve that conflict against your
current checkout before continuing; it does not overwrite your changes.
The source/ folder includes all changed files for inspection or manual integration.

The server's 9 automated tests and frontend DOM interaction checks passed.
Browser-rendered visual QA was unavailable because browser downloads were blocked.
The code is not pushed, deployed, or a custody/trading service. Password recovery,
email verification, MFA, direct messages and automatic wallet synchronization
are not implemented in this version.
