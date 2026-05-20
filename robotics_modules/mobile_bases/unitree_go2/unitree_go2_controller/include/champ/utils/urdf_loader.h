#ifndef URDF_LOADER_H
#define URDF_LOADER_H

#include <rclcpp/rclcpp.hpp>
#include <urdf/model.h>

#include <champ/quadruped_base/quadruped_base.h>
#include <champ/quadruped_base/quadruped_leg.h>
#include <champ/quadruped_base/quadruped_joint.h>

namespace champ
{
    namespace URDF
    {
        void getPose(urdf::Pose *pose, std::string ref_link, std::string end_link, urdf::Model &model)
        {
            urdf::LinkConstSharedPtr ref_link_ptr = model.getLink(ref_link);

            std::string current_parent_name = end_link;
            urdf::LinkConstSharedPtr prev_link = model.getLink(current_parent_name);

            while(ref_link_ptr->name != current_parent_name)
            {   
                urdf::LinkConstSharedPtr current_link = model.getLink(current_parent_name);
                urdf::Pose current_pose = current_link->parent_joint->parent_to_joint_origin_transform;
              
                current_parent_name = current_link->getParent()->name;
                prev_link = model.getLink(current_parent_name);
                pose->position.x += current_pose.position.x;
                pose->position.y += current_pose.position.y;
                pose->position.z += current_pose.position.z;
            }
        }

        void fillLeg(champ::QuadrupedLeg *leg, const rclcpp::node_interfaces::NodeParametersInterface::SharedPtr nh, urdf::Model &model, std::string links_map)
        {
            rclcpp::Parameter links_param_("links_param", std::vector<std::string> ({}));
            auto success = nh->get_parameter(links_map, links_param_);
            if (!success){
                throw std::runtime_error("No links config file provided");
            }

            std::vector<std::string> links_param = links_param_.as_string_array();
            
            RCLCPP_INFO(rclcpp::get_logger("urdf_loader"), "fillLeg for %s", links_map.c_str());
            RCLCPP_INFO(rclcpp::get_logger("urdf_loader"), "Root link: %s", model.getRoot()->name.c_str());

            for (int i = 2; i > -1; i--){
                std::string ref_link, end_link;
                if (i>0){
                    ref_link = links_param[i-1];
                }else {
                    ref_link = model.getRoot()->name;
                }

                end_link = links_param[i];
                
                RCLCPP_INFO(rclcpp::get_logger("urdf_loader"), "  Joint %d: ref=%s, end=%s", 
                            i, ref_link.c_str(), end_link.c_str());
                
                urdf::Pose pose;
                getPose(&pose, ref_link, end_link, model);
                double x,y,z;
                x = pose.position.x;
                y = pose.position.y;
                z = pose.position.z;
                
                RCLCPP_INFO(rclcpp::get_logger("urdf_loader"), "    Pose: x=%.4f, y=%.4f, z=%.4f", x, y, z);
                
                leg->joint_chain[i]->setTranslation(x,y,z);
                
                // ADD VERIFICATION IMMEDIATELY AFTER setTranslation:
                float check_x = leg->joint_chain[i]->x();
                float check_y = leg->joint_chain[i]->y();
                float check_z = leg->joint_chain[i]->z();
                RCLCPP_INFO(rclcpp::get_logger("urdf_loader"), 
                            "    After setTranslation - stored: x=%.4f, y=%.4f, z=%.4f", 
                            check_x, check_y, check_z);
            }
        }

        void loadFromFile(champ::QuadrupedBase &base, const rclcpp::node_interfaces::NodeParametersInterface::SharedPtr nh, const std::string& urdf_filepath)
        {
            urdf::Model model;
            // TODO fix temp path
            if (!model.initFile(urdf_filepath)){
                 RCLCPP_ERROR(rclcpp::get_logger("rclcpp"), "Failed to parse urdf file");
            } 
            
            RCLCPP_INFO(rclcpp::get_logger("rclcpp"), "Successfully parsed urdf file");
            std::vector<std::string> links_map;

            links_map.push_back("links_map.left_front");
            links_map.push_back("links_map.right_front");
            links_map.push_back("links_map.left_hind");
            links_map.push_back("links_map.right_hind");
            
            for(int i = 0; i < 4; i++)
            {
                fillLeg(base.legs[i], nh, model, links_map[i]);
            }
        }

        void loadFromString(champ::QuadrupedBase &base, const rclcpp::node_interfaces::NodeParametersInterface::SharedPtr nh, const std::string& urdf_string)
        {
            urdf::Model model;
            // TODO fix temp path
            if (!model.initString(urdf_string)){
                 RCLCPP_ERROR(rclcpp::get_logger("rclcpp"), "Failed to parse urdf string");
            } 
            
            RCLCPP_INFO(rclcpp::get_logger("rclcpp"), "Successfully parsed urdf file");
            std::vector<std::string> links_map;

            links_map.push_back("links_map.left_front");
            links_map.push_back("links_map.right_front");
            links_map.push_back("links_map.left_hind");
            links_map.push_back("links_map.right_hind");
            
            for(int i = 0; i < 4; i++)
            {
                fillLeg(base.legs[i], nh, model, links_map[i]);
            }
        }

        std::vector<std::string> getJointNames(const rclcpp::node_interfaces::NodeParametersInterface::SharedPtr nh)
        {
            std::vector<std::string> joints_map;
            std::vector<std::string> joint_names;

            joints_map.push_back("joints_map.left_front");
            joints_map.push_back("joints_map.right_front");
            joints_map.push_back("joints_map.left_hind");
            joints_map.push_back("joints_map.right_hind");

           
            for(int i = 0; i < 4; i++)
            {
                rclcpp::Parameter joints_param_("joints_param", std::vector<std::string> ({}));
                auto success = nh->get_parameter(joints_map[i], joints_param_);
                if (!success){
                    throw std::runtime_error("No joints config file provided");
                }


                std::vector<std::string> joints_param = joints_param_.as_string_array();
                for (int j=0;j<3;j++){
                    joint_names.push_back(joints_param[j]);
                }
            }

            return joint_names;
        }

        std::vector<std::string> getLinkNames(const rclcpp::node_interfaces::NodeParametersInterface::SharedPtr nh)
        {
            std::vector<std::string> links_map;
            std::vector<std::string> links_names;

            links_map.push_back("links_map.left_front");
            links_map.push_back("links_map.right_front");
            links_map.push_back("links_map.left_hind");
            links_map.push_back("links_map.right_hind");

            for(int i = 0; i < 4; i++)
            {
                rclcpp::Parameter links_param_("links_param", std::vector<std::string> ({}));
                auto success = nh->get_parameter(links_map[i], links_param_);
                if (!success){
                    throw std::runtime_error("No links config file provided");
                }


                std::vector<std::string> link_param = links_param_.as_string_array();
                for (int j=0;j<4;j++){
                    links_names.push_back(link_param[j]);
                }
            }

            return links_names;
        }
    }
}

#endif
