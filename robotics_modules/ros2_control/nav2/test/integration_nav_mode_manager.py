#!/usr/bin/env python3
"""
Exercise the installed Go2 nav mode manager in an isolated ROS domain.

Run inside a --network none container after building and sourcing the
go2_nav_interfaces and nav2 packages. Pass an empty writable directory as the
sole argument. No robot drivers run, and no navigation goals or velocity
commands are sent; the active goal is a synthetic action status message.
"""

import os
from pathlib import Path
import signal
import subprocess
import sys
import time

from action_msgs.msg import GoalStatus, GoalStatusArray
from ament_index_python.packages import get_package_prefix
from geometry_msgs.msg import PoseWithCovarianceStamped, Twist
from go2_nav_interfaces.msg import NavModeStatus
from go2_nav_interfaces.srv import ListMaps, SetNavMode
from integration_go2_mapping import active, call, start_synthetic_sensors, wait
from nav_msgs.msg import OccupancyGrid
import rclpy
from rclpy.qos import DurabilityPolicy, QoSProfile, ReliabilityPolicy
from slam_toolbox.srv import SaveMap

IDLE = SetNavMode.Request.IDLE
MANAGER = Path(get_package_prefix('nav2')) / 'lib' / 'nav2' / 'nav_mode_manager.py'
NAV2_NODES = ('planner_server', 'controller_server', 'bt_navigator')
NAV_NODES = ('slam_toolbox', 'map_server', 'amcl') + NAV2_NODES
WAITING = NavModeStatus.WAITING_FOR_POSE


def process_table():
    table = {}
    for stat in Path('/proc').glob('[0-9]*/stat'):
        try:
            fields = stat.read_text().rsplit(')', 1)[1].split()
            cmdline = (stat.parent / 'cmdline').read_bytes().replace(b'\0', b' ').decode()
        except (OSError, IndexError):
            continue
        table[int(stat.parent.name)] = (fields[0], int(fields[1]), cmdline)
    return table


def foreign_processes(*allowed):
    """Return live processes that are neither this harness, its ancestors nor allowed."""
    table = process_table()
    keep = set(allowed)
    pid = os.getpid()
    while pid in table:
        keep.add(pid)
        pid = table[pid][1]
    return {
        pid: cmdline for pid, (state, _, cmdline) in table.items()
        if pid not in keep and state != 'Z'
    }


def main():
    directory = Path(sys.argv[1]).resolve()
    directory.mkdir(parents=True, exist_ok=True)
    assert not list(directory.iterdir()), 'Use an empty test directory'
    maps_dir = directory / 'maps'
    maps_dir.mkdir()
    rclpy.init()
    node = rclpy.create_node('synthetic_nav_mode_fixture')
    start_synthetic_sensors(node)
    latched = QoSProfile(
        depth=1,
        reliability=ReliabilityPolicy.RELIABLE,
        durability=DurabilityPolicy.TRANSIENT_LOCAL,
    )
    statuses = []
    maps = []
    commands = []
    poses = []
    node.create_subscription(NavModeStatus, '/nav_mode/status', statuses.append, latched)
    node.create_subscription(OccupancyGrid, '/map', maps.append, latched)
    node.create_subscription(Twist, '/cmd_vel', commands.append, 10)
    node.create_subscription(PoseWithCovarianceStamped, '/amcl_pose', poses.append, 10)
    initial_pub = node.create_publisher(PoseWithCovarianceStamped, '/initialpose', 10)
    goal_pub = node.create_publisher(
        GoalStatusArray, '/navigate_to_pose/_action/status', latched)
    managers = []
    logs = []

    def start_manager(label, *params):
        statuses.clear()
        log = (directory / f'manager-{label}.log').open('w')
        logs.append(log)
        args = ['--ros-args', '-p', f'maps_dir:={maps_dir}']
        for param in params:
            args += ['-p', param]
        manager = subprocess.Popen(
            [str(MANAGER)] + args, stdout=log, stderr=subprocess.STDOUT)
        managers.append(manager)
        return manager

    def stop_manager(manager, sig):
        manager.send_signal(sig)
        manager.wait(timeout=60)
        assert manager.returncode == 0, manager.returncode
        deadline = time.monotonic() + 5
        while foreign_processes() and time.monotonic() < deadline:
            time.sleep(0.2)
        leftovers = foreign_processes()
        assert not leftovers, f'Child processes survived the manager: {leftovers}'
        print(f'PASS: manager exits on {signal.Signals(sig).name} '
              'and leaves no child processes', flush=True)

    def latest():
        return statuses[-1] if statuses else None

    def wait_status(mode, state, description, text='', map_name='', timeout=120):
        wait(
            node,
            lambda: latest() is not None and latest().mode == mode
            and latest().state == state and latest().map_name == map_name
            and text in latest().message,
            f'{description}: {mode}/{state}', timeout,
        )
        print(f'      status message: {latest().message}', flush=True)

    def set_mode(mode, map_name=''):
        return call(node, SetNavMode, '/nav_mode/set_mode',
                    SetNavMode.Request(mode=mode, map_name=map_name))

    def refused(mode, map_name, text, description):
        response = set_mode(mode, map_name)
        assert not response.success and text in response.message, response
        print(f'PASS: refused {description}: {response.message}', flush=True)

    def gone(name):
        wait(node, lambda: name not in node.get_node_names(), f'{name} gone', 30)

    def save(name):
        request = SaveMap.Request()
        request.name.data = str(maps_dir / name)
        assert call(node, SaveMap, '/slam_toolbox/save_map', request).result == 0
        assert (maps_dir / f'{name}.yaml').is_file()

    def list_maps():
        return list(call(node, ListMaps, '/nav_mode/list_maps', ListMaps.Request()).map_names)

    def publish_goal(state):
        goal_pub.publish(GoalStatusArray(status_list=[GoalStatus(status=state)]))

    def set_initial_pose():
        poses.clear()
        deadline = time.monotonic() + 30
        while not poses:
            assert time.monotonic() < deadline, 'AMCL did not accept the initial pose'
            initial = PoseWithCovarianceStamped()
            initial.header.frame_id = 'map'
            initial.header.stamp = node.get_clock().now().to_msg()
            initial.pose.pose.orientation.w = 1.0
            initial.pose.covariance[0] = 0.01
            initial.pose.covariance[7] = 0.01
            initial.pose.covariance[35] = 0.01
            initial_pub.publish(initial)
            settle(0.5)
        print('PASS: AMCL accepted the initial pose', flush=True)

    def wait_for_pose_state(map_name, description):
        wait_status('localization', WAITING, description, 'set the initial pose',
                    map_name=map_name, timeout=60)

    def localize(map_name, description):
        wait_for_pose_state(map_name, f'{description}: map loaded')
        set_initial_pose()
        wait_status('localization', NavModeStatus.READY, f'{description}: Nav2 ready',
                    map_name=map_name)
        for name in ('map_server', 'amcl') + NAV2_NODES:
            assert active(node, name), name
        print('PASS: map_server, amcl and Nav2 active', flush=True)

    def settle(seconds):
        deadline = time.monotonic() + seconds
        while time.monotonic() < deadline:
            rclpy.spin_once(node, timeout_sec=0.05)

    def launch_pid(manager, launch_file):
        table = process_table()
        return next(
            pid for pid, (_, parent, cmdline) in table.items()
            if parent == manager.pid and launch_file in cmdline
        )

    def crash(manager, launch_file, description):
        os.kill(launch_pid(manager, launch_file), signal.SIGKILL)
        wait_status(IDLE, NavModeStatus.FAILED, description, 'unexpectedly', timeout=60)
        for name in NAV_NODES:
            gone(name)
        leftovers = foreign_processes(manager.pid)
        assert not leftovers, f'Orphans after launch crash: {leftovers}'
        print('PASS: no orphaned nodes after the crash', flush=True)

    try:
        manager = start_manager('empty')
        wait_status(IDLE, NavModeStatus.READY,
                    'boot without default map is idle', 'No default map')
        assert list_maps() == []
        refused('navigate', '', 'Unknown mode', 'unknown mode')
        refused('localization', '../maps', 'map_name', 'unsafe map name')
        refused('localization', 'missing', 'Map not found', 'missing map')

        assert set_mode('mapping').success
        wait_status('mapping', NavModeStatus.READY, 'mapping ready')
        assert active(node, 'slam_toolbox')
        wait(node, lambda: maps and maps[-1].info.width > 0, 'SLAM publishes map')
        assert 'amcl' not in node.get_node_names()
        save('office')
        save('default')
        assert list_maps() == ['default', 'office'], list_maps()
        print('PASS: list_maps returns saved maps', flush=True)

        assert set_mode('localization', 'office').success
        refused('idle', '', 'already in progress', 'request while switching')
        wait_for_pose_state('office', 'localization loads map, waits for pose')
        for name in ('map_server', 'amcl'):
            assert active(node, name), name
        gone('slam_toolbox')
        settle(15.0)
        assert latest().state == WAITING, latest()
        started = set(NAV2_NODES) & set(node.get_node_names())
        assert not started, f'Nav2 started before the initial pose: {started}'
        print('PASS: still waiting for the pose after 15 s; Nav2 not started', flush=True)
        set_initial_pose()
        wait_status('localization', NavModeStatus.READY, 'Nav2 ready after the pose',
                    map_name='office')
        for name in NAV2_NODES:
            assert active(node, name), name
        print('PASS: Nav2 active', flush=True)

        publish_goal(GoalStatus.STATUS_EXECUTING)
        settle(2.0)
        refused('mapping', '', 'Cancel the active navigation goal first', 'active goal')
        publish_goal(GoalStatus.STATUS_SUCCEEDED)
        settle(2.0)
        assert set_mode('mapping').success, 'switch refused after the goal finished'
        wait_status('mapping', NavModeStatus.READY, 'mapping ready after localization')
        assert active(node, 'slam_toolbox')
        for name in ('amcl', 'map_server') + NAV2_NODES:
            gone(name)

        crash(manager, 'go2_navigation.launch.py', 'mapping launch crash fails to idle')

        assert set_mode('localization', 'default').success
        localize('default', 'localization on default')
        stop_manager(manager, signal.SIGTERM)

        manager = start_manager('default', 'ready_timeout:=45.0')
        wait_for_pose_state('default', 'boot loads default map, waits for pose')
        assert set_mode('localization', 'office').success
        print('PASS: switch accepted while waiting for the pose', flush=True)
        wait_for_pose_state('office', 'other map while waiting')
        assert set_mode('idle').success
        wait_status(IDLE, NavModeStatus.READY, 'idle while waiting for the pose', 'Idle')
        for name in NAV_NODES:
            gone(name)
        leftovers = foreign_processes(manager.pid)
        assert not leftovers, f'Processes left after leaving the pose wait: {leftovers}'
        print('PASS: leaving the pose wait stops the localization launch', flush=True)

        assert set_mode('localization', 'default').success
        wait_for_pose_state('default', 'localization before crash')
        crash(manager, 'go2_navigation.launch.py', 'crash while waiting fails to idle')
        assert set_mode('localization', 'default').success
        localize('default', 'localization before Nav2 crash')
        crash(manager, 'nav2.launch.py', 'Nav2 launch crash fails to idle')

        (maps_dir / 'broken.yaml').write_text(
            'image: missing.pgm\nresolution: 0.05\norigin: [0.0, 0.0, 0.0]\n'
            'negate: 0\noccupied_thresh: 0.65\nfree_thresh: 0.25\n')
        assert set_mode('localization', 'broken').success
        wait_status(IDLE, NavModeStatus.FAILED, 'broken map fails to idle', 'broken',
                    timeout=120)
        for name in NAV_NODES:
            gone(name)
        leftovers = foreign_processes(manager.pid)
        assert not leftovers, f'Processes left after failure: {leftovers}'
        print('PASS: failed switch leaves no mode running', flush=True)
        assert set_mode('mapping').success
        wait_status('mapping', NavModeStatus.READY, 'mapping before SIGINT')
        stop_manager(manager, signal.SIGINT)

        assert not any(abs(value) > 0 for cmd in commands for value in (
            cmd.linear.x, cmd.linear.y, cmd.angular.z,
        )), 'Unexpected nonzero velocity command'
        print('PASS: no nonzero velocity commands', flush=True)
    finally:
        for manager in managers:
            if manager.poll() is None:
                manager.send_signal(signal.SIGINT)
                try:
                    manager.wait(timeout=60)
                except subprocess.TimeoutExpired:
                    manager.kill()
        for log in logs:
            log.close()
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
