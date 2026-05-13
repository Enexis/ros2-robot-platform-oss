#include "quadruped_controller.h"

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<QuadrupedController>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
