#pragma once
#include "macro-action-edit.hpp"
#include "duration-control.hpp"
#include "source-selection.hpp"

#include <QSpinBox>
#include <QHBoxLayout>

namespace advss {

enum class MediaAction {
	PLAY,
	PAUSE,
	STOP,
	RESTART,
	NEXT,
	PREVIOUS,
	SEEK,
};

class MacroActionMedia : public MacroAction {
public:
	MacroActionMedia(Macro *m) : MacroAction(m) {}
	bool PerformAction();
	void LogAction() const;
	bool Save(obs_data_t *obj) const;
	bool Load(obs_data_t *obj);
	std::string GetShortDesc() const;
	std::string GetId() const { return id; };
	static std::shared_ptr<MacroAction> Create(Macro *m)
	{
		return std::make_shared<MacroActionMedia>(m);
	}

	SourceSelection _mediaSource;
	MediaAction _action = MediaAction::PLAY;
	Duration _seek;

private:
	static bool _registered;
	static const std::string id;
};

class MacroActionMediaEdit : public QWidget {
	Q_OBJECT

public:
	MacroActionMediaEdit(
		QWidget *parent,
		std::shared_ptr<MacroActionMedia> entryData = nullptr);
	void UpdateEntryData();
	static QWidget *Create(QWidget *parent,
			       std::shared_ptr<MacroAction> action)
	{
		return new MacroActionMediaEdit(
			parent,
			std::dynamic_pointer_cast<MacroActionMedia>(action));
	}

private slots:
	void SourceChanged(const SourceSelection &);
	void ActionChanged(int value);
	void DurationChanged(const Duration &value);
signals:
	void HeaderInfoChanged(const QString &);

protected:
	SourceSelectionWidget *_sources;
	QComboBox *_actions;
	DurationSelection *_seek;
	std::shared_ptr<MacroActionMedia> _entryData;

private:
	void SetWidgetVisibility();

	QHBoxLayout *_mainLayout;
	bool _loading = true;
};

} // namespace advss
