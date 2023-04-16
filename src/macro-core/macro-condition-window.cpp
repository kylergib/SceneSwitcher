#include "macro-condition-window.hpp"
#include "switcher-data.hpp"
#include "platform-funcs.hpp"
#include "utility.hpp"

#include <regex>

namespace advss {

const std::string MacroConditionWindow::id = "window";

bool MacroConditionWindow::_registered = MacroConditionFactory::Register(
	MacroConditionWindow::id,
	{MacroConditionWindow::Create, MacroConditionWindowEdit::Create,
	 "AdvSceneSwitcher.condition.window"});

static bool matchRegex(const RegexConfig &conf, const std::string &msg,
		       const std::string &expr)
{
	auto regex = conf.GetRegularExpression(expr);
	if (!regex.isValid()) {
		return false;
	}
	auto match = regex.match(QString::fromStdString(msg));
	return match.hasMatch();
}

static bool windowContainsText(const std::string &window,
			       const std::string &matchText,
			       const RegexConfig &conf)
{
	auto text = GetTextInWindow(window);
	if (!text.has_value()) {
		return false;
	}

	if (conf.Enabled()) {
		return matchRegex(conf, *text, matchText);
	}
	return text == matchText;
}

bool MacroConditionWindow::WindowMatches(const std::string &window)
{
	const bool focusCheckOK = (!_focus || window == switcher->currentTitle);
	if (!focusCheckOK) {
		return false;
	}
	const bool fullscreenCheckOK = (!_fullscreen || IsFullscreen(window));
	if (!fullscreenCheckOK) {
		return false;
	}
	const bool maxCheckOK = (!_maximized || IsMaximized(window));
	if (!maxCheckOK) {
		return false;
	}
	const bool textCheckOK =
		(!_checkText || windowContainsText(window, _text, _regex));
	if (!textCheckOK) {
		return false;
	}
	return true;
}

bool MacroConditionWindow::WindowRegexMatches(
	const std::vector<std::string> &windowList)
{
	for (auto &window : windowList) {
		try {
			std::regex expr(_window);
			if (!std::regex_match(window, expr)) {
				continue;
			}
		} catch (const std::regex_error &) {
		}

		if (WindowMatches(window)) {
			return true;
		}
	}
	return false;
}

static bool foregroundWindowChanged()
{
	return switcher->currentTitle != switcher->lastTitle;
}

bool MacroConditionWindow::CheckCondition()
{
	const std::string &currentWindowTitle = switcher->currentTitle;
	SetVariableValue(currentWindowTitle);

	std::vector<std::string> windowList;
	GetWindowList(windowList);

	bool match = false;
	if (std::find(windowList.begin(), windowList.end(), _window) !=
	    windowList.end()) {
		match = WindowMatches(_window);
	} else {
		match = WindowRegexMatches(windowList);
	}
	match = match && (!_windowFocusChanged || foregroundWindowChanged());
	return match;
}

bool MacroConditionWindow::Save(obs_data_t *obj) const
{
	MacroCondition::Save(obj);
	obs_data_set_bool(obj, "checkTitle", _checkTitle);
	obs_data_set_string(obj, "window", _window.c_str());
	obs_data_set_bool(obj, "fullscreen", _fullscreen);
	obs_data_set_bool(obj, "maximized", _maximized);
	obs_data_set_bool(obj, "focus", _focus);
	obs_data_set_bool(obj, "windowFocusChanged", _windowFocusChanged);
	obs_data_set_bool(obj, "checkWindowText", _checkText);
	_text.Save(obj, "text");
	_regex.Save(obj);
	obs_data_set_int(obj, "version", 1);
	return true;
}

bool MacroConditionWindow::Load(obs_data_t *obj)
{
	MacroCondition::Load(obj);
	if (!obs_data_has_user_value(obj, "version")) {
		_checkTitle = true;
	} else {
		_checkTitle = obs_data_get_bool(obj, "checkTitle");
	}
	_window = obs_data_get_string(obj, "window");
	_fullscreen = obs_data_get_bool(obj, "fullscreen");
	_maximized = obs_data_get_bool(obj, "maximized");
	_focus = obs_data_get_bool(obj, "focus");
	_windowFocusChanged = obs_data_get_bool(obj, "windowFocusChanged");
#ifdef _WIN32
	_checkText = obs_data_get_bool(obj, "checkWindowText");
#else
	_checkText = false;
#endif
	_text.Load(obj, "text");
	_regex.Load(obj);
	return true;
}

std::string MacroConditionWindow::GetShortDesc() const
{
	return _window;
}

MacroConditionWindowEdit::MacroConditionWindowEdit(
	QWidget *parent, std::shared_ptr<MacroConditionWindow> entryData)
	: QWidget(parent),
	  _windowSelection(new QComboBox()),
	  _checkTitle(new QCheckBox()),
	  _fullscreen(new QCheckBox()),
	  _maximized(new QCheckBox()),
	  _focused(new QCheckBox()),
	  _windowFocusChanged(new QCheckBox()),
	  _checkText(new QCheckBox()),
	  _text(new VariableTextEdit(this)),
	  _regex(new RegexConfigWidget(this)),
	  _focusWindow(new QLabel()),
	  _currentFocusLayout(new QHBoxLayout())
{
	_windowSelection->setEditable(true);
	_windowSelection->setMaxVisibleItems(20);

	QWidget::connect(_windowSelection,
			 SIGNAL(currentTextChanged(const QString &)), this,
			 SLOT(WindowChanged(const QString &)));
	QWidget::connect(_checkTitle, SIGNAL(stateChanged(int)), this,
			 SLOT(CheckTitleChanged(int)));
	QWidget::connect(_fullscreen, SIGNAL(stateChanged(int)), this,
			 SLOT(FullscreenChanged(int)));
	QWidget::connect(_maximized, SIGNAL(stateChanged(int)), this,
			 SLOT(MaximizedChanged(int)));
	QWidget::connect(_focused, SIGNAL(stateChanged(int)), this,
			 SLOT(FocusedChanged(int)));
	QWidget::connect(_windowFocusChanged, SIGNAL(stateChanged(int)), this,
			 SLOT(WindowFocusChanged(int)));
	QWidget::connect(_checkText, SIGNAL(stateChanged(int)), this,
			 SLOT(CheckTextChanged(int)));
	QWidget::connect(_text, SIGNAL(textChanged()), this,
			 SLOT(WindowTextChanged()));
	QWidget::connect(_regex, SIGNAL(RegexConfigChanged(RegexConfig)), this,
			 SLOT(RegexChanged(RegexConfig)));
	QWidget::connect(&_timer, SIGNAL(timeout()), this,
			 SLOT(UpdateFocusWindow()));

	PopulateWindowSelection(_windowSelection);

	std::unordered_map<std::string, QWidget *> widgetPlaceholders = {
		{"{{windows}}", _windowSelection},
		{"{{checkTitle}}", _checkTitle},
		{"{{fullscreen}}", _fullscreen},
		{"{{maximized}}", _maximized},
		{"{{focused}}", _focused},
		{"{{windowFocusChanged}}", _windowFocusChanged},
		{"{{focusWindow}}", _focusWindow},
		{"{{checkText}}", _checkText},
		{"{{windowText}}", _text},
		{"{{regex}}", _regex},
	};

	auto titleLayout = new QHBoxLayout;
	titleLayout->setContentsMargins(0, 0, 0, 0);
	PlaceWidgets(obs_module_text(
			     "AdvSceneSwitcher.condition.window.entry.window"),
		     titleLayout, widgetPlaceholders);
	auto fullscreenLayout = new QHBoxLayout;
	fullscreenLayout->setContentsMargins(0, 0, 0, 0);
	PlaceWidgets(
		obs_module_text(
			"AdvSceneSwitcher.condition.window.entry.fullscreen"),
		fullscreenLayout, widgetPlaceholders);
	auto maximizedLayout = new QHBoxLayout;
	maximizedLayout->setContentsMargins(0, 0, 0, 0);
	PlaceWidgets(
		obs_module_text(
			"AdvSceneSwitcher.condition.window.entry.maximized"),
		maximizedLayout, widgetPlaceholders);
	auto focusedLayout = new QHBoxLayout;
	focusedLayout->setContentsMargins(0, 0, 0, 0);
	PlaceWidgets(obs_module_text(
			     "AdvSceneSwitcher.condition.window.entry.focused"),
		     focusedLayout, widgetPlaceholders);
	auto focusChangedLayout = new QHBoxLayout;
	focusChangedLayout->setContentsMargins(0, 0, 0, 0);
	PlaceWidgets(
		obs_module_text(
			"AdvSceneSwitcher.condition.window.entry.focusedChange"),
		focusChangedLayout, widgetPlaceholders);
	auto textLayout = new QHBoxLayout;
	textLayout->setContentsMargins(0, 0, 0, 0);
	PlaceWidgets(
		obs_module_text("AdvSceneSwitcher.condition.window.entry.text"),
		textLayout, widgetPlaceholders);
	_text->setSizePolicy(QSizePolicy::MinimumExpanding,
			     QSizePolicy::Preferred);
	textLayout->setStretchFactor(_text, 10);
	_currentFocusLayout->setContentsMargins(0, 0, 0, 0);
	PlaceWidgets(
		obs_module_text(
			"AdvSceneSwitcher.condition.window.entry.currentFocus"),
		_currentFocusLayout, widgetPlaceholders);
	auto regexLayout = new QHBoxLayout;
	regexLayout->setContentsMargins(0, 0, 0, 0);
	regexLayout->addWidget(_regex);
	regexLayout->addStretch();

	auto mainLayout = new QVBoxLayout;
	mainLayout->addLayout(titleLayout);
	mainLayout->addLayout(fullscreenLayout);
	mainLayout->addLayout(maximizedLayout);
	mainLayout->addLayout(focusedLayout);
	mainLayout->addLayout(focusChangedLayout);
	mainLayout->addLayout(textLayout);
	mainLayout->addLayout(regexLayout);
#ifndef _WIN32
	SetLayoutVisible(textLayout, false);
	SetLayoutVisible(regexLayout, false);
#endif
	mainLayout->addLayout(_currentFocusLayout);
	setLayout(mainLayout);

	_entryData = entryData;
	UpdateEntryData();
	_loading = false;

	_timer.start(1000);
}

void MacroConditionWindowEdit::WindowChanged(const QString &text)
{
	if (_loading || !_entryData) {
		return;
	}

	auto lock = LockContext();
	_entryData->_window = text.toStdString();
	emit HeaderInfoChanged(
		QString::fromStdString(_entryData->GetShortDesc()));
}

void MacroConditionWindowEdit::CheckTitleChanged(int state)
{
	if (_loading || !_entryData) {
		return;
	}

	std::lock_guard<std::mutex> lock(switcher->m);
	if (!state) {
		const QSignalBlocker b(_windowSelection);
		_entryData->_window = ".*";
		_windowSelection->setCurrentText(".*");
	}
	_entryData->_checkTitle = state;
	SetWidgetVisibility();
}

void MacroConditionWindowEdit::CheckTextChanged(int state)
{
	if (_loading || !_entryData) {
		return;
	}

	std::lock_guard<std::mutex> lock(switcher->m);
	_entryData->_checkText = state;
	SetWidgetVisibility();
}

void MacroConditionWindowEdit::WindowTextChanged()
{
	if (_loading || !_entryData) {
		return;
	}

	std::lock_guard<std::mutex> lock(switcher->m);
	_entryData->_text = _text->toPlainText().toStdString();
	adjustSize();
	updateGeometry();
}

void MacroConditionWindowEdit::RegexChanged(RegexConfig conf)
{
	if (_loading || !_entryData) {
		return;
	}

	std::lock_guard<std::mutex> lock(switcher->m);
	_entryData->_regex = conf;
	adjustSize();
	updateGeometry();
}

void MacroConditionWindowEdit::FullscreenChanged(int state)
{
	if (_loading || !_entryData) {
		return;
	}

	auto lock = LockContext();
	_entryData->_fullscreen = state;
}

void MacroConditionWindowEdit::MaximizedChanged(int state)
{
	if (_loading || !_entryData) {
		return;
	}

	auto lock = LockContext();
	_entryData->_maximized = state;
}

void MacroConditionWindowEdit::FocusedChanged(int state)
{
	if (_loading || !_entryData) {
		return;
	}

	auto lock = LockContext();
	_entryData->_focus = state;
	SetWidgetVisibility();
}

void MacroConditionWindowEdit::WindowFocusChanged(int state)
{
	if (_loading || !_entryData) {
		return;
	}

	auto lock = LockContext();
	_entryData->_windowFocusChanged = state;
	SetWidgetVisibility();
}

void MacroConditionWindowEdit::UpdateFocusWindow()
{
	_focusWindow->setText(QString::fromStdString(switcher->currentTitle));
}

void MacroConditionWindowEdit::SetWidgetVisibility()
{
	if (!_entryData) {
		return;
	}
	SetLayoutVisible(_currentFocusLayout,
			 _entryData->_focus || _entryData->_windowFocusChanged);
	_windowSelection->setVisible(_entryData->_checkTitle);
	_regex->setVisible(_entryData->_checkText);
	_text->setVisible(_entryData->_checkText);
	adjustSize();
	updateGeometry();
}

void MacroConditionWindowEdit::UpdateEntryData()
{
	if (!_entryData) {
		return;
	}

	_windowSelection->setCurrentText(_entryData->_window.c_str());
	_checkTitle->setChecked(_entryData->_checkTitle);
	_fullscreen->setChecked(_entryData->_fullscreen);
	_maximized->setChecked(_entryData->_maximized);
	_focused->setChecked(_entryData->_focus);
	_windowFocusChanged->setChecked(_entryData->_windowFocusChanged);
	_checkText->setChecked(_entryData->_checkText);
	_text->setPlainText(_entryData->_text);
	_regex->SetRegexConfig(_entryData->_regex);
	SetWidgetVisibility();
}

} // namespace advss
