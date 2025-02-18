#pragma once
#include "macro-segment.hpp"
#include "macro-ref.hpp"
#include <duration-control.hpp>

namespace advss {

constexpr auto logic_root_offset = 100;

enum class LogicType {
	ROOT_NONE = 0,
	ROOT_NOT,
	ROOT_LAST,
	// leave some space for potential expansion
	NONE = 100,
	AND,
	OR,
	AND_NOT,
	OR_NOT,
	LAST,
};

static inline bool isRootLogicType(LogicType l)
{
	return static_cast<int>(l) < logic_root_offset;
}

struct LogicTypeInfo {
	std::string _name;
};

class DurationModifier {
public:
	enum class Type {
		NONE,
		MORE,
		EQUAL,
		LESS,
		WITHIN, // Condition will remain true for a set a amount of time
			// regardless of current state
	};

	void Save(obs_data_t *obj, const char *condName = "time_constraint",
		  const char *duration = "seconds") const;
	void Load(obs_data_t *obj, const char *condName = "time_constraint",
		  const char *duration = "seconds");
	void SetModifier(Type cond) { _type = cond; }
	void SetTimeRemaining(const double &val) { _dur.SetTimeRemaining(val); }
	void SetValue(const Duration &value) { _dur = value; }
	Type GetType() { return _type; }
	Duration GetDuration() { return _dur; }
	bool DurationReached();
	void Reset();

private:
	Type _type = Type::NONE;
	Duration _dur;
	bool _timeReached = false;
};

class MacroCondition : public MacroSegment {
public:
	MacroCondition(Macro *m, bool supportsVariableValue = false);
	virtual ~MacroCondition() = default;
	virtual bool CheckCondition() = 0;
	virtual bool Save(obs_data_t *obj) const = 0;
	virtual bool Load(obs_data_t *obj) = 0;
	LogicType GetLogicType() { return _logic; }
	void SetLogicType(LogicType logic) { _logic = logic; }
	static const std::map<LogicType, LogicTypeInfo> logicTypes;
	void ResetDuration();
	void CheckDurationModifier(bool &val);
	DurationModifier GetDurationModifier() { return _duration; }
	void SetDurationModifier(DurationModifier::Type m);
	void SetDuration(const Duration &duration);

private:
	LogicType _logic = LogicType::ROOT_NONE;
	DurationModifier _duration;
};

class MacroRefCondition : virtual public MacroCondition {
public:
	MacroRefCondition(Macro *m, bool supportsVariableValue = false);
	bool PostLoad() override;

	MacroRef _macro;
};

class MultiMacroRefCondtition : virtual public MacroCondition {
public:
	MultiMacroRefCondtition(Macro *m, bool supportsVariableValue = false);
	bool PostLoad() override;

	std::vector<MacroRef> _macros;
};

} // namespace advss
