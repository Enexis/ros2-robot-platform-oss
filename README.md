# ROS 2 Robot Platform OSS

ROS 2 Jazzy workspace for ROS 2 robot platforms in a modular way. Below is the folder structure and a short description of each (sub)folder.

Project structure

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

## Repository Layout

```text
.
├── cyclonedds/                  # CycloneDDS interface profiles
├── Docker/                      # Dockerfile and compose setup
└── packages/
		├── go2_bringup/             # Top-level bringup launch
		├── go2_description/         # URDF/Xacro, robot_state_publisher launch
		├── go2_driver/              # ROS 2 component driver bridge
		├── go2_interfaces/          # Service definitions
		├── go2_rviz/                # RViz launch + config
		├── unitree_api/             # Unitree API messages
		└── unitree_go/              # Unitree robot messages
```

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

For laptop usage, set the interface names in `cyclonedds/cyclonedds_laptop.xml` to match your host network adapters.

### 3) Build image and start container

```bash
cd Docker
docker compose up -d --build ros2-go2
```

### 4) Open a shell in the running container

```bash
docker exec -it ros2-go2-dev bash
```

### 5) Launch bringup

```bash
ros2 launch go2_bringup go2.launch.py
```

Notes:
- Compose uses `network_mode: host`.
- GUI forwarding is preconfigured for WSLg/X11 mounts in `Docker/docker-compose.yml`.

---

## Main Launch Files

- `go2_bringup/launch/go2.launch.py`
	- launches `go2_description`, `go2_driver`, optional `go2_rviz`
	- args: `rviz` (`True/False`), `lidar` and `realsense` (reserved)
- `go2_description/launch/robot.launch.py`
	- publishes `robot_state_publisher` from URDF/Xacro
- `go2_driver/launch/go2_driver.launch.py`
	- starts `go2_driver` component container
	- runs `pointcloud_to_laserscan`
- `go2_rviz/launch/rviz.launch.py`
	- starts RViz with packaged config

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