#include "nav_mode_panel.hpp"

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QMetaObject>
#include <QTimer>
#include <QVBoxLayout>

#include <memory>
#include <string>
#include <vector>

#include <pluginlib/class_list_macros.hpp>
#include <rviz_common/display_context.hpp>
#include <rviz_common/ros_integration/ros_node_abstraction_iface.hpp>

namespace go2_nav_mode_panel
{

namespace
{
constexpr int kResponseTimeoutMs = 10000;
constexpr char kSetModeService[] = "/nav_mode/set_mode";
constexpr char kListMapsService[] = "/nav_mode/list_maps";
}  // namespace

NavModePanel::NavModePanel(QWidget * parent)
: rviz_common::Panel(parent),
  idle_button_(new QRadioButton("Idle")),
  mapping_button_(new QRadioButton("Mapping")),
  localization_button_(new QRadioButton("Localization")),
  map_combo_(new QComboBox),
  refresh_button_(new QPushButton("Refresh")),
  apply_button_(new QPushButton("Apply")),
  status_label_(new QLabel("Waiting for /nav_mode/status ...")),
  response_label_(new QLabel)
{
  auto * modes = new QButtonGroup(this);
  auto * mode_layout = new QHBoxLayout;
  for (auto * button : {idle_button_, mapping_button_, localization_button_}) {
    modes->addButton(button);
    mode_layout->addWidget(button);
    connect(button, &QRadioButton::toggled, this, &NavModePanel::updateControls);
  }
  idle_button_->setChecked(true);

  auto * map_layout = new QHBoxLayout;
  map_layout->addWidget(new QLabel("Map:"));
  map_layout->addWidget(map_combo_, 1);
  map_layout->addWidget(refresh_button_);

  for (auto * label : {status_label_, response_label_}) {
    label->setTextFormat(Qt::PlainText);
    label->setWordWrap(true);
  }

  auto * layout = new QVBoxLayout;
  layout->addLayout(mode_layout);
  layout->addLayout(map_layout);
  layout->addWidget(apply_button_);
  layout->addWidget(status_label_);
  layout->addWidget(response_label_);
  layout->addStretch();
  setLayout(layout);

  connect(refresh_button_, &QPushButton::clicked, this, &NavModePanel::refreshMaps);
  connect(apply_button_, &QPushButton::clicked, this, &NavModePanel::apply);
  connect(
    map_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
    this, &NavModePanel::updateControls);
  updateControls();
}

void NavModePanel::onInitialize()
{
  node_ = getDisplayContext()->getRosNodeAbstraction().lock()->get_raw_node();
  set_mode_client_ = node_->create_client<SetNavMode>(kSetModeService);
  list_maps_client_ = node_->create_client<ListMaps>(kListMapsService);
  // ROS callbacks only queue work onto the GUI thread; they never block it.
  status_sub_ = node_->create_subscription<NavModeStatus>(
    "/nav_mode/status", rclcpp::QoS(1).reliable().transient_local(),
    [this](NavModeStatus::ConstSharedPtr msg) {
      QMetaObject::invokeMethod(this, [this, msg]() {showStatus(*msg);}, Qt::QueuedConnection);
    });
}

void NavModePanel::showStatus(const NavModeStatus & status)
{
  switching_ = status.state == NavModeStatus::SWITCHING;
  status_map_ = QString::fromStdString(status.map_name);
  QString mode = QString::fromStdString(status.mode);
  if (!status_map_.isEmpty()) {
    mode += QString(" (%1)").arg(status_map_);
  }
  QString state = QString::fromStdString(status.state);
  QString style;
  if (status.state == NavModeStatus::SWITCHING) {
    state = "Switching...";
  } else if (status.state == NavModeStatus::WAITING_FOR_POSE) {
    state = "Waiting for initial pose (2D Pose Estimate)";
    style = "color: #d08000; font-weight: bold;";
  } else if (status.state == NavModeStatus::READY) {
    state = "Ready";
  } else if (status.state == NavModeStatus::FAILED) {
    state = "Failed";
    style = "color: red;";
  }
  status_label_->setText(
    QString("Robot: %1 - %2\n%3").arg(mode, state, QString::fromStdString(status.message)));
  status_label_->setStyleSheet(style);

  if (!status_received_) {
    status_received_ = true;
    if (status.mode == SetNavMode::Request::MAPPING) {
      mapping_button_->setChecked(true);
    } else if (status.mode == SetNavMode::Request::LOCALIZATION) {
      localization_button_->setChecked(true);
    }
    refreshMaps();
  }
  updateControls();
}

void NavModePanel::refreshMaps()
{
  if (!list_maps_client_) {
    return;
  }
  if (!list_maps_client_->service_is_ready()) {
    response_label_->setText(QString("Service %1 is not available").arg(kListMapsService));
    return;
  }
  list_maps_client_->prune_pending_requests();
  list_maps_client_->async_send_request(
    std::make_shared<ListMaps::Request>(),
    [this](rclcpp::Client<ListMaps>::SharedFuture future) {
      auto names = future.get()->map_names;
      QMetaObject::invokeMethod(this, [this, names]() {setMaps(names);}, Qt::QueuedConnection);
    });
}

void NavModePanel::setMaps(const std::vector<std::string> & map_names)
{
  QString selected = map_combo_->currentText();
  if (selected.isEmpty()) {
    selected = status_map_;
  }
  map_combo_->clear();
  for (const auto & name : map_names) {
    map_combo_->addItem(QString::fromStdString(name));
  }
  const int index = map_combo_->findText(selected);
  if (index >= 0) {
    map_combo_->setCurrentIndex(index);
  }
  response_label_->setStyleSheet("");
  response_label_->setText(QString("%1 saved map(s) found").arg(map_names.size()));
  updateControls();
}

std::string NavModePanel::selectedMode() const
{
  if (mapping_button_->isChecked()) {
    return SetNavMode::Request::MAPPING;
  }
  if (localization_button_->isChecked()) {
    return SetNavMode::Request::LOCALIZATION;
  }
  return SetNavMode::Request::IDLE;
}

void NavModePanel::apply()
{
  if (!set_mode_client_ || switching_ || request_pending_) {
    return;
  }
  if (!set_mode_client_->service_is_ready()) {
    response_label_->setStyleSheet("color: red;");
    response_label_->setText(
      QString("Service %1 is not available; is the robot nav service running?")
      .arg(kSetModeService));
    return;
  }
  auto request = std::make_shared<SetNavMode::Request>();
  request->mode = selectedMode();
  if (localization_button_->isChecked()) {
    request->map_name = map_combo_->currentText().toStdString();
  }
  const auto seq = ++request_seq_;
  request_pending_ = true;
  response_label_->setStyleSheet("");
  response_label_->setText(QString("Requesting %1 ...").arg(QString::fromStdString(request->mode)));
  const auto pending = set_mode_client_->async_send_request(
    request,
    [this, seq](rclcpp::Client<SetNavMode>::SharedFuture future) {
      auto response = future.get();
      QMetaObject::invokeMethod(
        this, [this, seq, response]() {
          if (seq != request_seq_) {
            return;
          }
          request_pending_ = false;
          response_label_->setStyleSheet(response->success ? "" : "color: red;");
          response_label_->setText(QString::fromStdString(response->message));
          updateControls();
        }, Qt::QueuedConnection);
    });
  const auto request_id = pending.request_id;
  QTimer::singleShot(
    kResponseTimeoutMs, this, [this, seq, request_id]() {
      if (!request_pending_ || seq != request_seq_) {
        return;
      }
      set_mode_client_->remove_pending_request(request_id);
      request_pending_ = false;
      ++request_seq_;
      response_label_->setStyleSheet("color: red;");
      response_label_->setText(QString("No response from %1").arg(kSetModeService));
      updateControls();
    });
  updateControls();
}

void NavModePanel::updateControls()
{
  const bool localization = localization_button_->isChecked();
  map_combo_->setEnabled(localization);
  apply_button_->setEnabled(
    !switching_ && !request_pending_ && (!localization || map_combo_->count() > 0));
  apply_button_->setText(switching_ ? "Switching..." : "Apply");
}

}  // namespace go2_nav_mode_panel

PLUGINLIB_EXPORT_CLASS(go2_nav_mode_panel::NavModePanel, rviz_common::Panel)
