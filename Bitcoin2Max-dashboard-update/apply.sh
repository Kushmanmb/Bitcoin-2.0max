#!/usr/bin/env bash
set -euo pipefail
installer_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(git rev-parse --show-toplevel)"
cd "$repo_root"
if ! node -e 'if (Number(process.versions.node.split(".")[0]) !== 24) process.exit(1)'; then
  echo "Use Node.js 24 before installing. With nvm: nvm install 24 && nvm use 24" >&2
  exit 1
fi
# README is optional so this update can coexist with the native-coin draft.
git apply --check --exclude=README.md "$installer_dir/dashboard.patch"
git apply --exclude=README.md "$installer_dir/dashboard.patch"
if git apply --check --include=README.md "$installer_dir/dashboard.patch" 2>/dev/null; then
  git apply --include=README.md "$installer_dir/dashboard.patch"
else
  echo "README left as-is because it has other edits. See DASHBOARD-SETUP.md."
fi
printf '
Dashboard installed. Run:
  npm run test:dashboard
  npm start

Open http://localhost:3000/dashboard.html
'
