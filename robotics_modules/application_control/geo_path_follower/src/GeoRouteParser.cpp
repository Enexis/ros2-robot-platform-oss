#include <fstream>
#include <stdlib.h>
#include <vector>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include <geographic_msgs/msg/geo_pose.hpp>
#include <tf2/LinearMath/Quaternion.h>
#include <array>
#include <cmath>
#include "GeoRouteParser.hpp"

static double deg2rad(double deg) { return deg * M_PI / 180.0; }

// Compute bearing from A → B (ENU heading)
double GeoRouteParser::compute_bearing(double lat1_deg, double lon1_deg,
                                       double lat2_deg, double lon2_deg)
{
    double lat1 = deg2rad(lat1_deg);
    double lat2 = deg2rad(lat2_deg);
    double dlon = deg2rad(lon2_deg - lon1_deg);

    double y = std::sin(dlon) * std::cos(lat2);
    double x = std::cos(lat1) * std::sin(lat2) -
               std::sin(lat1) * std::cos(lat2) * std::cos(dlon);

    double bearing = std::atan2(y, x); // radians
    return bearing;                    // ENU yaw
}

void GeoRouteParser::convert_to_geopose(const std::vector<std::array<double, 3>> &coordinates, std::vector<geographic_msgs::msg::GeoPose> &poses)
{
    for (const auto &coordinate : coordinates)
    {
        geographic_msgs::msg::GeoPose pose;
        pose.position.longitude = coordinate[0];
        pose.position.latitude = coordinate[1];
        pose.position.altitude = coordinate[2];
        poses.push_back(pose);
    }
}

void GeoRouteParser::add_heading(std::vector<geographic_msgs::msg::GeoPose> &poses)
{
    if (poses.size() < 2)
        return;
    tf2::Quaternion q;
    for (size_t i = 1; i < poses.size(); i++)
    {
        // Orientation: yaw toward target
        double yaw = compute_bearing(poses.at(i - 1).position.latitude, poses.at(i - 1).position.longitude, poses.at(i).position.latitude, poses.at(i).position.longitude);

        q.setRPY(0.0, 0.0, yaw);
        poses[i - 1].orientation.x = q.x();
        poses[i - 1].orientation.y = q.y();
        poses[i - 1].orientation.z = q.z();
        poses[i - 1].orientation.w = q.w();
    }
}

json GeoRouteParser::load_json(const char *path)
{
    if (path[0] == '\0')
    {
        throw std::runtime_error(std::string("Filepath empty"));
    }
    
    std::ifstream f(path);
    if (!f.is_open())
    {
        throw std::runtime_error(std::string("Could not open file: ") + path);
    }
    try
    {
        return json::parse(f);
    }
    catch (const json::parse_error &e)
    {
        throw std::runtime_error(std::string("JSON parse error in ") + path + ": " + e.what());
    }
}

void GeoRouteParser::parse_path(const json &data,
                                std::vector<std::array<double, 3>> *coordinates)
{
    if (!data.contains("features") || !data["features"].is_array())
    {
        throw std::runtime_error("GeoJSON missing 'features' array");
    }

    for (const auto &feature : data["features"])
    {
        // Validate geometry
        if (!feature.contains("geometry") ||
            !feature["geometry"].contains("type") ||
            !feature["geometry"].contains("coordinates"))
        {
            continue;
        }

        std::string type = feature["geometry"]["type"].get<std::string>();
        const auto &coords = feature["geometry"]["coordinates"];
        if (type != "LineString" ||
            !coords.is_array())
        {
            continue;
        }

        coordinates->clear();
        coordinates->reserve(coords.size());

        for (const auto &pt : coords)
        {
            if (!pt.is_array() || pt.size() < 2)
            {
                throw std::runtime_error("Invalid coordinates");
            }

            std::array<double, 3> position = {pt[0].get<double>(), pt[1].get<double>(), 0};
            if (pt.size() == 3)
            {
                position[2] = pt[2].get<double>();
            }
            coordinates->emplace_back(position);
        }
        return; // done after parsing "Path"
    }

    throw std::runtime_error("No feature with type LineString found");
}

std::vector<geographic_msgs::msg::GeoPose> GeoRouteParser::parse(const char *path)
{
    std::vector<std::array<double, 3>> coordinates;
    std::vector<geographic_msgs::msg::GeoPose> poses;
    json data = load_json(path);
    parse_path(data, &coordinates);
    convert_to_geopose(coordinates, poses);
    add_heading(poses);
    return poses;
}