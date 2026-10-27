from launch import LaunchDescription, LaunchContext
from launch.actions import (
    DeclareLaunchArgument,
    IncludeLaunchDescription,
    OpaqueFunction,
)
from launch.conditions import IfCondition, UnlessCondition
from launch.substitutions import (
    LaunchConfiguration,
    PythonExpression,
    PathJoinSubstitution,
)
from launch_ros.actions import Node
from launch.launch_description_sources import PythonLaunchDescriptionSource
from ament_index_python.packages import get_package_share_directory

import os
import yaml

def launch_setup(context: LaunchContext) -> list:
    """The launch setup.

    Args:
        context (LaunchContext): The launch context.

    Returns:
        list: The actions to start.
    """

    xacro_file = os.path.join(
        get_package_share_directory("enexis_go2"), "xacro", "enexis_go2.xacro"
    )

    # On hardware, imu_tilt_tf publishes base_footprint -> base_link from the IMU.
    hardware = LaunchConfiguration("use_sim_time").perform(context).lower() not in ("true", "1")
    xacro_args = yaml.safe_load(LaunchConfiguration("xacro_arguments").perform(context)) or {}
    if hardware:
        xacro_args.setdefault("imu_tilt", "true")

    robot_description_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(
                get_package_share_directory("go2_bringup"), "launch", "go2.launch.py"
            )
        ),
        launch_arguments={
            "namespace": LaunchConfiguration("namespace"),
            "use_sim_time": LaunchConfiguration("use_sim_time"),
            "xacro_file": xacro_file,
            "xacro_arguments": yaml.safe_dump(xacro_args, default_flow_style=True).strip(),
            "rviz": "False",
        }.items(),
    )

    imu_tilt_node = Node(
        package="enexis_go2",
        executable="imu_tilt_tf.py",
        name="imu_tilt_tf",
        output="screen",
        ros_arguments=["--log-level", "rmw_cyclonedds_cpp:=error"],
        condition=IfCondition(str(hardware)),
    )

    rviz_cmd = Node(
        package="rviz2",
        executable="rviz2",
        name="rviz2",
        output="screen",
        arguments=[
            "-d",
            os.path.join(
                get_package_share_directory("enexis_go2"), "config", "rviz.rviz"
            ),
        ],
        parameters=[{"use_sim_time": LaunchConfiguration("use_sim_time")}],
        condition=IfCondition(LaunchConfiguration("rviz").perform(context)),
    )

    pc_to_scan_node = Node(
        package="pointcloud_to_laserscan",
        executable="pointcloud_to_laserscan_node",
        name="livox_pc2l_scan",
        output="screen",
        ros_arguments=["--log-level", "rmw_cyclonedds_cpp:=error"],
        parameters=[{
            "target_frame": "base_footprint",
            "transform_tolerance": 0.1,
            # base_footprint is level and at body height, ~0.30 m above the
            # ground when standing, so this keeps points 0.08-0.45 m above the
            # ground. Body tilt is in TF via imu_tilt_tf.
            "min_height": -0.22,
            "max_height": 0.15,
            "angle_min": -3.14159,
            "angle_max": 3.14159,
            "scan_time": 0.05,
            "range_min": 0.3,
            "range_max": 30.0,
        }],
        remappings=[("cloud_in", "livox/lidar"), ("scan", "scan_unfiltered")],
        condition=UnlessCondition(LaunchConfiguration("use_sim_time")),
    )

    # Removes returns from the lidar protection mount (within ~7 cm of the
    # lidar) so they do not show up as obstacles in the costmaps.
    scan_filter_node = Node(
        package="laser_filters",
        executable="scan_to_scan_filter_chain",
        name="livox_scan_filter",
        output="screen",
        ros_arguments=["--log-level", "rmw_cyclonedds_cpp:=error"],
        parameters=[{
            "filter1": {
                "name": "lidar_mount_box",
                "type": "laser_filters/LaserScanBoxFilter",
                "params": {
                    # /scan lies in the base_footprint plane, 0.12 m
                    # below the lidar, so the box is tall in z. The ~18.5 deg
                    # lidar pitch shifts those points ~4 cm forward in
                    # lidar_link. The mount shows up at about +-45 deg and up to
                    # 16 cm from the lidar, so the box covers ~16 cm ahead and
                    # 15 cm to each side.
                    "box_frame": "lidar_link",
                    "min_x": -0.06,
                    "max_x": 0.19,
                    "min_y": -0.15,
                    "max_y": 0.15,
                    "min_z": -0.5,
                    "max_z": 0.5,
                    "invert": False,
                },
            },
        }],
        remappings=[("scan", "scan_unfiltered"), ("scan_filtered", "scan")],
        condition=UnlessCondition(LaunchConfiguration("use_sim_time")),
    )

    return [robot_description_cmd, imu_tilt_node, rviz_cmd, pc_to_scan_node, scan_filter_node]


def generate_launch_description() -> LaunchDescription:
    """Generate the launch description.

    Returns:
        LaunchDescription: The launch description.
    """
    declared_arguments = []

    declared_arguments.append(
        DeclareLaunchArgument(
            "xacro_arguments",
            default_value="{}",
            description="xacro arguments to be added to the robot description.",
        )
    )

    declared_arguments.append(
        DeclareLaunchArgument(
            "namespace",
            default_value="",
            description="Namespace for the robot.",
        )
    )

    declared_arguments.append(
        DeclareLaunchArgument(
            "use_sim_time",
            default_value="False",
            description="Use simulation time.",
        )
    )

    declared_arguments.append(
        DeclareLaunchArgument(
            "rviz",
            default_value="False",
            description="Launch rviz.",
        )
    )

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
