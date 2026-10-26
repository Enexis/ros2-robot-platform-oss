# flake8: noqa: Q000

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import (
    DeclareLaunchArgument,
    IncludeLaunchDescription,
    OpaqueFunction,
)
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from nav2_common.launch import RewrittenYaml


def launch_setup(context):
    nav2_params = os.path.join(
        get_package_share_directory("nav2"),
        "config",
        "nav2_config.yaml",
    )
    use_sim_time = LaunchConfiguration("use_sim_time")
    static_map = LaunchConfiguration("static_map").perform(context).lower()
    if static_map not in ("true", "false"):
        raise RuntimeError("static_map must be true or false")

    params_file = nav2_params
    if static_map == "true":
        params_file = RewrittenYaml(
            source_file=nav2_params,
            root_key="",
            param_rewrites={
                "global_costmap.global_costmap.ros__parameters.rolling_window": "false",
                "global_costmap.global_costmap.ros__parameters.static_layer.enabled": "true",
            },
            convert_types=True,
        )

    navigation2_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(
                get_package_share_directory("nav2_bringup"),
                "launch",
                "navigation_launch.py",
            )
        ),
        launch_arguments={
            "use_sim_time": use_sim_time,
            "params_file": params_file,
            "autostart": "True",
            # Hide Cyclone DDS warnings about Unitree endpoints without ROS type hashes.
            "log_level": "rmw_cyclonedds_cpp:=error",
        }.items(),
    )

    return [navigation2_cmd]


def generate_launch_description():
    return LaunchDescription([
        DeclareLaunchArgument(
            "use_sim_time",
            default_value="True",
            description="Use simulation clock if true.",
        ),
        DeclareLaunchArgument(
            "static_map",
            default_value="false",
            description="Enable the static map layer for physical map localization.",
        ),
        OpaqueFunction(function=launch_setup),
    ])