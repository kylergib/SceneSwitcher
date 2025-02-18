#include "macro-action-wait.hpp"
#include "switcher-data.hpp"
#include "utility.hpp"

#include <random>

namespace advss {

const std::string MacroActionWait::id = "wait";

bool MacroActionWait::_registered = MacroActionFactory::Register(
	MacroActionWait::id,
	{MacroActionWait::Create, MacroActionWaitEdit::Create,
	 "AdvSceneSwitcher.action.wait"});

static std::map<WaitType, std::string> waitTypes = {
	{WaitType::FIXED, "AdvSceneSwitcher.action.wait.type.fixed"},
	{WaitType::RANDOM, "AdvSceneSwitcher.action.wait.type.random"},
};

static std::random_device rd;
static std::default_random_engine re(rd());

static void waitHelper(std::unique_lock<std::mutex> *lock, Macro *macro,
		       std::chrono::high_resolution_clock::time_point &time)
{
	while (!switcher->abortMacroWait && !macro->GetStop()) {
		if (switcher->macroWaitCv.wait_until(*lock, time) ==
		    std::cv_status::timeout) {
			break;
		}
	}
}

bool MacroActionWait::PerformAction()
{
	double sleepDuration;
	if (_waitType == WaitType::FIXED) {
		sleepDuration = _duration.Seconds();
	} else {
		double min = (_duration.Seconds() < _duration2.Seconds())
				     ? _duration.Seconds()
				     : _duration2.Seconds();
		double max = (_duration.Seconds() < _duration2.Seconds())
				     ? _duration2.Seconds()
				     : _duration.Seconds();
		std::uniform_real_distribution<double> unif(min, max);
		sleepDuration = unif(re);
	}
	vblog(LOG_INFO, "perform action wait with duration of %f",
	      sleepDuration);

	auto time = std::chrono::high_resolution_clock::now() +
		    std::chrono::milliseconds((int)(sleepDuration * 1000));

	switcher->abortMacroWait = false;
	std::unique_lock<std::mutex> lock(switcher->m);
	waitHelper(&lock, GetMacro(), time);

	return !switcher->abortMacroWait;
}

bool MacroActionWait::Save(obs_data_t *obj) const
{
	MacroAction::Save(obj);
	_duration.Save(obj);
	_duration2.Save(obj, "duration2");
	obs_data_set_int(obj, "waitType", static_cast<int>(_waitType));
	obs_data_set_int(obj, "version", 1);
	return true;
}

bool MacroActionWait::Load(obs_data_t *obj)
{
	MacroAction::Load(obj);
	_duration.Load(obj);
	// TODO: remove this fallback
	if (obs_data_get_int(obj, "version") == 1) {
		_duration2.Load(obj, "duration2");
	} else {
		_duration2.Load(obj, "seconds2");
		_duration2.SetUnit(static_cast<Duration::Unit>(
			obs_data_get_int(obj, "displayUnit2")));
	}
	_waitType = static_cast<WaitType>(obs_data_get_int(obj, "waitType"));
	return true;
}

static inline void populateTypeSelection(QComboBox *list)
{
	for (auto entry : waitTypes) {
		list->addItem(obs_module_text(entry.second.c_str()));
	}
}

MacroActionWaitEdit::MacroActionWaitEdit(
	QWidget *parent, std::shared_ptr<MacroActionWait> entryData)
	: QWidget(parent)
{
	_duration = new DurationSelection();
	_duration2 = new DurationSelection();
	_waitType = new QComboBox();

	populateTypeSelection(_waitType);

	QWidget::connect(_duration, SIGNAL(DurationChanged(const Duration &)),
			 this, SLOT(DurationChanged(const Duration &)));
	QWidget::connect(_duration2, SIGNAL(DurationChanged(const Duration &)),
			 this, SLOT(Duration2Changed(const Duration &)));
	QWidget::connect(_waitType, SIGNAL(currentIndexChanged(int)), this,
			 SLOT(TypeChanged(int)));

	_mainLayout = new QHBoxLayout;
	setLayout(_mainLayout);

	_entryData = entryData;
	UpdateEntryData();
	_loading = false;
}

void MacroActionWaitEdit::UpdateEntryData()
{
	if (!_entryData) {
		return;
	}

	if (_entryData->_waitType == WaitType::FIXED) {
		SetupFixedDurationEdit();
	} else {
		SetupRandomDurationEdit();
	}

	_duration->SetDuration(_entryData->_duration);
	_duration2->SetDuration(_entryData->_duration2);
	_waitType->setCurrentIndex(static_cast<int>(_entryData->_waitType));
}

void MacroActionWaitEdit::SetupFixedDurationEdit()
{
	_mainLayout->removeWidget(_duration);
	_mainLayout->removeWidget(_duration2);
	_mainLayout->removeWidget(_waitType);
	ClearLayout(_mainLayout);
	std::unordered_map<std::string, QWidget *> widgetPlaceholders = {
		{"{{duration}}", _duration},
		{"{{waitType}}", _waitType},
	};
	PlaceWidgets(
		obs_module_text("AdvSceneSwitcher.action.wait.entry.fixed"),
		_mainLayout, widgetPlaceholders);
	_duration2->hide();
}

void MacroActionWaitEdit::SetupRandomDurationEdit()
{
	_mainLayout->removeWidget(_duration);
	_mainLayout->removeWidget(_duration2);
	_mainLayout->removeWidget(_waitType);
	ClearLayout(_mainLayout);
	std::unordered_map<std::string, QWidget *> widgetPlaceholders = {
		{"{{duration}}", _duration},
		{"{{duration2}}", _duration2},
		{"{{waitType}}", _waitType},
	};
	PlaceWidgets(
		obs_module_text("AdvSceneSwitcher.action.wait.entry.random"),
		_mainLayout, widgetPlaceholders);
	_duration2->show();
}

void MacroActionWaitEdit::TypeChanged(int value)
{
	if (_loading || !_entryData) {
		return;
	}

	auto lock = LockContext();
	WaitType type = static_cast<WaitType>(value);

	if (type == WaitType::FIXED) {
		SetupFixedDurationEdit();
	} else {
		SetupRandomDurationEdit();
	}

	_entryData->_waitType = type;
}

void MacroActionWaitEdit::DurationChanged(const Duration &dur)
{
	if (_loading || !_entryData) {
		return;
	}

	auto lock = LockContext();
	_entryData->_duration = dur;
}

void MacroActionWaitEdit::Duration2Changed(const Duration &dur)
{
	if (_loading || !_entryData) {
		return;
	}

	auto lock = LockContext();
	_entryData->_duration2 = dur;
}

} // namespace advss
