#include <vector>
#include <geographic_msgs/msg/geo_pose.hpp>
#include <nlohmann/json.hpp>
#include <array>
using json = nlohmann::json;
class GeoRouteParser
{
public:
    static std::vector<geographic_msgs::msg::GeoPose> parse(const char *path);

private:
    static json load_json(const char *path);
    static void parse_path(const json &data,
               std::vector<std::array<double, 3>> *coordinates);
    static void add_heading(std::vector<geographic_msgs::msg::GeoPose> &poses);
    static double compute_bearing(double lat1_deg, double lon1_deg,
                    double lat2_deg, double lon2_deg);
    static void convert_to_geopose(const std::vector<std::array<double, 3>> &coordinates, std::vector<geographic_msgs::msg::GeoPose> &poses);
};
