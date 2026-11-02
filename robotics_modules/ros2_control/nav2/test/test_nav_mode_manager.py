"""Unit tests for the nav mode manager logic that needs no ROS graph."""

import importlib.util
from pathlib import Path
import subprocess

from action_msgs.msg import GoalStatus, GoalStatusArray
from builtin_interfaces.msg import Time

SCRIPT = Path(__file__).resolve().parents[1] / 'scripts' / 'nav_mode_manager.py'
spec = importlib.util.spec_from_file_location('nav_mode_manager', SCRIPT)
manager = importlib.util.module_from_spec(spec)
spec.loader.exec_module(manager)


def test_list_maps_returns_sorted_selectable_basenames(tmp_path):
    for name in ('office', 'Lab-2', 'a_b', 'with space', 'notes'):
        (tmp_path / f'{name}.yaml').write_text('image: x.pgm\n')
    (tmp_path / 'notes.yaml').unlink()
    (tmp_path / 'notes.pgm').write_text('')
    (tmp_path / 'graph.posegraph').write_text('')
    (tmp_path / 'dir.yaml').mkdir()
    assert manager.list_maps(tmp_path) == ['Lab-2', 'a_b', 'office']


def test_list_maps_missing_directory(tmp_path):
    assert manager.list_maps(tmp_path / 'missing') == []


def test_validate_request(tmp_path):
    (tmp_path / 'office.yaml').write_text('image: office.pgm\n')
    validate = manager.validate_request
    assert validate('idle', '', tmp_path) is None
    assert validate('mapping', '', tmp_path) is None
    assert validate('localization', 'office', tmp_path) is None
    assert 'Unknown mode' in validate('navigate', '', tmp_path)
    assert 'Unknown mode' in validate('', '', tmp_path)
    assert 'map_name' in validate('localization', '', tmp_path)
    assert 'map_name' in validate('localization', '../office', tmp_path)
    assert 'map_name' in validate('localization', 'office.yaml', tmp_path)
    assert 'Map not found' in validate('localization', 'warehouse', tmp_path)


def test_goal_active():
    def array(*states):
        return GoalStatusArray(status_list=[GoalStatus(status=state) for state in states])

    assert not manager.goal_active([])
    assert not manager.goal_active([array()])
    assert not manager.goal_active([array(
        GoalStatus.STATUS_SUCCEEDED, GoalStatus.STATUS_CANCELED, GoalStatus.STATUS_ABORTED,
    )])
    for state in (
        GoalStatus.STATUS_ACCEPTED, GoalStatus.STATUS_EXECUTING, GoalStatus.STATUS_CANCELING,
    ):
        assert manager.goal_active([array(), array(GoalStatus.STATUS_SUCCEEDED, state)])


def test_stop_process_group_stops_whole_tree():
    # The grandchild ignores SIGINT, so stopping must escalate to SIGTERM.
    process = subprocess.Popen(
        ['bash', '-c', "bash -c 'trap \"\" INT; sleep 60' & sleep 60; wait"],
        start_new_session=True,
    )
    manager.stop_process_group(process, interrupt_timeout=1.0)
    assert process.returncode is not None
    assert not manager._group_alive(process.pid)


def test_pose_is_fresh():
    since = Time(sec=100, nanosec=500)
    assert not manager.pose_is_fresh(Time(sec=100, nanosec=499), since)
    assert not manager.pose_is_fresh(Time(sec=99, nanosec=900), since)
    assert manager.pose_is_fresh(Time(sec=100, nanosec=500), since)
    assert manager.pose_is_fresh(Time(sec=101, nanosec=0), since)
    assert not manager.pose_is_fresh(Time(sec=101, nanosec=0), None)


def test_first_exit_reports_any_exited_child():
    running = subprocess.Popen(['sleep', '60'])
    finished = subprocess.Popen(['bash', '-c', 'exit 3'])
    finished.wait()
    try:
        assert manager.first_exit([('localization launch', running)]) is None
        assert manager.first_exit([
            ('localization launch', running), ('navigation launch', finished),
        ]) == 'navigation launch exited with code 3'
    finally:
        running.kill()
        running.wait()
