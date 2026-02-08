#include "Filters.h"

namespace PhotoMode
{
	void Filters::GetOriginalState()
	{
		const auto IMGS = RE::ImageSpaceManager::GetSingleton();
		if (IMGS->currentBaseData) {
			imageSpaceData = *IMGS->currentBaseData;
		}
		IMGS->overrideBaseData = &imageSpaceData;
	}

	void Filters::RevertState(bool a_fullReset)
	{
		// reset imagespace
		const auto IMGS = RE::ImageSpaceManager::GetSingleton();
		if (a_fullReset) {
			IMGS->overrideBaseData = nullptr;
		} else if (IMGS->overrideBaseData) {
			if (IMGS->currentBaseData) {
				imageSpaceData = *IMGS->currentBaseData;
			}
			IMGS->overrideBaseData = &imageSpaceData;
		}

		// reset imod
		if (imodPlayed) {
			if (currentImod) {
				RE::ImageSpaceModifierInstanceForm::Stop(currentImod);
				currentImod = nullptr;
			}
			imodPlayed = false;
		}
	}

	void Filters::Draw()
	{
		if (const auto& overrideData = RE::ImageSpaceManager::GetSingleton()->overrideBaseData) {
			FUCK::SliderFloat("$PM_Brightness"_T, &overrideData->cinematic.brightness, 0.0f, 3.0f);
			FUCK::SliderFloat("$PM_Saturation"_T, &overrideData->cinematic.saturation, 0.0f, 3.0f);
			FUCK::SliderFloat("$PM_Contrast"_T, &overrideData->cinematic.contrast, 0.0f, 3.0f);

			FUCK::SliderFloat("$PM_TintAlpha"_T, &overrideData->tint.amount, 0.0f, 1.0f);
			FUCK::Indent();
			{
				FUCK::SliderFloat("$PM_TintRed"_T, &overrideData->tint.color.red, 0.0f, 1.0f);
				FUCK::SliderFloat("$PM_TintBlue"_T, &overrideData->tint.color.blue, 0.0f, 1.0f);
				FUCK::SliderFloat("$PM_TintGreen"_T, &overrideData->tint.color.green, 0.0f, 1.0f);
			}
			FUCK::Unindent();
		} else {
			RE::ImageSpaceManager::GetSingleton()->overrideBaseData = &imageSpaceData;
		}

		// ImageSpace Modifier selection using ComboForm
		std::uint32_t imodFormID = currentImod ? currentImod->GetFormID() : 0;
		if (FUCK::ComboForm("$PM_ImageSpaceModifiers"_T, &imodFormID, static_cast<std::uint8_t>(RE::FormType::ImageAdapter))) {
			if (auto imod = RE::TESForm::LookupByID<RE::TESImageSpaceModifier>(imodFormID)) {
				if (currentImod) {
					RE::ImageSpaceModifierInstanceForm::Stop(currentImod);
				}
				RE::ImageSpaceModifierInstanceForm::Trigger(imod, 1.0, nullptr);
				currentImod = imod;
				imodPlayed = true;
			}
		}
	}
}
