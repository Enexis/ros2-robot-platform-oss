# flake8: noqa: Q000

from pathlib import Path
import re

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
    mode = LaunchConfiguration("mode").perform(context)
    map_name = LaunchConfiguration("map_name").perform(context)
    maps_dir = LaunchConfiguration("maps_dir").perform(context)
    use_sim_time = LaunchConfiguration("use_sim_time").perform(context).lower()

    if mode not in ("mapping", "localization"):
        raise RuntimeError("mode must be explicitly set to mapping or localization")
    if use_sim_time not in ("true", "false"):
        raise RuntimeError("use_sim_time must be true or false")
    if use_sim_time != "false":
        raise RuntimeError("physical Go2 mapping and localization require use_sim_time:=False")

    nav2_share = Path(get_package_share_directory("nav2"))

    if mode == "localization":
        if not re.fullmatch(r"[A-Za-z0-9_-]+", map_name):
            raise RuntimeError(
                "localization requires map_name using ASCII letters, digits, _ or -"
            )
        map_yaml = Path(maps_dir) / f"{map_name}.yaml"
        if not map_yaml.is_file():
            raise RuntimeError(
                f"Map YAML does not exist: {map_yaml}; "
                "choose an existing map or start mapping mode and save it first."
            )

    if mode == "mapping":
        return [
            IncludeLaunchDescription(
                PythonLaunchDescriptionSource(
                    str(
                        Path(get_package_share_directory("slam_toolbox"))
                        / "launch"
                        / "online_async_launch.py"
                    )
                ),
                launch_arguments={
                    "slam_params_file": str(
                        nav2_share / "config" / "slam_toolbox_mapping.yaml"
                    ),
                    "use_sim_time": "False",
                    "autostart": "true",
                    "use_lifecycle_manager": "false",
                }.items(),
            )
        ]

    nav2_params = str(nav2_share / "config" / "nav2_config.yaml")
    localization_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            str(
                Path(get_package_share_directory("nav2_bringup"))
                / "launch"
                / "localization_launch.py"
            )
        ),
        launch_arguments={
            "map": str(map_yaml),
            "params_file": RewrittenYaml(
                source_file=nav2_params,
                root_key="",
                param_rewrites={
                    "map_server.ros__parameters.yaml_filename": str(map_yaml),
                },
                convert_types=True,
            ),
            "use_sim_time": "False",
            "autostart": "True",
        }.items(),
    )
    navigation_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(str(nav2_share / "launch" / "nav2.launch.py")),
        launch_arguments={
            "use_sim_time": "False",
            "static_map": "true",
        }.items(),
    )
    return [localization_launch, navigation_launch]


def generate_launch_description():
    return LaunchDescription(
        [
            DeclareLaunchArgument(
                "mode",
                default_value="",
                description="Required physical mode: mapping or localization.",
            ),
            DeclareLaunchArgument(
                "map_name",
                default_value="",
                description="Saved map basename required only for localization.",
            ),
            DeclareLaunchArgument(
                "maps_dir",
                default_value="/maps",
                description="Host-persisted map directory mounted into the container.",
            ),
            DeclareLaunchArgument(
                "use_sim_time",
                default_value="False",
                description="Physical Go2 modes require wall-clock time.",
            ),
            OpaqueFunction(function=launch_setup),
        ]
    )
