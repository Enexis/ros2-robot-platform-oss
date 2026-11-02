#!/usr/bin/env python3
"""Switch the physical Go2 between idle, mapping and saved-map localization."""

import os
from pathlib import Path
import re
import signal
import subprocess
import threading
import time

from action_msgs.msg import GoalStatus, GoalStatusArray
from geometry_msgs.msg import PoseWithCovarianceStamped
from go2_nav_interfaces.msg import NavModeStatus
from go2_nav_interfaces.srv import ListMaps, SetNavMode
from lifecycle_msgs.msg import State
from lifecycle_msgs.srv import GetState
import rclpy
from rclpy.qos import DurabilityPolicy, QoSProfile, ReliabilityPolicy
from rclpy.signals import SignalHandlerOptions
from std_srvs.srv import Trigger

IDLE = SetNavMode.Request.IDLE
MAPPING = SetNavMode.Request.MAPPING
LOCALIZATION = SetNavMode.Request.LOCALIZATION
MODES = (IDLE, MAPPING, LOCALIZATION)
MAP_NAME_PATTERN = re.compile(r'[A-Za-z0-9_-]+')
GOAL_STATUS_TOPICS = (
    '/navigate_to_pose/_action/status',
    '/navigate_through_poses/_action/status',
    '/follow_waypoints/_action/status',
)
ACTIVE_GOAL_STATES = (
    GoalStatus.STATUS_ACCEPTED,
    GoalStatus.STATUS_EXECUTING,
    GoalStatus.STATUS_CANCELING,
)
ACTIVE_GOAL_MESSAGE = 'Cancel the active navigation goal first'


def list_maps(maps_dir):
    """Return the sorted basenames of the selectable map YAML files."""
    try:
        return sorted(
            path.stem for path in Path(maps_dir).glob('*.yaml')
            if path.is_file() and MAP_NAME_PATTERN.fullmatch(path.stem)
        )
    except OSError:
        return []


def validate_request(mode, map_name, maps_dir):
    """Return an error message for an invalid mode request, otherwise None."""
    if mode not in MODES:
        return f'Unknown mode "{mode}"; use idle, mapping or localization'
    if mode != LOCALIZATION:
        return None
    if not MAP_NAME_PATTERN.fullmatch(map_name):
        return 'Localization requires a map_name using ASCII letters, digits, _ or -'
    map_yaml = Path(maps_dir) / f'{map_name}.yaml'
    if not map_yaml.is_file():
        return f'Map not found: {map_yaml}'
    return None


def goal_active(status_arrays):
    """Return True if any latest action status array reports an unfinished goal."""
    return any(
        status.status in ACTIVE_GOAL_STATES
        for array in status_arrays
        for status in array.status_list
    )


def pose_is_fresh(stamp, since):
    """Return True if a builtin_interfaces/Time stamp is not older than since."""
    return since is not None and (stamp.sec, stamp.nanosec) >= (since.sec, since.nanosec)


def first_exit(children):
    """Return a description of the first exited (name, process) child, otherwise None."""
    for name, process in children:
        if process.poll() is not None:
            return f'{name} exited with code {process.returncode}'
    return None


def describe(mode, map_name):
    return f'localization on map "{map_name}"' if mode == LOCALIZATION else mode


def _group_alive(pgid):
    # Zombies do not count: they cannot publish or hold DDS endpoints.
    for stat in Path('/proc').glob('[0-9]*/stat'):
        try:
            fields = stat.read_text().rsplit(')', 1)[1].split()
        except (OSError, IndexError):
            continue
        if int(fields[2]) == pgid and fields[0] != 'Z':
            return True
    return False


def _reap_group(pgid):
    # Group members orphaned by a killed launch are reparented to PID 1 (this
    # process in Compose); reap them so they do not linger as zombies.
    while True:
        try:
            pid, _ = os.waitpid(-pgid, os.WNOHANG)
        except ChildProcessError:
            return
        if pid == 0:
            return


def stop_process_group(process, interrupt_timeout=15.0):
    """Stop a child started with start_new_session and every process in its group."""
    pgid = process.pid
    for sig, timeout in (
        (signal.SIGINT, interrupt_timeout),
        (signal.SIGTERM, 5.0),
        (signal.SIGKILL, 5.0),
    ):
        process.poll()
        _reap_group(pgid)
        if not _group_alive(pgid):
            break
        try:
            os.killpg(pgid, sig)
        except ProcessLookupError:
            break
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            process.poll()
            _reap_group(pgid)
            if not _group_alive(pgid):
                break
            time.sleep(0.1)
    process.wait()
    _reap_group(pgid)


class NavModeManager:

    def __init__(self, node):
        self._node = node
        self._maps_dir = node.declare_parameter('maps_dir', '/maps').value
        self._default_map = node.declare_parameter('default_map', 'default').value
        self._ready_timeout = float(node.declare_parameter('ready_timeout', 60.0).value)
        self._lock = threading.Lock()
        self._stopping = threading.Event()
        self._switching = False
        self._waiting_pose = False
        self._pose_since = None
        self._worker = None
        self._children = []
        self._mode = IDLE
        self._map_name = ''
        self._goal_status = {}

        latched = QoSProfile(
            depth=1,
            reliability=ReliabilityPolicy.RELIABLE,
            durability=DurabilityPolicy.TRANSIENT_LOCAL,
        )
        self._status_pub = node.create_publisher(NavModeStatus, '/nav_mode/status', latched)
        for topic in GOAL_STATUS_TOPICS:
            node.create_subscription(
                GoalStatusArray, topic,
                lambda msg, topic=topic: self._on_goal_status(topic, msg), latched,
            )
        # Volatile (compatible with any AMCL durability) so no stale latched pose is replayed.
        node.create_subscription(
            PoseWithCovarianceStamped, '/amcl_pose', self._on_amcl_pose, 10)
        self._slam_state = node.create_client(GetState, '/slam_toolbox/get_state')
        self._localization_active = node.create_client(
            Trigger, '/lifecycle_manager_localization/is_active')
        self._navigation_active = node.create_client(
            Trigger, '/lifecycle_manager_navigation/is_active')
        node.create_service(SetNavMode, '/nav_mode/set_mode', self._on_set_mode)
        node.create_service(ListMaps, '/nav_mode/list_maps', self._on_list_maps)
        node.create_timer(1.0, self._watch_children)

        if validate_request(LOCALIZATION, self._default_map, self._maps_dir) is None:
            with self._lock:
                self._switching = True
            self._start_worker(self._switch, LOCALIZATION, self._default_map)
        else:
            default_yaml = Path(self._maps_dir) / f'{self._default_map}.yaml'
            self._publish(IDLE, '', NavModeStatus.READY,
                          f'No default map ({default_yaml}); choose a mode')

    def _publish(self, mode, map_name, state, message):
        if state == NavModeStatus.FAILED:
            self._node.get_logger().error(message)
        else:
            self._node.get_logger().info(message)
        self._status_pub.publish(
            NavModeStatus(mode=mode, map_name=map_name, state=state, message=message))

    def _on_goal_status(self, topic, msg):
        with self._lock:
            self._goal_status[topic] = msg

    def _on_list_maps(self, request, response):
        response.map_names = list_maps(self._maps_dir)
        return response

    def _on_set_mode(self, request, response):
        mode = request.mode.strip().lower()
        map_name = request.map_name.strip() if mode == LOCALIZATION else ''
        error = validate_request(mode, map_name, self._maps_dir)
        with self._lock:
            if error is None and self._switching:
                error = 'A mode switch is already in progress'
            if error is None and goal_active(self._goal_status.values()):
                error = ACTIVE_GOAL_MESSAGE
            if error is None:
                self._switching = True
                self._waiting_pose = False
        if error is not None:
            self._node.get_logger().warning(f'Refused mode request: {error}')
            response.success = False
            response.message = error
            return response
        self._start_worker(self._switch, mode, map_name)
        response.success = True
        response.message = f'Switching to {describe(mode, map_name)}'
        return response

    def _on_amcl_pose(self, msg):
        with self._lock:
            if (not self._waiting_pose or self._switching
                    or not pose_is_fresh(msg.header.stamp, self._pose_since)):
                return
            self._waiting_pose = False
            self._switching = True
            map_name = self._map_name
        self._start_worker(self._start_navigation, map_name)

    def _start_worker(self, target, *args):
        self._worker = threading.Thread(target=target, args=args, daemon=True)
        self._worker.start()

    def _switch(self, mode, map_name):
        try:
            self._publish(mode, map_name, NavModeStatus.SWITCHING,
                          f'Switching to {describe(mode, map_name)}')
            self._stop_children()
            if mode == IDLE:
                self._finish(IDLE, '', NavModeStatus.READY,
                             'Idle: no mapping, localization or navigation running')
                return
            command = [
                'ros2', 'launch', 'nav2', 'go2_navigation.launch.py',
                f'mode:={mode}', f'maps_dir:={self._maps_dir}', 'use_sim_time:=False',
            ]
            if mode == MAPPING:
                self._run_step(
                    'mapping launch', command, self._slam_active, MAPPING, '',
                    NavModeStatus.READY, 'Mapping ready (new map)')
                return
            self._pose_since = self._node.get_clock().now().to_msg()
            # Nav2 starts only after the initial pose; its global costmap needs map -> odom.
            self._run_step(
                'localization launch',
                command + [f'map_name:={map_name}', 'start_navigation:=false'],
                lambda: self._is_active(self._localization_active), LOCALIZATION, map_name,
                NavModeStatus.WAITING_FOR_POSE,
                f'Map "{map_name}" loaded - set the initial pose with 2D Pose Estimate')
        except Exception as exc:  # noqa: B902 - never leave a half-started mode
            self._fail(f'Mode switch failed: {exc}')

    def _start_navigation(self, map_name):
        try:
            self._publish(LOCALIZATION, map_name, NavModeStatus.SWITCHING,
                          f'Initial pose received; starting Nav2 on map "{map_name}"')
            self._run_step(
                'navigation launch',
                ['ros2', 'launch', 'nav2', 'nav2.launch.py',
                 'use_sim_time:=False', 'static_map:=true'],
                lambda: self._is_active(self._navigation_active), LOCALIZATION, map_name,
                NavModeStatus.READY, f'Localization and Nav2 ready on map "{map_name}"')
        except Exception as exc:  # noqa: B902 - never leave a half-started mode
            self._fail(f'Starting Nav2 failed: {exc}')

    def _run_step(self, name, command, ready_check, mode, map_name, state, message):
        with self._lock:
            if self._stopping.is_set():
                return
            # Own process group so the whole launch tree can be signalled at once.
            self._children.append((name, subprocess.Popen(command, start_new_session=True)))
            self._mode, self._map_name = mode, map_name
        error = self._wait_until(ready_check)
        if self._stopping.is_set():
            return
        if error is None:
            self._finish(mode, map_name, state, message)
        else:
            self._fail(f'{describe(mode, map_name).capitalize()} failed: {error}')

    def _finish(self, mode, map_name, state, message):
        # Publish under the lock so a pose-triggered step cannot overtake this status.
        with self._lock:
            self._mode, self._map_name = mode, map_name
            self._waiting_pose = state == NavModeStatus.WAITING_FOR_POSE
            self._switching = False
            self._publish(mode, map_name, state, message)

    def _fail(self, reason):
        self._stop_children()
        if not self._stopping.is_set():
            self._finish(IDLE, '', NavModeStatus.FAILED, f'{reason}; now idle')

    def _stop_children(self):
        with self._lock:
            children, self._children = self._children, []
            self._mode, self._map_name = IDLE, ''
            self._waiting_pose = False
        for _, process in reversed(children):
            stop_process_group(process)
        with self._lock:
            # The stopped action servers no longer have goals.
            self._goal_status.clear()

    def _exited_child(self):
        with self._lock:
            children = list(self._children)
        return first_exit(children)

    def _wait_until(self, ready_check):
        deadline = time.monotonic() + self._ready_timeout
        while time.monotonic() < deadline:
            if self._stopping.is_set():
                return 'manager shutting down'
            exited = self._exited_child()
            if exited is not None:
                return exited
            if ready_check():
                return None
            time.sleep(1.0)
        return f'not ready within {self._ready_timeout:.0f} s'

    def _slam_active(self):
        state = self._call(self._slam_state, GetState.Request())
        return state is not None and state.current_state.id == State.PRIMARY_STATE_ACTIVE

    def _is_active(self, client):
        return getattr(self._call(client, Trigger.Request()), 'success', False)

    def _call(self, client, request, timeout=2.0):
        if not client.service_is_ready():
            return None
        future = client.call_async(request)
        deadline = time.monotonic() + timeout
        while not future.done() and time.monotonic() < deadline:
            time.sleep(0.05)
        if not future.done():
            future.cancel()
            return None
        return future.result()

    def _watch_children(self):
        with self._lock:
            if self._switching:
                return
            exited = first_exit(self._children)
            if exited is None:
                return
            self._switching = True
            self._waiting_pose = False
            reason = (f'{describe(self._mode, self._map_name).capitalize()}: '
                      f'{exited} unexpectedly')
        self._start_worker(self._fail, reason)

    def request_stop(self):
        """Flag shutdown; return True only for the first request."""
        first = not self._stopping.is_set()
        self._stopping.set()
        return first

    def shutdown(self):
        self._stopping.set()
        if self._worker is not None:
            self._worker.join()
        self._stop_children()


def main():
    # Handle SIGINT/SIGTERM here so the context stays valid while the child stops.
    rclpy.init(signal_handler_options=SignalHandlerOptions.NO)
    node = rclpy.create_node('nav_mode_manager')
    manager = NavModeManager(node)

    def on_signal(signum, frame):
        if manager.request_stop():
            raise KeyboardInterrupt

    signal.signal(signal.SIGINT, on_signal)
    signal.signal(signal.SIGTERM, on_signal)
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        manager.shutdown()
        node.destroy_node()
        rclpy.try_shutdown()


if __name__ == '__main__':
    main()
