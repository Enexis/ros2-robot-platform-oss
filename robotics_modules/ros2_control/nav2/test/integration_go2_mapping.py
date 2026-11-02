#!/usr/bin/env python3
"""
Exercise the installed Go2 mapping workflow in an isolated ROS domain.

Run inside a --network none container after building and sourcing the package.
Pass an empty writable directory as the sole argument. No robot drivers run.
"""

import math
import os
from pathlib import Path
import signal
import subprocess
import sys
import time

from geometry_msgs.msg import PoseWithCovarianceStamped, TransformStamped, Twist
from lifecycle_msgs.srv import GetState
from nav_msgs.msg import OccupancyGrid, Odometry
from rcl_interfaces.srv import GetParameters
import rclpy
from rclpy.qos import DurabilityPolicy, qos_profile_sensor_data, QoSProfile
from sensor_msgs.msg import LaserScan
from slam_toolbox.srv import DeserializePoseGraph, SaveMap, SerializePoseGraph
from tf2_ros import StaticTransformBroadcaster, TransformBroadcaster


def wait(node, predicate, description, timeout=40, report=True):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        rclpy.spin_once(node, timeout_sec=0.05)
        if predicate():
            if report:
                print(f'PASS: {description}', flush=True)
            return
    raise AssertionError(f'Timed out: {description}')


def call(node, kind, name, request):
    client = node.create_client(kind, name)
    assert client.wait_for_service(timeout_sec=15), name
    future = client.call_async(request)
    wait(node, future.done, name, report=False)
    response = future.result()
    node.destroy_client(client)
    return response


def active(node, name):
    return call(node, GetState, f'/{name}/get_state', GetState.Request()).current_state.id == 3


def main():
    directory = Path(sys.argv[1]).resolve()
    directory.mkdir(parents=True, exist_ok=True)
    assert not list(directory.iterdir()), 'Use an empty test directory'
    rclpy.init()
    node = rclpy.create_node('synthetic_go2_fixture')
    broadcaster = TransformBroadcaster(node)
    static = StaticTransformBroadcaster(node)
    sensor = TransformStamped()
    sensor.header.frame_id = 'base_footprint'
    sensor.child_frame_id = 'synthetic_laser'
    sensor.transform.rotation.w = 1.0
    sensor.header.stamp = node.get_clock().now().to_msg()
    static.sendTransform(sensor)
    scan_pub = node.create_publisher(LaserScan, '/scan', qos_profile_sensor_data)
    odom_pub = node.create_publisher(Odometry, '/odometry/local', 10)
    maps = []
    costs = []
    poses = []
    commands = []
    transient = QoSProfile(depth=1, durability=DurabilityPolicy.TRANSIENT_LOCAL)
    node.create_subscription(OccupancyGrid, '/map', maps.append, transient)
    node.create_subscription(OccupancyGrid, '/global_costmap/costmap', costs.append, transient)
    node.create_subscription(PoseWithCovarianceStamped, '/amcl_pose', poses.append, 10)
    node.create_subscription(Twist, '/cmd_vel', commands.append, 10)
    initial_pub = node.create_publisher(PoseWithCovarianceStamped, '/initialpose', 10)

    def publish():
        stamp = node.get_clock().now().to_msg()
        transform = TransformStamped()
        transform.header.stamp = stamp
        transform.header.frame_id = 'odom'
        transform.child_frame_id = 'base_footprint'
        transform.transform.rotation.w = 1.0
        broadcaster.sendTransform(transform)
        odom = Odometry()
        odom.header.stamp = stamp
        odom.header.frame_id = 'odom'
        odom.child_frame_id = 'base_footprint'
        odom.pose.pose.orientation.w = 1.0
        odom_pub.publish(odom)
        scan = LaserScan()
        scan.header.stamp = stamp
        scan.header.frame_id = 'synthetic_laser'
        scan.angle_min = -math.pi
        scan.angle_max = math.pi
        scan.angle_increment = 2 * math.pi / 360
        scan.time_increment = 0.0
        scan.scan_time = 0.1
        scan.range_min = 0.05
        scan.range_max = 20.0
        # Ray intersections with an asymmetric room [-4, 6] x [-3, 5].
        for index in range(361):
            angle = scan.angle_min + index * scan.angle_increment
            x, y = math.cos(angle), math.sin(angle)
            distances = []
            if abs(x) > 1e-6:
                distances.append((6 if x > 0 else -4) / x)
            if abs(y) > 1e-6:
                distances.append((5 if y > 0 else -3) / y)
            scan.ranges.append(min(distances))
        scan_pub.publish(scan)

    node.create_timer(0.1, publish)
    processes = []
    logs = []

    def start(mode, name='', label=''):
        log = (directory / f'{mode}-{label or name or "fresh"}.log').open('w')
        logs.append(log)
        process = subprocess.Popen(
            ['ros2', 'launch', 'nav2', 'go2_navigation.launch.py',
             f'mode:={mode}', f'maps_dir:={directory}', 'use_sim_time:=False']
            + ([f'map_name:={name}'] if name else []),
            stdout=log, stderr=subprocess.STDOUT, start_new_session=True,
        )
        processes.append(process)
        return process

    def stop(process):
        if process.poll() is None:
            os.killpg(process.pid, signal.SIGINT)
            try:
                process.wait(timeout=12)
            except subprocess.TimeoutExpired:
                os.killpg(process.pid, signal.SIGKILL)
                process.wait(timeout=5)

    def save(name):
        prefix = str(directory / name)
        serialized = call(
            node, SerializePoseGraph, '/slam_toolbox/serialize_map',
            SerializePoseGraph.Request(filename=prefix),
        )
        assert serialized.result == SerializePoseGraph.Response.RESULT_SUCCESS
        request = SaveMap.Request()
        request.name.data = prefix
        saved = call(node, SaveMap, '/slam_toolbox/save_map', request)
        assert saved.result == SaveMap.Response.RESULT_SUCCESS
        for suffix in ('.yaml', '.pgm', '.posegraph', '.data'):
            asset = directory / f'{name}{suffix}'
            assert asset.is_file() and asset.stat().st_size, str(asset)
        print(f'PASS: standard services save occupancy and pose graph: {name}', flush=True)

    try:
        mapping = start('mapping')
        wait(node, lambda: active(node, 'slam_toolbox'), 'SLAM Toolbox lifecycle-active')
        wait(
            node, lambda: maps and maps[-1].info.width > 0,
            'SLAM consumes scans and publishes map',
        )
        assert 'controller_server' not in node.get_node_names()
        save('synthetic')
        stop(mapping)
        maps.clear()
        resumed = start('mapping', label='resumed')
        wait(node, lambda: active(node, 'slam_toolbox'), 'resumed SLAM lifecycle-active')
        request = DeserializePoseGraph.Request()
        request.filename = str(directory / 'synthetic')
        request.match_type = DeserializePoseGraph.Request.START_AT_GIVEN_POSE
        request.initial_pose.x = 0.0
        request.initial_pose.y = 0.0
        request.initial_pose.theta = 0.0
        call(node, DeserializePoseGraph, '/slam_toolbox/deserialize_map', request)
        wait(node, lambda: maps and maps[-1].info.width > 0, 'resumed pose graph publishes map')
        save('extension')
        stop(resumed)
        assert f'Load From File {directory}/synthetic.posegraph' in (
            directory / 'mapping-resumed.log'
        ).read_text(), 'Expected the saved pose graph to be deserialized'
        maps.clear()
        localization = start('localization', 'synthetic')
        for name in ('map_server', 'amcl'):
            wait(node, lambda name=name: active(node, name), f'{name} active')
        wait(node, lambda: maps and maps[-1].info.width > 0, 'Map Server loads saved occupancy')
        for _ in range(5):
            initial = PoseWithCovarianceStamped()
            initial.header.frame_id = 'map'
            initial.header.stamp = node.get_clock().now().to_msg()
            initial.pose.pose.orientation.w = 1.0
            initial.pose.covariance[0] = 0.01
            initial.pose.covariance[7] = 0.01
            initial.pose.covariance[35] = 0.01
            initial_pub.publish(initial)
            rclpy.spin_once(node, timeout_sec=0.2)
        wait(node, lambda: bool(poses), 'AMCL consumes scan and publishes pose')
        wait(node, lambda: active(node, 'planner_server'), 'Nav2 planner active', 60)
        wait(node, lambda: costs and costs[-1].info.width > 0, 'static global costmap published')
        for name in (
            'controller_server', 'smoother_server', 'route_server', 'behavior_server',
            'bt_navigator', 'waypoint_follower', 'velocity_smoother',
            'collision_monitor', 'docking_server',
        ):
            if name in node.get_node_names():
                wait(node, lambda name=name: active(node, name), f'Nav2 {name} active', 60)
        params = call(
            node, GetParameters, '/global_costmap/global_costmap/get_parameters',
            GetParameters.Request(names=['plugins', 'rolling_window', 'static_layer.enabled']),
        ).values
        assert 'static_layer' in params[0].string_array_value
        assert params[1].bool_value is False and params[2].bool_value is True
        assert costs[-1].info.width == maps[-1].info.width
        assert costs[-1].info.height == maps[-1].info.height
        assert not any(abs(value) > 0 for cmd in commands for value in (
            cmd.linear.x, cmd.linear.y, cmd.angular.z,
        )), 'Unexpected nonzero command without a navigation goal'
        assert 'slam_toolbox' not in node.get_node_names()
        print('PASS: saved-map localization, static-layer dimensions, no motion goals', flush=True)
        stop(localization)
    finally:
        for process in reversed(processes):
            stop(process)
        for log in logs:
            log.close()
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
