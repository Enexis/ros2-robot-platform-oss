from nav2_simple_commander.robot_navigator import BasicNavigator
from geometry_msgs.msg import PoseStamped
import rclpy

def make_pose(x, y, yaw):
    pose = PoseStamped()
    pose.header.frame_id = 'map'
    pose.header.stamp = navigator.get_clock().now().to_msg()
    pose.pose.position.x = x
    pose.pose.position.y = y
    pose.pose.orientation.z = yaw
    pose.pose.orientation.w = 1.0
    return pose

rclpy.init()
navigator = BasicNavigator()

# Wait until Nav2 is active
navigator.waitUntilNav2Active(localizer='robot_localization')


poses = [
    make_pose(1.0, 0.0, 0.0),
    make_pose(2.0, -3.0, 0.0),
    make_pose(9.0, 0.0, 0.0),
]

navigator.followWaypoints(poses)
# navigator.goThroughPoses(poses)

while not navigator.isTaskComplete():
    active_goal = navigator.getGoalPose()
    print("Active Goal:", active_goal)

result = navigator.getResult()
print("Navigation result:", result)

# navigator.lifecycleShutdown()
rclpy.shutdown()
