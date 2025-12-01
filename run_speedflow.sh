#!/bin/bash
# Run SpeedFlow with proper GST_PLUGIN_PATH

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Set plugin path so GStreamer can find our custom plugin
export GST_PLUGIN_PATH="$SCRIPT_DIR/build/plugins"

# Run speedflow with all arguments passed through
exec "$SCRIPT_DIR/build/speedflow" "$@"
