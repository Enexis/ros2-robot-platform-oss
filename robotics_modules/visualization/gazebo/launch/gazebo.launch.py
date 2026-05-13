import os

from launch import LaunchContext, LaunchDescription
from launch.actions import OpaqueFunction, DeclareLaunchArgument, ExecuteProcess
from launch_ros.actions import Node, SetParameter
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.substitutions import FindPackageShare
from ament_index_python.packages import get_package_share_directory
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource


def launch_setup(context: LaunchContext) -> list:
    """The launch setup.

    Args:
        context (LaunchContext): The launch context.

    Returns:
        list: The actions to start.
    """
    namespace = LaunchConfiguration('namespace')
    use_sim_time = LaunchConfiguration('use_sim_time')
    world = LaunchConfiguration('world')
    parameter_bridge_config = LaunchConfiguration('parameter_bridge_config')
    
    gz_gui_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            FindPackageShare('ros_gz_sim'),
            '/launch/gz_sim.launch.py'
        ]),
        launch_arguments={
            'gz_args': '-g',
        }.items(),
    )

    ros_gz_sim_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            FindPackageShare('ros_gz_sim'),
            '/launch/ros_gz_sim.launch.py'
        ]),
        launch_arguments={
            'bridge_name': 'parameter_bridge',
            'config_file': parameter_bridge_config,
            'use_sim_time': use_sim_time.perform(context),
            'world_sdf_file': f"{world.perform(context)}.sdf",
        }.items(),
    )

    gz_spawn_model_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            FindPackageShare('ros_gz_sim'),
            '/launch/gz_spawn_model.launch.py'
        ]),
        launch_arguments={
            'topic': f"{namespace.perform(context)}/robot_description",
            'world': world.perform(context),
            'x': LaunchConfiguration('spawn_x'),
            'y': LaunchConfiguration('spawn_y'),
            'z': LaunchConfiguration('spawn_z'),
        }.items(),
    )

    return [ros_gz_sim_launch, gz_gui_launch, gz_spawn_model_launch]



def generate_launch_description() -> LaunchDescription:
    """Generate the launch description.

    Returns:
        LaunchDescription: The launch description.
    """
    declare_world_cmd = DeclareLaunchArgument(
        'world',
        default_value='empty',
        description='World file to load in Gazebo'
    )
    declare_namespace_cmd = DeclareLaunchArgument(
        'namespace',
        default_value='',
        description='Namespace for the robot'
    )
    declare_sim_time_cmd = DeclareLaunchArgument(
        'use_sim_time',
        default_value='True',
        description='Use sim time for the robot'
    )

    declare_parameter_bridge_config_cmd = DeclareLaunchArgument(
        'parameter_bridge_config',
        default_value=os.path.join(
                get_package_share_directory('gazebo'),
                'config',
                'parameter_bridge.yaml'
            ),
        description='Path to the parameter bridge configuration file'
    )
    declare_spawn_x_cmd = DeclareLaunchArgument(
        'spawn_x',
        default_value='0.0',
        description='X position to spawn the robot'
    )
    declare_spawn_y_cmd = DeclareLaunchArgument(
        'spawn_y',
        default_value='0.0',
        description='Y position to spawn the robot'
    )
    declare_spawn_z_cmd = DeclareLaunchArgument(
        'spawn_z',
        default_value='0.5',
        description='Z position to spawn the robot'
    )


    return LaunchDescription(
        [
            declare_world_cmd,
            declare_namespace_cmd,
            declare_sim_time_cmd,
            declare_parameter_bridge_config_cmd,
            declare_spawn_x_cmd,
            declare_spawn_y_cmd,
            declare_spawn_z_cmd,
            OpaqueFunction(function=launch_setup),
        ]
    )
