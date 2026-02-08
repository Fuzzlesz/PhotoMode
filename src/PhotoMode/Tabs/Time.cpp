#include "Time.h"

namespace PhotoMode
{
	void Time::OriginalState::Get()
	{
		freezeTime = RE::Main::GetSingleton()->freezeTime;
		globalTimeMult = RE::BSTimer::QGlobalTimeMultiplier();

		const auto calendar = RE::Calendar::GetSingleton();
		timescale = calendar->GetTimescale();
		gameHour = calendar->gameHour->value;
	}

	void Time::OriginalState::Revert() const
	{
		RE::Main::GetSingleton()->freezeTime = freezeTime;
		RE::BSTimer::GetSingleton()->SetGlobalTimeMultiplier(globalTimeMult, true);

		const auto calendar = RE::Calendar::GetSingleton();
		calendar->timeScale->value = timescale;
		calendar->gameHour->value = gameHour;
	}

	void Time::GetOriginalState()
	{
		originalState.Get();
	}

	void Time::RevertState()
	{
		originalState.Revert();

		// is this needed when value is updated every frame?
		currentTimescaleMult = 1.0f;
		currentGlobalTimeMult = 1.0f;

		// revert weather
		if (weatherForced) {
			const auto sky = RE::Sky::GetSingleton();
			sky->ReleaseWeatherOverride();
			sky->ResetWeather();
			if (originalWeather) {
				sky->ForceWeather(originalWeather, false);
			}
			weatherForced = false;
		}
	}

	void Time::OnFrameUpdate() const
	{
		if (weatherForced) {
			RE::Sky::GetSingleton()->lastWeatherUpdate = RE::Calendar::GetSingleton()->gameHour->value;
		}
	}

	void Time::Draw()
	{
		FUCK::Checkbox("$PM_FreezeTime"_T, &RE::Main::GetSingleton()->freezeTime);

		currentGlobalTimeMult = RE::BSTimer::QGlobalTimeMultiplier();
		if (FUCK::SliderFloat("$PM_GlobalTimeMult"_T, &currentGlobalTimeMult, 0.01f, 2.0f)) {
			RE::BSTimer::GetSingleton()->SetGlobalTimeMultiplier(currentGlobalTimeMult, true);
		}

		FUCK::Dummy({ 0, 5 });

		auto&       gameHour = RE::Calendar::GetSingleton()->gameHour->value;
		std::string timeFormat = std::format("{:%I:%M %p}", std::chrono::duration<float, std::ratio<3600>>(gameHour));
		FUCK::SliderFloat("$PM_GameHour"_T, &gameHour, 0.0f, 23.99f, timeFormat.c_str());

		if (FUCK::DragFloat("$PM_TimeScaleMult"_T, &currentTimescaleMult, 10.0f, 1.0f, 1000.0f, "%.0fX")) {
			RE::Calendar::GetSingleton()->timeScale->value = originalState.timescale * currentTimescaleMult;
		}

		FUCK::BeginDisabled(RE::Sky::GetSingleton()->mode == RE::Sky::Mode::kInterior);
		{
			// Weather selection using ComboForm
			std::uint32_t weatherFormID = originalWeather ? originalWeather->GetFormID() : 0;
			if (FUCK::ComboForm("$PM_Weathers"_T, &weatherFormID, static_cast<std::uint8_t>(RE::FormType::Weather))) {
				if (auto weather = RE::TESForm::LookupByID<RE::TESWeather>(weatherFormID)) {
					RE::Sky::GetSingleton()->ForceWeather(weather, true);
					weatherForced = true;
				}
			}
		}
		FUCK::EndDisabled();
	}
}
