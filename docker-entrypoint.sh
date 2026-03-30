#!/bin/sh
# docker-entrypoint.sh - Bitcoin 2.0max container entry point
#
# Priority for selecting the configuration file:
#   1. Explicit --conf / -conf argument passed by the user at runtime.
#   2. A bitcoin2max.conf file found inside the mounted data directory
#      (/var/lib/bitcoin2max/bitcoin2max.conf).
#   3. The built-in default at /etc/bitcoin2max/bitcoin2max.conf.
#
# Any extra arguments (e.g. --help, --datadir) are forwarded to the daemon.

set -e

DAEMON=/usr/local/bin/bitcoin2maxd
USER_CONF=/var/lib/bitcoin2max/bitcoin2max.conf
DEFAULT_CONF=/etc/bitcoin2max/bitcoin2max.conf

# Check whether the caller already supplied an explicit --conf / -conf flag
# in any of its forms: --conf <path>, -conf <path>, --conf=<path>, -conf=<path>.
has_conf_flag() {
    for arg in "$@"; do
        case "$arg" in
            --conf|-conf|--conf=*|-conf=*) return 0 ;;
        esac
    done
    return 1
}

if has_conf_flag "$@"; then
    exec "$DAEMON" "$@"
elif [ -f "$USER_CONF" ]; then
    exec "$DAEMON" --conf "$USER_CONF" "$@"
else
    exec "$DAEMON" --conf "$DEFAULT_CONF" "$@"
fi
