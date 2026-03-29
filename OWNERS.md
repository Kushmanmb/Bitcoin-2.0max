# Owners

This file documents the humans responsible for the Bitcoin 2.0max project.

## Project Lead

| GitHub Handle | Role | Responsibilities |
|---|---|---|
| [@Kushmanmb](https://github.com/Kushmanmb) | Project Lead & Maintainer | Overall architecture, release management, security review, community |

## Area Owners

The table below maps repository areas to their primary reviewers.
All code review requests are automatically assigned via [`.github/CODEOWNERS`](.github/CODEOWNERS).

| Area | Path(s) | Owner(s) |
|---|---|---|
| Core node logic | `src/node/`, `include/bitcoin2max/` | @Kushmanmb |
| Cryptography | `src/crypto/` | @Kushmanmb |
| Electrum client | `src/electrum/` | @Kushmanmb |
| Configuration | `src/config/`, `conf/` | @Kushmanmb |
| Build system | `CMakeLists.txt`, `cmake/`, `Dockerfile` | @Kushmanmb |
| CI / GitHub Actions | `.github/workflows/` | @Kushmanmb |
| Tests | `tests/` | @Kushmanmb |
| Documentation | `*.md`, `docs/` | @Kushmanmb |

## Becoming an Owner

If you have made significant contributions to an area and would like to
be listed as a co-owner:

1. Open a pull request updating this file and `.github/CODEOWNERS`.
2. The existing owner(s) for that area must approve.
3. The project lead will merge once approved.

## Governance

Bitcoin 2.0max follows a **BDFL** (Benevolent Dictator For Life) model.
The project lead has final say on all technical and organisational
decisions.  Major changes (consensus rule changes, breaking API changes,
release scheduling) require explicit sign-off from the project lead.

## Contact

For security vulnerabilities, please use GitHub's private vulnerability
reporting feature or open an issue with the **security** label.

For general questions, open a GitHub Discussion or issue.
