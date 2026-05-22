import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.substitutions import LaunchConfiguration
from launch.actions import IncludeLaunchDescription, DeclareLaunchArgument
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.conditions import IfCondition
from nav2_common.launch import RewrittenYaml


def generate_launch_description():
    nav2_params = os.path.join(
        get_package_share_directory( "nav2"),
        "config",
        "nav2_config.yaml"
    )
    # configured_params = RewrittenYaml(
    #     source_file=nav2_params, root_key="", param_rewrites="", convert_types=True
    # )

    navigation2_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(
                get_package_share_directory('nav2_bringup'),
                "launch",
                "navigation_launch.py"
            )
        ),
        launch_arguments={
            "use_sim_time": "True",
            "params_file": nav2_params,
            "autostart": "True",
        }.items(),
    )

    return LaunchDescription([navigation2_cmd])