import rclpy
from rclpy.node import Node
from rclpy.action import ActionClient

from geometry_msgs.msg import PoseStamped
from nav_msgs.msg import Odometry
from nav2_msgs.action import NavigateToPose, NavigateThroughPoses
import math




class SmartNavigator(Node):
    def __init__(self):
        super().__init__('smart_navigator')

        self.navigate_client = ActionClient(
            self, NavigateToPose, 'navigate_to_pose'
        )

        self.through_client = ActionClient(
            self, NavigateThroughPoses, 'navigate_through_poses'
        )

        self.robot_pose = None
        self.sub = self.create_subscription(
            Odometry, 'odometry/global', self.odom_cb, 10
        )

        self.state = 'INIT'
        
        self.waypoints = [
            self.make_pose(2.0, 0.0),
            self.make_pose(5.0, 0.0),
            self.make_pose(8.0, 1.0),
            self.make_pose(10.0, 1.5),
        ]
        self.active_goal = self.waypoints[0]

        self.timer = self.create_timer(0.1, self.step)

    def make_pose(self, x, y):
        p = PoseStamped()
        p.header.frame_id = 'map'
        p.pose.position.x = x
        p.pose.position.y = y
        p.pose.orientation.w = 1.0
        return p

    def odom_cb(self, msg):
        self.robot_pose = msg.pose.pose.position

    def distance_to(self, pose):
        return math.hypot(
            pose.pose.position.x - self.robot_pose.x,
            pose.pose.position.y - self.robot_pose.y
        )

    def step(self):
        if self.robot_pose is None:
            return

        if self.state == 'INIT':
            self.state = 'NAV_TO_POSE'
            self.send_nav_to_pose(self.active_goal)
            self.get_logger().info("Navigating to initial pose")

        elif self.state == 'NAV_TO_POSE':      
            if self.distance_to(self.active_goal) <= 0.20:
                self.get_logger().info("Reached pose")
                self.cancel_nav_to_pose()
                if len(self.waypoints) >= 2:
                    self.get_logger().info("switching to waypoint navigation")
                    self.state = 'WAYPOINTS'
                else:
                    self.get_logger().info("No waypoints left, navigation complete")
                    self.state = 'COMPLETE'

        elif self.state == 'WAYPOINTS':
            if self.distance_to(self.active_goal) <= 0.20:
                self.get_logger().info("Reached waypoint")
                self.waypoints.pop(0)
                if len(self.waypoints) >= 2:
                    self.get_logger().info("continuing waypoint navigation")
                    self.send_waypoints([self.waypoints[0], self.waypoints[1]])
                    self.active_goal = self.waypoints[1]
                    self.state = 'WAYPOINTS'

                elif len(self.waypoints) >= 1:
                    self.active_goal = self.waypoints[0]
                    self.get_logger().info("Next waypoint is now active goal")
                else:
                    self.get_logger().info("No waypoints left, navigation complete")
                    self.state = 'COMPLETE'
                    navigator.destroy_node()
                    rclpy.shutdown()

    def send_nav_to_pose(self, goal_pose):
        goal = NavigateToPose.Goal()
        goal.pose = goal_pose
        self.navigate_client.wait_for_server()
        self.nav_future = self.navigate_client.send_goal_async(goal)

    def cancel_nav_to_pose(self):
        if hasattr(self, 'nav_future') and self.nav_future.done():
            handle = self.nav_future.result()
            handle.cancel_goal_async()

    def send_waypoints(self, waypoints):
        goal = NavigateThroughPoses.Goal()
        goal.poses = waypoints
        self.through_client.wait_for_server()
        self.through_client.send_goal_async(goal)

if __name__ == '__main__':
    rclpy.init()
    navigator = SmartNavigator()
    rclpy.spin(navigator)
    