# ROS 2 Robot Platform OSS

ROS 2 Jazzy workspace for ROS 2 robot platforms in a modular way. Below is the folder structure and a short description of each (sub)folder.

## Repository Layout

```text
├─ architecture/
│  ├─ robot_platform/
│  └─ <specific platform architecture>
├─ cyclonedds
├─ Docker/
│  ├─ base_dev/
│  ├─ visualization/
│  └─ <specific platform images>
├─ robotics_modules/
│  ├─ application_control/
│  ├─ mobile_bases/
│  ├─ platform_kits/
│  ├─ robotic_arms/
│  ├─ ros2_control/
│  ├─ sensors/
│  └─ visualization/
└─ utilities/
   └─ osm2sdf/
```

Descriptions

- `architecture/`
  - High-level system architecture and design artifacts (diagrams, specs).
  - `robot_platform/`: Generic robot platform architecture and shared patterns.
  - Should be extended with platform specific architectures

- `cyclonedds`
  - Configuration of Cyclone DDS (ROS 2 middleware) network.

- `Docker/`
  - Docker build contexts and assets for development and runtime images.
  - `base_dev/`: Base and development image
  - `visualization/`: Image for gazebo and visualization tools (e.g., RViz, web dashboards).
  - Shoud be extended with images and context tailored to the specific robot platforms.

- `.env`
  - Environment variable defaults used by Docker Compose.

- `docker-compose.common.yml`
  - Shared Compose settings included by environment-specific Compose files.

- `entrypoint.sh`
  - Container entrypoint script to source ROS 2 in case of running a non-interactive container.

- `robotics_modules/`
  - Source tree for ROS 2 packages and domain modules.
  - `application_control/`: High-level custom behavior, mission logic, and orchestrators.
  - `mobile_bases/`: Drivers, bring-up and adapters for mobile bases.
  - `platform_kits/`: Bundled launch/config sets for specific platform configurations.
  - `robotic_arms/`: Manipulator drivers, kinematics, and controllers.
  - `ros2_control/`: `ros2_control` and friends (`Nav2`/`Moveit2`) hardware interfaces, controller configs, and launch files.
  - `sensors/`: Sensor drivers, calibration, and message adapters.
  - `visualization/`: Gazebo, RViz configs, visualization nodes, and display resources for modules.

- `utilities/`
  - Helper tools and scripts used across modules and CI.
  - `osm2sdf/`: Tools to convert OpenStreetMap data to SDF for simulation environments.

---

## Safety

This project can command a real robot.
- Use an open, safe test area.
- Keep an emergency stop strategy ready.
- Start with low speeds and conservative commands.
- Verify command topics/services before enabling actuation.

---

## Supported Environment

- ROS 2: **Jazzy**
- Ubuntu: **24.04** (native, Docker, or WSL2 Ubuntu)
- DDS middleware: **CycloneDDS** (`rmw_cyclonedds_cpp`)

> The project is Linux-first. On Windows, use WSL2 (see Appendix A).

---

## Quick Start (Docker Compose)

This project is intended to be started with Docker Compose. The `ros2-go2` image build:
- installs ROS package dependencies with `rosdep`
- builds the workspace with `colcon`

Both the robot image (`Docker/enexis_go2/Dockerfile`) and the dev image (`Docker/base_dev/Dockerfile`) use cached layers, so a rebuild only redoes what changed:

| Change | What rebuilds |
|---|---|
| Launch/config files in `enexis_go2`, `nav2`, `localization` (dev image: also `gazebo`) | Only the last `colcon` layer (about 1 min on the Jetson) |
| Source in `unitree_go2` or the sensor packages (dev image: also `geo_path_follower`) | Both `colcon` layers (a few minutes) |
| Any `package.xml` | The `rosdep` dependency install and everything after it (about 30 min on the Jetson) |
| Dev image only: files in `utilities/` | Only the final `utilities` copy and shell setup (seconds) |

The Livox SDK and `livox_ros_driver2` are cloned in a cached layer. To get their latest upstream versions, build with `--no-cache`.
Each Dockerfile has a `Dockerfile.dockerignore` that limits the files sent to the build.

### 1) Clone

```bash
git clone https://github.com/<your-org-or-user>/ros2-robot-platform-oss.git
cd ros2-robot-platform-oss
```

### 2) Configure DDS profile in `Docker/.env`

Docker Compose reads `CYCLONEDDS_URI` from `Docker/.env`.

Edit `Docker/.env` and select the profile for your machine:

```dotenv
# Laptop development machine
CYCLONEDDS_URI=file:///workspace/cyclonedds/cyclonedds_laptop.xml

# Jetson on the robot (alternative)
# CYCLONEDDS_URI=file:///workspace/cyclonedds/cyclonedds_jetson.xml
```

For Jetson, set interface names in `cyclonedds/cyclonedds_jetson.xml` to match the device:
- `en*` = Ethernet interface connected to the Unitree Go2 embedded computer
- `wl*` = wireless interface on the Jetson

`cyclonedds_jetson.xml` requests a 10 MB DDS socket receive buffer, so large
messages such as `/livox/lidar` (about 0.5 MB) are not lost in fragments. Allow
this once on the Jetson host before you start the containers; otherwise every
ROS node fails with `failed to increase socket receive buffer size`:

```bash
sudo tee /etc/sysctl.d/10-cyclone-max.conf <<'EOF'
net.core.rmem_max=2147483647
net.ipv4.ipfrag_time=3
net.ipv4.ipfrag_high_thresh=134217728
EOF
sudo sysctl --system
```

For laptop usage, `cyclonedds/cyclonedds_laptop.xml` automatically selects a network
adapter when each ROS process starts. Restart the laptop containers after changing
network connections. With multiple active adapters, Cyclone DDS can select a
network that does not reach the robot. In that case, replace `autodetermine="true"`
with `name="<robot-network-interface>"` in the laptop profile.

### 3) Build image and start container

```bash
cd Docker
docker compose up -d --build enexis_dev
```

### 4) Open a shell in the running container

```bash
docker exec -it enexis_dev bash
```

### 5) Launch bringup

```bash
ros2 launch go2_bringup go2.launch.py
```

Notes:
- Compose uses `network_mode: host`.
- GUI forwarding is preconfigured for WSLg/X11 mounts in `Docker/docker-compose.yml`.

---

## Laptop operator tools

Use this setup to visualize and command the real robot from a laptop without
starting duplicate hardware drivers, localization, navigation, or Gazebo.
The onboard robot stack must run separately. Stop any simulation or hardware
stack running on the laptop before connecting to the robot.

Select the laptop DDS configuration in `Docker/.env`. The laptop profile selects
an adapter automatically; if it selects the wrong network, set an explicit
interface name as described in Quick Start step 2. DDS discovery
and traffic must be permitted between the machines. Match the robot's
`ROS_DOMAIN_ID` (default `0`); export it in the host terminal before starting
this stack if the robot uses a different domain.

From the repository root, start RViz and the tools container:

```bash
docker compose -f Docker/enexis_go2/docker-laptop-compose.yml up -d
```

Both services use the existing `enexis/dev:latest` image. If needed, build it
first with `docker compose -f Docker/base_dev/docker-compose.yml build enexis-dev`.
RViz uses real time and the configuration baked into the image, not a source
volume. The stack has its own Compose project name, `enexis_laptop`.

Open a ROS tools shell and verify discovery:

```bash
docker compose -f Docker/enexis_go2/docker-laptop-compose.yml exec tools bash
ros2 node list
ros2 topic list
```

For keyboard control, run this inside the tools shell:

```bash
ros2 run teleop_twist_keyboard teleop_twist_keyboard \
  --ros-args -p speed:=0.2 -p turn:=0.5
```

Teleop publishes `geometry_msgs/Twist` on `/cmd_vel` by default. Confirm this
matches the robot's command topic before use. Keep the terminal focused:
`i` moves forward, `,` backward, `j`/`l` turn, and `k` stops.
Press `k` before exiting with `Ctrl+C`. Do not run keyboard control concurrently
with navigation goals or other velocity command publishers. Teleop does not
start automatically.

RViz can send goals with its 2D Goal Pose tool when onboard navigation and
localization are operational. To stop the laptop services:

```bash
docker compose -f Docker/enexis_go2/docker-laptop-compose.yml down
```

The laptop RViz uses the existing `rviz.rviz` view, with the supported SLAM
Toolbox and Nav2 panels, the **Go2 Nav Mode** panel, a `/map` occupancy-map
display, and an **AMCL Pose** display (`/amcl_pose`, with covariance). The
fixed frame is `map`, so the robot and scan only appear once mapping or
localization publishes `map -> odom`. In Idle mode, select `odom` under
**Global Options -> Fixed Frame**.
RViz itself does not run mapping, localization, or navigation; the Go2 Nav
Mode panel only asks the robot's mode manager to switch modes.

## Physical Go2 mapping and localization

Rebuild the robot image on the Jetson and the developer image on the laptop
after updating this repository so the installed launch files, mode manager,
interfaces, RViz panel, and RViz configuration are current:

```bash
# Jetson
docker compose -f Docker/enexis_go2/docker-compose.yml build platform_kit
# Laptop
docker compose -f Docker/base_dev/docker-compose.yml build enexis-dev
```

Maps are stored on the Jetson host and mounted into the `nav` container
at `/maps`. The default host directory is `$HOME/go2_maps`; to select another
directory, export `GO2_MAPS_DIR` before using Compose. Create the host
directory once before starting the stack:

```bash
mkdir -p "${GO2_MAPS_DIR:-$HOME/go2_maps}"
```

The physical workflow uses the existing `/scan` and local EKF
`odom -> base_footprint` transform. Start the whole robot stack, including
the `nav` service:

```bash
docker compose -f Docker/enexis_go2/docker-compose.yml up -d
```

### Modes

The `nav` service runs the `nav_mode_manager` node, which runs exactly one
mode at a time:

- **Idle**: nothing runs and nothing publishes `map -> odom`.
- **Mapping**: SLAM Toolbox (online async) with a new, empty map every time.
  Nav2 does not run.
- **Localization**: Nav2 Map Server, AMCL, and Nav2 with a static global
  costmap layer on a saved map.

At boot, the manager starts localization on `/maps/default.yaml` if it
exists. Otherwise it stays idle and reports
`No default map (/maps/default.yaml); choose a mode`. To load a map at boot,
save it as `/maps/default` (see [Mapping](#mapping)).

Switch modes with the **Go2 Nav Mode** panel in laptop RViz:

1. Choose **Idle**, **Mapping**, or **Localization**.
2. For localization, pick a saved map from the drop-down. **Refresh** reloads
   the list from the robot.
3. Press **Apply**.

The panel shows the robot's mode, map, state (`switching`,
`waiting_for_pose`, `ready`, or `failed`), and message, plus the reply to the
last request, such as a refusal. **Apply** is disabled while a switch is in
progress, but enabled while localization waits for the initial pose. A switch
stops the current mode cleanly before starting the new one, so two modes
never publish `map -> odom` together. Expect a switch to take about 10-15 s
on the Jetson.

Without RViz, use the services from a ROS shell. On the Jetson, open the
shell in the `nav` container; on the laptop, the `tools` container
(see [Laptop operator tools](#laptop-operator-tools)) also works:

```bash
docker compose -f Docker/enexis_go2/docker-compose.yml exec nav bash
ros2 topic echo /nav_mode/status
ros2 service call /nav_mode/list_maps go2_nav_interfaces/srv/ListMaps
ros2 service call /nav_mode/set_mode go2_nav_interfaces/srv/SetNavMode \
  "{mode: localization, map_name: office}"
ros2 service call /nav_mode/set_mode go2_nav_interfaces/srv/SetNavMode \
  "{mode: mapping}"
ros2 service call /nav_mode/set_mode go2_nav_interfaces/srv/SetNavMode \
  "{mode: idle}"
```

A response with `success: true` only means the request was accepted. Follow
the progress on `/nav_mode/status`. The manager refuses a request, with
`success: false` and the reason, in these cases:

- The mode is unknown.
- The map does not exist. Map names (without `.yaml`) may only use ASCII
  letters, digits, `_`, and `-`.
- Another switch is still in progress.
- A Nav2 goal is active (`Cancel the active navigation goal first`). Cancel
  it, for example in the Nav2 panel, and let the robot stop first.

If a mode fails to start, the manager stops everything it started, returns
to Idle, and reports state `failed` with the reason. This happens on a launch
error, when a step is not ready within `ready_timeout` (60 s by default; the
wait for the initial pose has no limit), or when its processes exit
unexpectedly at any time. A mode is never left half-started. Idle is always
available as a safe fallback. Stopping the `nav` container stops the active
mode cleanly first.

### Mapping

Mapping always starts a new, empty map. With the robot stationary, select
**Mapping** and press **Apply**. Mapping runs asynchronous SLAM Toolbox,
which publishes the `map -> odom` transform. Manually drive the robot to map
the environment. Nav2 is not started in mapping mode, and no autonomous goals
are sent. Stop the robot and allow the map to settle before saving.

Use the standard **SlamToolboxPlugin** RViz panel's **Save Map** control for
the occupancy map and **Serialize Map** for the pose graph. These are
**separate operations**: an occupancy map (`.yaml` and `.pgm`) is used by
AMCL/Nav2, whereas a serialized graph (`.posegraph` and `.data`) is needed
to resume SLAM. Saving one does not save the other. Only maps with a `.yaml`
file appear in the Go2 Nav Mode drop-down.

Enter an absolute, extension-free robot-container path such as
`/maps/warehouse` in the panel's corresponding filename field. Use
`/maps/default` for the map to load at boot. Even when RViz runs on the
laptop, files are written by SLAM Toolbox on the robot, not into the laptop
filesystem. `/maps` is the persistent host mount. Alternatively, call the
standard services from a ROS shell in the `nav` container:

```bash
docker compose -f Docker/enexis_go2/docker-compose.yml exec nav bash
ros2 service call /slam_toolbox/save_map slam_toolbox/srv/SaveMap \
  "{name: {data: '/maps/warehouse'}}"
ros2 service call /slam_toolbox/serialize_map \
  slam_toolbox/srv/SerializePoseGraph "{filename: '/maps/warehouse'}"
```

Check each response for `result: 0` (success) and confirm that all four files
exist in the host maps directory before switching modes. These operations
are not transactional and **can overwrite existing files without a guard**.
Back up existing maps and choose unused output prefixes to preserve them.
If either operation fails, inspect the logs and files before retrying; do
not assume the other operation succeeded or that a partial save was rolled back.

### Resume mapping from a pose graph

Switch to **Mapping**, which starts an empty map. Then use the SLAM Toolbox
panel's **Deserialize Map** control with the saved graph prefix and a
supported start mode. Alternatively, call the standard service from a ROS
shell in the `nav` container with the operator-known robot pose in the saved
map's `map` frame:

```bash
ros2 service call /slam_toolbox/deserialize_map \
  slam_toolbox/srv/DeserializePoseGraph \
  "{filename: '/maps/warehouse', match_type: 2, initial_pose: {x: 1.2, y: -0.4, theta: 1.57}}"
```

Replace the example x, y (metres), and theta/yaw (radians) with the
operator-determined pose; no pose is inferred. The installed Jazzy interface
uses `geometry_msgs/Pose2D` and defines `UNSET=0`, `START_AT_FIRST_NODE=1`,
`START_AT_GIVEN_POSE=2`, and `LOCALIZE_AT_POSE=3`. The example uses
`START_AT_GIVEN_POSE` to resume mapping, not SLAM Toolbox localization
(saved-map localization here uses AMCL instead). Inspect the installed
interface with `ros2 interface show slam_toolbox/srv/DeserializePoseGraph`
inside `nav` if needed.

Jazzy has an empty deserialization response: completion alone cannot
confirm successful loading. Check the
SLAM Toolbox logs for successful file loading and verify map/scan alignment in
RViz while stationary before manually mapping the extension. Save the
occupancy map and serialize the updated graph separately under a new,
unused prefix such as `/maps/warehouse-extension`.

### Localization and Nav2

Cancel any active goal and stop the robot before switching modes.
Localization requires an existing `/maps/<name>.yaml` map and its referenced
image (normally saved next to it). The manager checks the name and YAML
existence; Map Server validates and loads its contents. A broken map fails
the switch, and the robot returns to Idle.

Select **Localization**, choose the map, and press **Apply**. Localization
starts in two steps:

1. The manager starts Map Server and AMCL. When both are active, the state
   becomes `waiting_for_pose` and the panel shows
   **Waiting for initial pose (2D Pose Estimate)**. There is no time limit,
   also not for the boot default map.
2. In laptop RViz, use **2D Pose Estimate** to set the robot's pose in the
   map. As soon as AMCL publishes this pose, it also publishes
   `map -> odom`, and the manager starts Nav2. The state becomes `ready` when
   all Nav2 servers are active (within `ready_timeout`).

While waiting for the pose, you can still select another mode or map; the
manager stops Map Server and AMCL cleanly first. A broken map file is only
detected after `ready_timeout`, because Map Server keeps running in an error
state instead of exiting.

State `ready` means Map Server, AMCL, and all Nav2 servers are active. AMCL
alone publishes `map -> odom`; the local EKF continues to publish
`odom -> base_footprint`. Use **2D Goal Pose** only when ready to navigate.
Startup never dispatches or replays a goal automatically. Mapping remains
manual-drive only. The simulation Compose file and its default Nav2
launch/config remain unchanged.

## Main Launch Files

The `nav2` package's `nav2.launch.py` forwards `use_sim_time` to Nav2.
Simulation Compose continues to use its existing default navigation launch
and rolling costmap. Physical mapping and localization use
`go2_navigation.launch.py` with an explicit mode and a map-name argument only
for localization. `start_navigation:=false` (default `true`) starts only Map
Server and AMCL in localization mode. The `nav` service's
`nav_mode_manager.py` (installed by the `nav2` package; interfaces in
`go2_nav_interfaces`) runs this launch file with `start_navigation:=false`
and, after the initial pose, `nav2.launch.py` as separate child processes.
To launch them by hand for debugging, stop the `nav` service first so that
only one mode runs. Mapping includes the upstream SLAM Toolbox online async
launch; saving and resuming use its standard services and RViz panel.
The static-layer plugin is present but disabled in the default rolling-costmap
configuration; only physical localization enables it.
Simulated time requires an active `/clock` publisher for costmap updates.

The runnable synthetic regression harness
`robotics_modules/ros2_control/nav2/test/integration_go2_mapping.py` accepts an
empty output directory and exercises scan-to-map publication, standard
occupancy save and graph serialization services, pose-graph deserialization,
AMCL activation, Nav2 lifecycle activation, and the static global costmap.
Run it only in a `docker run --rm --network none` container with the updated
`nav2` package built and sourced and an isolated `ROS_DOMAIN_ID`; it publishes
synthetic sensor data and TF, never navigation goals.

The companion harness
`robotics_modules/ros2_control/nav2/test/integration_nav_mode_manager.py`
runs the same way (with `go2_nav_interfaces` also built and sourced) and drives
the installed mode manager: boot without and with a default map, switching
between mapping and localization, the two-step localization (waiting for the
initial pose, then Nav2), switching away while waiting, refusals (including a
synthetic active-goal status), failure of a broken map or crashed launch back
to Idle, and clean shutdown without leftover processes. Unit tests for the
manager's validation, map listing, pose freshness check, and process-group
shutdown run with `colcon test` on `nav2`.

All robot nodes (platform, Livox, localization, Nav2) start with `--log-level rmw_cyclonedds_cpp:=error`.
This hides the "Failed to parse type hash" warnings for Unitree DDS topics, but also every other Cyclone DDS warning.

- `go2_bringup/launch/go2.launch.py`
	- launches `go2_description`, `go2_driver`, optional `go2_rviz`
  - args: `namespace`, `rviz`
- `go2_description/launch/robot.launch.py`
	- publishes `robot_state_publisher` from URDF/Xacro
- `go2_driver/launch/go2_driver.launch.py`
	- starts `go2_driver` component container
	- runs `pointcloud_to_laserscan`
- `go2_rviz/launch/rviz.launch.py`
	- starts RViz with packaged config

### Livox MID360s

- `livox` package wrapper launch: `ros2 launch livox livox_mid360s.launch.py`
- default cloud topic is `livox/lidar` (native Livox topic)
- update Livox network settings in `robotics_modules/sensors/livox/config/MID360s_config.json`
- Docker service: `Docker/enexis_go2/docker-compose.yml` -> `livox`
- The platform launch does not start the Livox driver; the separate Docker
  service owns it. In hardware mode (`use_sim_time:=False`), the platform launch
  converts `/livox/lidar` to `/scan_unfiltered` with node `/livox_pc2l_scan`.
  Node `/livox_scan_filter` (`laser_filters` box filter) then removes the returns
  from the lidar protection mount (box around `lidar_link`, about ±10 cm in the
  scan plane) and publishes `/scan`. In simulation, both nodes are
  disabled because Gazebo supplies `/scan`.
- `/scan` keeps points 0.08–0.45 m above the ground: `min_height`/`max_height`
  are −0.22/0.15 m in `base_footprint`, which is level and at body height
  (~0.30 m when standing).
- In hardware mode, node `/imu_tilt_tf` publishes `base_footprint -> base_link`
  with the body roll and pitch from `/imu` (yaw removed). The URDF then leaves
  out its fixed `base_footprint` joint (xacro argument `imu_tilt:=true`, set by
  the platform launch). Without this, the ±5° body tilt while walking puts the
  ground into `/scan` from about 1 m away. Simulation keeps the fixed joint.
- The lidar mount pitch in `enexis_go2.xacro` is 18.5°. It was measured on the
  robot (Livox IMU vs Go2 IMU, and the ground plane in the cloud); the drawing
  value is 17°. If the ground shows up in `/scan` while standing still on a
  flat floor, check this value first.
- On hardware, the local EKF takes its yaw rate from the Livox gyro, not from
  the Go2 `/imu` (`livox_gyro:=True` in `localization.launch.py`). Measured on
  the robot, the Go2 gyro has a constant offset of 0.34°/s (the heading drifted
  ~20°/min while standing still) and reads turns ~3% too large. Node
  `/gyro_bias_correction` republishes `/livox/imu` as `/livox/imu_corrected`:
  it learns the gyro offset whenever the robot stands still (low gyro noise and
  no non-zero `/cmd_vel` for 1.5 s), subtracts it, and converts the Livox
  acceleration from g to m/s². `livox_frame` (the Livox IMU frame) is attached
  to `lidar_link` in `enexis_go2.xacro`. Simulation keeps `/imu`.
- Turn slowly while mapping. A lidar frame takes 100 ms and is not deskewed, so
  at 1.3 rad/s each frame is smeared over ~7.5°.

---

## ROS Interfaces

### Core Topics

Published by `go2_driver`:
- `/pointcloud` (`sensor_msgs/msg/PointCloud2`)
- `/joint_states` (`sensor_msgs/msg/JointState`)
- `/odom` (`nav_msgs/msg/Odometry`)
- `/imu` (`unitree_go/msg/IMUState`)
- `/api/sport/request` (`unitree_api/msg/Request`)

Subscribed by `go2_driver`:
- `/utlidar/cloud` (`sensor_msgs/msg/PointCloud2`)
- `/utlidar/robot_pose` (`geometry_msgs/msg/PoseStamped`)
- `/joy` (`sensor_msgs/msg/Joy`)
- `/lowstate` (`unitree_go/msg/LowState`)
- `/cmd_vel` (`geometry_msgs/msg/Twist`)

### Services (from `go2_interfaces`)

Exposed by `go2_driver`:
- `/body_height`
- `/continuous_gait`
- `/euler`
- `/foot_raise_height`
- `/mode`
- `/pose`
- `/speed_level`
- `/switch_gait`
- `/switch_joystick`

Example calls:

```bash
# Set mode
ros2 service call /mode go2_interfaces/srv/Mode "{mode: stand_up}"

# Set velocity command
ros2 topic pub /cmd_vel geometry_msgs/msg/Twist "{linear: {x: 0.2, y: 0.0, z: 0.0}, angular: {x: 0.0, y: 0.0, z: 0.1}}" -r 10

# Set gait (0..4)
ros2 service call /switch_gait go2_interfaces/srv/SwitchGait "{d: 1}"
```

---

## Development Commands

For day-to-day development inside the container, this workspace uses short aliases for common `colcon` and environment commands.

- `resource`: source ROS 2 and workspace setup files in one command.
- `cb`: build all packages with symlink install.
- `cbs`: build selected packages only.
- `ct`: run tests.
- `ctr`: print verbose test results.

### No discovery across machines

- Ensure both machines use the same DDS implementation (`rmw_cyclonedds_cpp`).
- Check `CYCLONEDDS_URI` points to a valid XML file.
- Verify XML network interface names match actual interfaces.
- Open UDP ports used by DDS discovery (commonly in `7400-7600` range).

### `/scan` stops while `/livox/lidar` still publishes

The converter drops a point cloud when one of its UDP fragments is lost. Check
`awk '/^Udp:/{getline; print $6}' /proc/net/snmp` (UDP `RcvbufErrors`) twice;
if it increases, apply the Jetson sysctl settings from Quick Start step 2.

Windows firewall example (PowerShell as Administrator):

```powershell
New-NetFirewallRule -DisplayName "ROS2 UDP 7400-7600" -Direction Inbound -Action Allow -Protocol UDP -LocalPort 7400-7600
New-NetFirewallRule -DisplayName "ROS2 UDP 7400-7600 Outbound" -Direction Outbound -Action Allow -Protocol UDP -LocalPort 7400-7600
```

## Appendix A: WSL2 Setup

# A guide to configuring WSL2 for using ROS2 on Windows 11

## Install Windows Terminal

First of all I recommend installing Windows Terminal if you haven't got it already.

[Install Windows Terminal](https://aka.ms/terminal)

## Install WSL2

Open PowerShell in **administrator** mode by right-clicking and selecting `Run as administrator`, enter the following command, wait for the install to finish and then restart your machine.

```powershell
wsl --install
```

WSL will install the latest long term support version of Ubuntu by default, this is exactly what we want for our ROS2 dev environment. If it for some reason fails to install Ubuntu then you can install it manually with this command:

```powershell
wsl --install -d Ubuntu
```

## Post install

Open Windows Terminal and click the arrow on the new tab icon and open Ubuntu. You will be prompted to choose a UNIX username and password.

> ⚠️ You will not see that you are typing in the terminal when inputting your password. This is normal.

When you have made a user you will be moved to your Ubuntu home directory. This is indicated in the terminal by `your-username@your-wsl-machine-name:~`, where the tilde character `~` is short for the path to your home folder.

The next step is to update the Ubuntu installation. You can do that by running the following command in the Ubuntu terminal.

```bash
sudo apt update && sudo apt upgrade -y
```

## Enable GUI Applications

To run Linux GUI apps, you should make sure you have the latest GPU drivers matching your system. This is needed in order to enable you to use a virtual GPU (vGPU) so you can benefit from hardware-accelerated OpenGL rendering.

- [**Intel** GPU driver](https://www.intel.com/content/www/us/en/download/19344/intel-graphics-windows-dch-drivers.html)
- [**AMD** GPU driver](https://www.amd.com/en/support)
- [**NVIDIA** GPU driver](https://www.nvidia.com/Download/index.aspx?lang=en-us)

## WSL2 Networking

To enable networking that reliably works you need Windows 11 Pro. This is because you need the Hyper-V tools in order to create a virtual network switch. For Windows 11 Home you can skip this section and use the less reliable workaround presented later.

### Hyper-V

Let’s check if Hyper-V is enabled. Do that by running this command in an administrator PowerShell prompt:

```powershell
Get-WindowsOptionalFeature -Online -FeatureName Microsoft-Hyper-V-All
```

If it is disabled, run the following command to enable Hyper-V. After it is done installing, restart your computer.

```powershell
Enable-WindowsOptionalFeature -Online -FeatureName Microsoft-Hyper-V -All
```

### Creating the Virtual Network Adapter

After you have Hyper-V installed, we can move on to creating the virtual network adapter. Start by running this command (in an administrator PowerShell) to get a list of your physical adapters:

```powershell
Get-NetAdapter
```

You need to choose either an Ethernet adapter or WiFi adapter to link the virtual switch to. To create the switch, run the following command. If you are using an Ethernet adapter, or your WiFi adapter has another name, replace `"WiFi"` with the name you got from the previous command:

```powershell
New-VMSwitch -Name "External Switch" -NetAdapterName "WiFi" -AllowManagementOS $true
```

## Configuring WSL2

Now we will configure WSL2 to use the virtual network adapter we just created. We will also configure the amount of RAM and swap space we want to make available to WSL2. We will do this by entering this command in PowerShell. It will open the configuration file in Notepad:

```powershell
notepad .wslconfig
```

If you have 32GB of RAM (or more) you can just replace your config file with this:

```ini
[wsl2]
swap=8589934592
memory=25769803776
networkingMode=bridged
vmSwitch="External Switch"
```

If you have less than 32GB, then you need to replace the memory amount with:

- `memory=12884901888` for 16GB (or more)
- `memory=6442450944` for 8GB (or more)

> ⚠️ You can allocate less, but you will likely encounter issues with building large ROS2 packages.

If you have allocated the suggested amount and still encounter memory-related issues when building ROS2 packages with `colcon`, you can try increasing the swap amount to:

```ini
swap=17179869184
```

## Workaround for Windows 11 Home

Since we can’t make a virtual network adapter without Hyper-V, we can do the next best thing: mirror the Windows network adapter. This will enable the WSL2 installation to communicate with other devices on your LAN.

This is built into WSL2, so the only thing we need to do is enable it in the config file:

```powershell
notepad .wslconfig
```

If you followed the previous step, you should already have stuff in your config file. Remove:

```ini
vmSwitch="External Switch"
```

Replace:

```ini
networkingMode=bridged
```

with:

```ini
networkingMode=mirrored
```

Your config should now look like this:

```ini
[wsl2]
swap=8589934592
memory=25769803776
networkingMode=mirrored
```

Next step is to allow inbound connections. This is needed in order to be able to detect ROS2 nodes running on other machines. Do it by running this command in an administrator PowerShell:

```powershell
Set-NetFirewallHyperVVMSetting -Name '{40E0AC32-46A5-438A-A0B2-2B479E8F2E90}' -DefaultInboundAction Allow
```

## Some "Nice to Haves"

If you want to open a folder from WSL in windows explorer, you can do so by typing `explorer .`

If you want to seamlessly open your Ubuntu projects in VSCode, follow this tutorial to set up VSCode for WSL [Get started using VS Code with WSL | Microsoft Learn](https://learn.microsoft.com/en-us/windows/wsl/tutorials/wsl-vscode)

## Install ROS2

Now WSL is up and running, the final step is to install ROS2 and start testing that everything works. Follow the guide on how to install for Ubuntu (deb packages) from the ROS2 documentation. [Ubuntu (deb packages) — ROS 2 Documentation: Jazzy documentation](https://docs.ros.org/en/jazzy/Installation/Ubuntu-Install-Debs.html)

## Troubleshooting

If you get issues with the `ros2 node list` command, it could be that the daemon service is not running. This command should fix it:

```bash
ros2 daemon start
```

If you still have issues detecting nodes on other machines, it is likely that the Windows firewall is blocking some of the traffic. Run these commands (in an administrator PowerShell) to make exceptions for ROS2 ports, and afterwards reboot your machine:

```powershell
New-NetFirewallRule -DisplayName "ROS2 UDP 7400-7600" -Direction Inbound -Action Allow -Protocol UDP -LocalPort 7400-7600
```

```powershell
New-NetFirewallRule -DisplayName "ROS2 UDP 7400-7600 Outbound" -Direction Outbound -Action Allow -Protocol UDP -LocalPort 7400-7600
```

## References

[Install WSL](https://learn.microsoft.com/en-us/windows/wsl/install)

[Accessing network applications with WSL](https://learn.microsoft.com/en-us/windows/wsl/networking)

[Set up a WSL development environment](https://learn.microsoft.com/en-us/windows/wsl/setup/environment#set-up-your-linux-username-and-password)

[Get started using VS Code with WSL](https://learn.microsoft.com/en-us/windows/wsl/tutorials/wsl-vscode)