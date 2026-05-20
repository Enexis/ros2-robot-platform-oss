from launch import LaunchDescription, LaunchContext
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from launch.actions import OpaqueFunction

import xacro
import yaml
def launch_setup(context: LaunchContext) -> list:
    """The launch setup.

    Args:
        context (LaunchContext): The launch context.

    Returns:
        list: The actions to start.
    """
    # --- Retrieve evaluated arguments ---
    xacro_file = LaunchConfiguration("xacro_file").perform(context)
    xacro_args = yaml.safe_load(LaunchConfiguration("xacro_arguments").perform(context))
    xacro_args['namespace'] = LaunchConfiguration("namespace").perform(context)
    # xacro_args['ros_control_config_file'] = "/alliander/ros/install/go2_description/share/go2_description/config/ros_control.yaml"
    namespace = LaunchConfiguration("namespace")
    use_sim_time = LaunchConfiguration("use_sim_time")
    
    # --- Process xacro ---
    robot_description = xacro.process_file(xacro_file, mappings=xacro_args).toxml()
  
    robot_state_publisher = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        name='robot_state_publisher',
        namespace=namespace,
        output='screen',
        parameters=[{
            'use_sim_time': use_sim_time,
            'robot_description': robot_description,
            'publish_frequency': 10.0,
            'frame_prefix': '',
        }],
    )
    return [robot_state_publisher]


def generate_launch_description() -> LaunchDescription:
    """Generate the launch description.

    Returns:
        LaunchDescription: The launch description.
    """
    declared_arguments = []

    declared_arguments.append(
        DeclareLaunchArgument(
            'xacro_file',
            default_value=PathJoinSubstitution([
                FindPackageShare('go2_description'),
                'xacro',
                'unitree_go2.xacro']),
            description='xacro file to be added to the robot description.',
        )
    )

    declared_arguments.append(
        DeclareLaunchArgument(
            'xacro_arguments',
            default_value='{}',
            description='xacro arguments to be added to the robot description.',
        )
    )

    declared_arguments.append(
        DeclareLaunchArgument(
            'namespace',
            default_value='',
            description='namespace to be added to the robot description.',
        )
    )

    declared_arguments.append(
        DeclareLaunchArgument(
            'use_sim_time',
            default_value='True',
            description='Use simulation/Gazebo clock if true',
        )
    )

    return LaunchDescription(declared_arguments +[OpaqueFunction(function=launch_setup)])

