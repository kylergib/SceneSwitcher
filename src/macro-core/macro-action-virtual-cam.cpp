#include "macro-action-virtual-cam.hpp"
#include "utility.hpp"

namespace advss {

const std::string MacroActionVCam::id = "virtual_cam";

bool MacroActionVCam::_registered = MacroActionFactory::Register(
	MacroActionVCam::id,
	{MacroActionVCam::Create, MacroActionVCamEdit::Create,
	 "AdvSceneSwitcher.action.virtualCamera"});

const static std::map<VCamAction, std::string> actionTypes = {
	{VCamAction::STOP, "AdvSceneSwitcher.action.virtualCamera.type.stop"},
	{VCamAction::START, "AdvSceneSwitcher.action.virtualCamera.type.start"},
};

bool MacroActionVCam::PerformAction()
{
#if LIBOBS_API_VER >= MAKE_SEMANTIC_VERSION(27, 0, 0)
	switch (_action) {
	case VCamAction::STOP:
		if (obs_frontend_virtualcam_active()) {
			obs_frontend_stop_virtualcam();
		}
		break;
	case VCamAction::START:
		if (!obs_frontend_virtualcam_active()) {
			obs_frontend_start_virtualcam();
		}
		break;
	default:
		break;
	}
#endif
	return true;
}

void MacroActionVCam::LogAction() const
{
	auto it = actionTypes.find(_action);
	if (it != actionTypes.end()) {
		vblog(LOG_INFO, "performed action \"%s\"", it->second.c_str());
	} else {
		blog(LOG_WARNING, "ignored unknown virtual camera action %d",
		     static_cast<int>(_action));
	}
}

bool MacroActionVCam::Save(obs_data_t *obj) const
{
	MacroAction::Save(obj);
	obs_data_set_int(obj, "action", static_cast<int>(_action));
	return true;
}

bool MacroActionVCam::Load(obs_data_t *obj)
{
	MacroAction::Load(obj);
	_action = static_cast<VCamAction>(obs_data_get_int(obj, "action"));
	return true;
}

static inline void populateActionSelection(QComboBox *list)
{
	for (auto entry : actionTypes) {
		list->addItem(obs_module_text(entry.second.c_str()));
	}
}

MacroActionVCamEdit::MacroActionVCamEdit(
	QWidget *parent, std::shared_ptr<MacroActionVCam> entryData)
	: QWidget(parent)
{
	_actions = new QComboBox();

	populateActionSelection(_actions);

	QWidget::connect(_actions, SIGNAL(currentIndexChanged(int)), this,
			 SLOT(ActionChanged(int)));

	QHBoxLayout *mainLayout = new QHBoxLayout;
	std::unordered_map<std::string, QWidget *> widgetPlaceholders = {
		{"{{actions}}", _actions},
	};
	PlaceWidgets(
		obs_module_text("AdvSceneSwitcher.action.virtualCamera.entry"),
		mainLayout, widgetPlaceholders);
	setLayout(mainLayout);

	_entryData = entryData;
	UpdateEntryData();
	_loading = false;
}

void MacroActionVCamEdit::UpdateEntryData()
{
	if (!_entryData) {
		return;
	}
	_actions->setCurrentIndex(static_cast<int>(_entryData->_action));
}

void MacroActionVCamEdit::ActionChanged(int value)
{
	if (_loading || !_entryData) {
		return;
	}

	auto lock = LockContext();
	_entryData->_action = static_cast<VCamAction>(value);
}

} // namespace advss
