#pragma once
#include "obs-dock.hpp"

#include <QPushButton>
#include <QTimer>
#include <memory>

namespace advss {

class Macro;

class MacroDock : public OBSDock {
	Q_OBJECT

public:
	MacroDock(Macro *, QWidget *parent);
	void SetName(const QString &);
	void ShowRunButton(bool);
	void ShowPauseButton(bool);

private slots:
	void RunClicked();
	void PauseToggleClicked();
	void UpdatePauseText();

private:
	QPushButton *_run;
	QPushButton *_pauseToggle;
	QTimer _timer;

	Macro *_macro;
};

} // namespace advss
