#!/bin/bash
#
# Bootstraps a fresh Ubuntu machine to build this repo. Run from within an
# existing checkout of WROXIM (this script does not clone the repo itself).
#
#   bash hybrid/other/setup/ubuntu.sh

set -e

sudo apt-get update
sudo apt-get -y install build-essential cmake git wget

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
HYBRID_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"

"${HYBRID_DIR}/setup_dependencies.sh"

echo "Now build the simulator with:"
echo "  cd ${HYBRID_DIR}/bin && make"
