#pragma once
#include "macro-action-edit.hpp"
#include "variable-spinbox.hpp"

#include <QDoubleSpinBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <chrono>

namespace advss {

class MacroActionStream : public MacroAction {
public:
	MacroActionStream(Macro *m) : MacroAction(m) {}
	bool PerformAction();
	void LogAction() const;
	bool Save(obs_data_t *obj) const;
	bool Load(obs_data_t *obj);
	std::string GetId() const { return id; };
	static std::shared_ptr<MacroAction> Create(Macro *m)
	{
		return std::make_shared<MacroActionStream>(m);
	}

	enum class Action {
		STOP,
		START,
		KEYFRAME_INTERVAL,
	};
	Action _action = Action::STOP;
	NumberVariable<int> _keyFrameInterval = 0;

private:
	void SetKeyFrameInterval();
	bool CooldownDurationReached();
	static std::chrono::high_resolution_clock::time_point s_lastAttempt;

	static bool _registered;
	static const std::string id;
};

class MacroActionStreamEdit : public QWidget {
	Q_OBJECT

public:
	MacroActionStreamEdit(
		QWidget *parent,
		std::shared_ptr<MacroActionStream> entryData = nullptr);
	void UpdateEntryData();
	static QWidget *Create(QWidget *parent,
			       std::shared_ptr<MacroAction> action)
	{
		return new MacroActionStreamEdit(
			parent,
			std::dynamic_pointer_cast<MacroActionStream>(action));
	}

private slots:
	void ActionChanged(int value);
	void KeyFrameIntervalChanged(const NumberVariable<int> &);

protected:
	QComboBox *_actions;
	VariableSpinBox *_keyFrameInterval;
	std::shared_ptr<MacroActionStream> _entryData;

private:
	void SetWidgetVisiblity();

	QHBoxLayout *_mainLayout;
	bool _loading = true;
};

} // namespace advss
