#ifndef NAV_MODE_PANEL_HPP_
#define NAV_MODE_PANEL_HPP_

#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QString>

#include <cstdint>
#include <string>
#include <vector>

#include <go2_nav_interfaces/msg/nav_mode_status.hpp>
#include <go2_nav_interfaces/srv/list_maps.hpp>
#include <go2_nav_interfaces/srv/set_nav_mode.hpp>
#include <rclcpp/rclcpp.hpp>
#include <rviz_common/panel.hpp>

namespace go2_nav_mode_panel
{

class NavModePanel : public rviz_common::Panel
{
  Q_OBJECT

public:
  explicit NavModePanel(QWidget * parent = nullptr);
  void onInitialize() override;

private Q_SLOTS:
  void apply();
  void refreshMaps();
  void updateControls();

private:
  using NavModeStatus = go2_nav_interfaces::msg::NavModeStatus;
  using SetNavMode = go2_nav_interfaces::srv::SetNavMode;
  using ListMaps = go2_nav_interfaces::srv::ListMaps;

  void showStatus(const NavModeStatus & status);
  void setMaps(const std::vector<std::string> & map_names);
  std::string selectedMode() const;

  QRadioButton * idle_button_;
  QRadioButton * mapping_button_;
  QRadioButton * localization_button_;
  QComboBox * map_combo_;
  QPushButton * refresh_button_;
  QPushButton * apply_button_;
  QLabel * status_label_;
  QLabel * response_label_;

  rclcpp::Node::SharedPtr node_;
  rclcpp::Subscription<NavModeStatus>::SharedPtr status_sub_;
  rclcpp::Client<SetNavMode>::SharedPtr set_mode_client_;
  rclcpp::Client<ListMaps>::SharedPtr list_maps_client_;

  bool status_received_{false};
  bool switching_{false};
  bool request_pending_{false};
  std::uint64_t request_seq_{0};
  QString status_map_;
};

}  // namespace go2_nav_mode_panel

#endif  // NAV_MODE_PANEL_HPP_
