# Livox MID360s Integration

This package provides a stable wrapper launch for the upstream `livox_ros_driver2` package and keeps native Livox topics by default.

## What this package does

- Starts `livox_ros_driver2_node`.
- Uses `config/MID360s_config.json` by default.
- Publishes cloud on `livox/lidar` by default.

## Configure your network and lidar IP

Edit `config/MID360s_config.json` and set:

- `host_net_info[0].host_ip`: IP of the host NIC connected to the MID360s.
- `lidar_configs[0].ip`: IP of your MID360s device.

## Launch directly

```bash
ros2 launch livox livox_mid360s.launch.py
```

Useful overrides:

```bash
ros2 launch livox livox_mid360s.launch.py \
  config_path:=/workspace/install/livox/share/livox/config/MID360s_config.json \
  frame_id:=lidar_link \
  cloud_topic:=livox/lidar \
  publish_freq:=20.0
```

## Run with compose

Use the `livox` service in `Docker/enexis_go2/docker-compose.yml`.

```bash
cd Docker/enexis_go2
docker compose up -d --build platform_kit livox
```
