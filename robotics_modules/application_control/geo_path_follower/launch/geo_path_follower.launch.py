import os

from launch import LaunchContext, LaunchDescription
from launch.actions import OpaqueFunction, DeclareLaunchArgument
from launch.conditions import IfCondition
from launch_ros.actions import Node, SetParameter
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.substitutions import FindPackageShare
from ament_index_python.packages import get_package_share_directory


def launch_setup(context: LaunchContext) -> list:
    """The launch setup.

    Args:
        context (LaunchContext): The launch context.

    Returns:
        list: The actions to start.
    """
    geo_route_follower = Node(
        package="geo_path_follower",
        executable="GeoRouteFollower",
        name="geo_route_follower",
        parameters=[
            {"geoJson_file_path": LaunchConfiguration("geoJson_file_path")},
        ]
    )


    return [geo_route_follower]


def generate_launch_description() -> LaunchDescription:
    """Generate the launch description.

    Returns:
        LaunchDescription: The launch description.
    """

    declare_geJson_cmd = DeclareLaunchArgument(
        "geoJson_file_path",
        default_value="",
        description="path to geoJson file containing the path"
    )

    return LaunchDescription(
        [
            declare_geJson_cmd,
            OpaqueFunction(function=launch_setup),
        ]
    )
