#!/bin/bash

set -euo pipefail

REPO_ROOT="/opt/datacrumbs"
BUILD_DIR="${REPO_ROOT}/datacrumbs-build"
INSTALL_PREFIX="/opt/datacrumbs-install"
CONFIG_YAML="${INSTALL_PREFIX}/etc/datacrumbs/configs/docker.yaml"
PROBE_FILE="/tmp/datacrumbs-docker-probes.json.gz"
OUT_FILE="/tmp/img_temp.bin"

set +u
. /opt/rh/gcc-toolset-11/enable
set -u
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"
cmake -DDATACRUMBS_HOST=docker \
	-DDATACRUMBS_USER=root \
	-DDATACRUMBS_INSTALL_USER=docker \
	-DDATACRUMBS_KERNEL_HEADERS_PATH=/usr/src/kernels/4.18.0-348.7.1.el8_5.x86_64 \
	-DCMAKE_PREFIX_PATH=/usr/lib64/openmpi \
	-DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}" \
	"${REPO_ROOT}"
cmake --build . -j"$(nproc)"
cmake --install .

test -f "${CONFIG_YAML}"
test -x "${INSTALL_PREFIX}/bin/datacrumbs_probe_configurator"
test -x "${INSTALL_PREFIX}/bin/datacrumbs_wrap"

"${INSTALL_PREFIX}/bin/datacrumbs_probe_configurator" "${CONFIG_YAML}" "${PROBE_FILE}"
test -f "${PROBE_FILE}"

rm -f "${OUT_FILE}"
"${INSTALL_PREFIX}/bin/datacrumbs_wrap" \
	dd if=/dev/zero of="${OUT_FILE}" bs=1M count=16 status=none

test -f "${OUT_FILE}"
test "$(wc -c <"${OUT_FILE}")" = "16777216"
