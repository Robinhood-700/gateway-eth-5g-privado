#!/usr/bin/env bash
set -euo pipefail

PORT="${1:-/dev/ttyUSB0}"
BAUD="${2:-460800}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FW_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
IDF_EXPORT="${IDF_EXPORT:-/Users/antonio/esp/esp-idf/export.sh}"

if [[ ! -f "${IDF_EXPORT}" ]]; then
  echo "IDF export script not found: ${IDF_EXPORT}" >&2
  exit 1
fi

# shellcheck source=/dev/null
source "${IDF_EXPORT}"

cd "${FW_DIR}"
echo "Flashing from ${FW_DIR} to ${PORT} @ ${BAUD}"
idf.py -p "${PORT}" -b "${BAUD}" flash monitor
