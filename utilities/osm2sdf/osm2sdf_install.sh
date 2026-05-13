#!/usr/bin/env bash

set -euo pipefail

###############################################################################
# Usage
###############################################################################
usage() {
    cat <<EOF
Usage: $0 [-g | -v]

Options:
  -g    Install Python dependencies globally using apt
  -v    Create a Python virtual environment and install dependencies there

If no or an invalid option is provided, this help is shown.
EOF
    exit 1
}

if [ $# -ne 1 ]; then
    usage
fi

MODE="$1"

###############################################################################
# Download and unpack OSM2World
###############################################################################
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [ ! -d "${SCRIPT_DIR}/osm2world" ]; then
    mkdir -p "${SCRIPT_DIR}/osm2world"
    cd "${SCRIPT_DIR}/osm2world"

    wget https://osm2world.org/download/files/latest/OSM2World-latest-bin.zip
    unzip OSM2World-latest-bin.zip
    rm OSM2World-latest-bin.zip

    cd - >/dev/null 2>&1
fi

###############################################################################
# Base system dependencies
###############################################################################
# Install unzip and default-jre (needed for OSM2World)
apt-get update && apt-get install -y --no-install-recommends unzip default-jre

###############################################################################
# Python dependencies
###############################################################################
case "$MODE" in
    -g)
        # Global installation of Python packages
        apt-get install -y --no-install-recommends \
            python3-xmltodict \
            python3-pyproj \
            python3-termcolor
        ;;
    -v)
        # Virtual environment installation of Python packages
        if ! command -v python3 >/dev/null 2>&1; then
            echo "python3 not found. Please install python3 first."
            exit 1
        fi

        python3 -m venv "$SCRIPT_DIR/.venv"
        source "$SCRIPT_DIR/.venv/bin/activate"

        pip install --upgrade pip
        pip install xmltodict pyproj termcolor

        echo "Virtual environment created at: $SCRIPT_DIR/.venv"
        echo "Activate it with: source $SCRIPT_DIR/.venv/bin/activate"
        ;;
    *)
        usage
        ;;
esac

###############################################################################
# Environment variables and directories
###############################################################################
# Set OSM2SDF_PATH and GZ_SIM_RESOURCE_PATH in .bashrc
cat >> ~/.bashrc <<EOF

# Gazebo resource path extension (auto-added)
export OSM2SDF_PATH="${SCRIPT_DIR}"
if [ -n "\${GZ_SIM_RESOURCE_PATH:-}" ]; then
    export GZ_SIM_RESOURCE_PATH="\${GZ_SIM_RESOURCE_PATH}:${SCRIPT_DIR}/tmp"
else
    export GZ_SIM_RESOURCE_PATH="${SCRIPT_DIR}/tmp"
fi

EOF
mkdir -p "${SCRIPT_DIR}/tmp"

echo "Setup completed successfully to use with env variables resource with$ source ~/.bashrc"