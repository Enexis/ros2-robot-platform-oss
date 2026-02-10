#!/bin/bash
set -e

# Source the ROS setup script if it exists
if [ -f "/opt/ros/${ROS_DISTRO}/setup.bash" ]; then
    source "/opt/ros/${ROS_DISTRO}/setup.bash"
fi

# Source the workspace setup if it exists
if [ -f "/workspace/install/setup.bash" ]; then
    source "/workspace/install/setup.bash"
fi

# Execute the command passed to the container
exec "$@"
