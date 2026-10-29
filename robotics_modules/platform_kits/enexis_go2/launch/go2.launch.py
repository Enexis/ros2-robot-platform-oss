from launch import LaunchDescription, LaunchContext
from launch.actions import (
    DeclareLaunchArgument,
    IncludeLaunchDescription,
    OpaqueFunction,
)
from launch.conditions import IfCondition
from launch.substitutions import (
    LaunchConfiguration,
    PythonExpression,
    PathJoinSubstitution,
)
from launch_ros.actions import Node
from launch.launch_description_sources import PythonLaunchDescriptionSource
from ament_index_python.packages import get_package_share_directory

import os


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
            "xacro_arguments": LaunchConfiguration("xacro_arguments"),
            "rviz": "False",
        }.items(),
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

    livox_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(
                get_package_share_directory("livox"), "launch", "livox_mid360s.launch.py"
            )
        ),
        launch_arguments={
            "frame_id": "lidar_link",
        }.items(),
        condition=IfCondition(LaunchConfiguration("livox").perform(context)),
    )

    pc_to_scan_node = Node(
        package="pointcloud_to_laserscan",
        executable="pointcloud_to_laserscan_node",
        name="livox_pc2l_scan",
        output="screen",
        parameters=[{
            "target_frame": "lidar_link",
            "transform_tolerance": 0.01,
            "min_height": -0.1,
            "max_height": 0.2,
            "angle_min": -3.14159,
            "angle_max": 3.14159,
            "scan_time": 0.05,
            "range_min": 0.1,
            "range_max": 30.0,
        }],
        remappings=[("cloud_in", "livox/lidar"), ("scan", "scan")],
        condition=IfCondition(LaunchConfiguration("livox").perform(context)),
    )

    return [robot_description_cmd, rviz_cmd, livox_cmd, pc_to_scan_node]


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

    declared_arguments.append(
        DeclareLaunchArgument(
            "livox",
            default_value="False",
            description="Launch Livox MID360s driver.",
        )
    )

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
