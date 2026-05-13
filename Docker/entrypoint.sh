#!/bin/bash -i

# Source the workspace setup if it exists
if [ -f "/$WORKDIR/install/setup.bash" ]; then 
    source "/$WORKDIR/install/setup.bash"
fi

exec "$@"
