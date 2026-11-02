string SWITCHING="switching"
# Localization: map loaded, waiting for the initial pose before starting Nav2.
string WAITING_FOR_POSE="waiting_for_pose"
string READY="ready"
string FAILED="failed"

# Current or target mode: idle, mapping or localization.
string mode
# Map basename used for localization; empty otherwise.
string map_name
# One of SWITCHING, WAITING_FOR_POSE, READY or FAILED.
string state
string message
