#include "Character.h"

namespace PhotoMode
{
	namespace MFG
	{
		void Data::Expression::ApplyExpression(RE::Actor* a_actor) const
		{
			if (const auto faceData = a_actor->GetFaceGenAnimationData()) {
				if (modifier == 0) {
					faceData->ClearExpressionOverride();
					faceData->Reset(0.0f, true, false, false, false);
					if (a_actor->IsPlayerRef()) {
						RE::BSFaceGenManager::GetSingleton()->isReset = true;
					}
				} else {
					faceData->exprOverride = false;
					faceData->SetExpressionOverride(modifier - 1, static_cast<float>(strength / 100.0f));
					faceData->exprOverride = true;
				}
			}
		}

		void Data::Modifier::ApplyModifier(std::uint32_t idx, RE::Actor* a_actor) const
		{
			if (const auto faceData = a_actor->GetFaceGenAnimationData()) {
				RE::BSSpinLockGuard locker(faceData->lock);
				faceData->modifierKeyFrame.SetValue(idx, static_cast<float>(strength / 100.0f));
			}
		}

		void Data::Modifier::ApplyPhenome(std::uint32_t idx, RE::Actor* a_actor) const
		{
			if (const auto faceData = a_actor->GetFaceGenAnimationData()) {
				RE::BSSpinLockGuard locker(faceData->lock);
				faceData->phenomeKeyFrame.SetValue(idx, static_cast<float>(strength / 100.0f));
			}
		}

		void Data::Revert(RE::Actor* a_actor)
		{
			if (const auto faceData = a_actor->GetFaceGenAnimationData()) {
				faceData->ClearExpressionOverride();
				faceData->Reset(0.0f, true, true, true, false);
				if (a_actor->IsPlayerRef()) {
					RE::BSFaceGenManager::GetSingleton()->isReset = true;
				}
			}

			expressionData.modifier = 0;
			expressionData.strength = 100;

			for (std::uint32_t i = 0; i < phonemes.size(); i++) {
				phonemeData[i].strength = 0;
			}
			for (std::uint32_t i = 0; i < modifiers.size(); i++) {
				modifierData[i].strength = 0;
			}
		}
	}

	void Character::State::Get(const RE::Actor* a_actor)
	{
		visible = a_actor->Get3D() ? !a_actor->Get3D()->GetAppCulled() : false;

		rotZ = a_actor->GetAngleZ();
		pos = a_actor->GetPosition();
	}

	void Character::RevertIdle() const
	{
		if (const auto currentProcess = character ? character->currentProcess : nullptr) {
			currentProcess->StopCurrentIdle(character, true);
			currentProcess->PlayIdle(character, resetRootIdle, nullptr);
		}
	}

	Character::Character(RE::Actor* a_actor) :
		character(a_actor)
	{
		if (a_actor->IsPlayerRef()) {
			characterName = TRANSLATE_S("$PM_Player");
		} else if (const auto actorbase = a_actor->GetActorBase(); actorbase && actorbase->IsUnique()) {
			characterName = a_actor->GetName();
		} else {
			characterName = std::format("{} [0x{:X}]", a_actor->GetName(), a_actor->GetFormID());
		}

		GetOriginalState();

		if (!character->IsPlayerRef()) {
			character->InitiateDoNothingPackage();
		}
	}

	void Character::GetOriginalState()
	{
		originalState.Get(character);
	}

	void Character::RevertState()
	{
		// revert current values
		currentState.pos = RE::NiPoint3();

		if (rotationChanged) {
			character->SetHeading(originalState.rotZ);
		}
		if (positionChanged) {
			character->SetPosition(originalState.pos, true);
		}
		if (positionChanged || rotationChanged) {
			auto charController = character->GetCharController();
			if (charController) {
				charController->flags.reset(RE::CHARACTER_FLAGS::kNoSim);
			}

			character->UpdateActor3DPosition();

			positionChanged = false;
			rotationChanged = false;
		}

		if (!currentState.visible) {
			if (const auto root = character->Get3D()) {
				root->CullGeometry(false);
			}
			currentState.visible = true;
		}

		// reset expressions
		if (mfgEdited) {
			mfgData.Revert(character);
			mfgEdited = false;
		}

		// revert idles
		if (idlePlayed) {
			RevertIdle();
			idlePlayed = false;
		}

		// revert effects
		if (vfxPlayed || effectsPlayed) {
			if (const auto processLists = RE::ProcessLists::GetSingleton()) {
				const auto handle = character->CreateRefHandle();
				processLists->ForEachMagicTempEffect([&](RE::BSTempEffect* a_effect) {
					if (const auto referenceEffect = a_effect->As<RE::ReferenceEffect>()) {
						if (referenceEffect->target == handle) {
							referenceEffect->finished = true;
						}
					}
					return RE::BSContainer::ForEachResult::kContinue;
				});
			}
			vfxPlayed = false;
			effectsPlayed = false;
		}

		if (!character->IsPlayerRef()) {
			character->EndInterruptPackage(false);
		}
	}

	const char* Character::GetName() const
	{
		return characterName.c_str();
	}

	void Character::Draw(bool a_resetTabs, bool a_navigateWithMouse)
	{
		if (a_resetTabs) {
			a_navigateWithMouse ? FUCK::SetItemDefaultFocus() : FUCK::SetKeyboardFocusHere();
		}

		bool visible = currentState.visible;
		if (FUCK::Checkbox(character->IsPlayerRef() ? "$PM_ShowPlayer"_T : "$PM_ShowCharacter"_T, &visible)) {
			currentState.visible = visible;
			if (const auto root = character->Get3D()) {
				root->CullGeometry(!currentState.visible);
			}
		}

		FUCK::Spacing();

		FUCK::BeginDisabled(!currentState.visible);
		{
			if (FUCK::BeginTabBar("Player#TopBar", 0)) {
				// ugly af, improve later
				const float width = FUCK::GetContentRegionAvail().x / 4;

				if (character->GetFaceGenAnimationData()) {
					FUCK::SetNextItemWidth(width);
					const int flags = a_resetTabs ? (1 << 0) : 0;  // ImGuiTabItemFlags_SetSelected = 1 << 0
					if (FUCK::BeginTabItem("$PM_Expressions"_T, flags)) {
						using namespace MFG;

						// Convert expressions array to vector for EnumStepper
						static std::vector<std::string> expressionList(expressions.begin(), expressions.end());

						std::uint8_t exprIdx = static_cast<std::uint8_t>(mfgData.expressionData.modifier);
						if (FUCK::EnumStepper("$PM_Expression"_T, &exprIdx, expressionList)) {
							mfgData.expressionData.modifier = exprIdx;
							if (mfgData.expressionData.strength > 0) {
								mfgData.expressionData.ApplyExpression(character);
								mfgEdited = true;
							}
						}

						FUCK::Indent();
						{
							FUCK::BeginDisabled(mfgData.expressionData.modifier == 0);
							{
								if (FUCK::SliderInt("$PM_Intensity"_T, &mfgData.expressionData.strength, 0, 100)) {
									mfgData.expressionData.ApplyExpression(character);
									mfgEdited = true;
								}
							}
							FUCK::EndDisabled();
						}
						FUCK::Unindent();

						FUCK::Spacing();

						if (FUCK::TreeNode("$PM_Phoneme"_T)) {
							for (std::uint32_t i = 0; i < phonemes.size(); i++) {
								if (FUCK::SliderInt(TRANSLATE(phonemes[i]), &mfgData.phonemeData[i].strength, 0, 100)) {
									mfgData.phonemeData[i].ApplyPhenome(i, character);
									mfgEdited = true;
								}
							}
							FUCK::TreePop();
						}

						if (FUCK::TreeNode("$PM_Modifier"_T)) {
							for (std::uint32_t i = 0; i < modifiers.size(); i++) {
								if (FUCK::SliderInt(TRANSLATE(modifiers[i]), &mfgData.modifierData[i].strength, 0, 100)) {
									mfgData.modifierData[i].ApplyModifier(i, character);
									mfgEdited = true;
								}
							}
							FUCK::TreePop();
						}
						FUCK::EndTabItem();
					}
				}

				FUCK::SetNextItemWidth(width);
				if (FUCK::BeginTabItem("$PM_Poses"_T)) {
					if (FUCK::ComboForm("$PM_Idles"_T, &selectedIdle, static_cast<std::uint8_t>(RE::FormType::Idle))) {
						if (auto idle = RE::TESForm::LookupByID<RE::TESIdleForm>(selectedIdle)) {
							if (idlePlayed) {
								RevertIdle();
								idlePlayed = false;
							}
							if (const auto currentProcess = character->currentProcess) {
								if (currentProcess->PlayIdle(character, idle, nullptr)) {
									idlePlayed = true;
								}
							}
						}
					}
					FUCK::EndTabItem();
				}

				FUCK::SetNextItemWidth(width);
				if (FUCK::BeginTabItem("$PM_Effects"_T)) {
					if (FUCK::ComboForm("$PM_EffectShaders"_T, &selectedEffectShader, static_cast<std::uint8_t>(RE::FormType::EffectShader))) {
						if (auto effectShader = RE::TESForm::LookupByID<RE::TESEffectShader>(selectedEffectShader)) {
							character->ApplyEffectShader(effectShader);
							effectsPlayed = true;
						}
					}

					if (FUCK::ComboForm("$PM_VisualEffects"_T, &selectedVFX, static_cast<std::uint8_t>(RE::FormType::ReferenceEffect))) {
						if (auto a_vfx = RE::TESForm::LookupByID<RE::BGSReferenceEffect>(selectedVFX)) {
							if (const auto effectShader = a_vfx->data.effectShader) {
								character->ApplyEffectShader(effectShader, -1, nullptr, a_vfx->data.flags.any(RE::BGSReferenceEffect::Flag::kFaceTarget), a_vfx->data.flags.any(RE::BGSReferenceEffect::Flag::kAttachToCamera));
							}
							if (const auto artObject = a_vfx->data.artObject) {
								character->ApplyArtObject(artObject, -1, nullptr, a_vfx->data.flags.any(RE::BGSReferenceEffect::Flag::kFaceTarget), a_vfx->data.flags.any(RE::BGSReferenceEffect::Flag::kAttachToCamera));
							}
							vfxPlayed = true;
						}
					}
					FUCK::EndTabItem();
				}

				FUCK::SetNextItemWidth(width);
				if (FUCK::BeginTabItem("$PM_Transforms"_T)) {
					currentState.rotZ = RE::rad_to_deg(character->GetAngleZ());
					if (FUCK::SliderFloat("$PM_Rotation"_T, &currentState.rotZ, 0.0f, 360.0f)) {
						character->SetHeading(RE::deg_to_rad(currentState.rotZ));
						rotationChanged = true;
					}

					bool update = FUCK::SliderFloat("$PM_PositionLeftRight"_T, &currentState.pos.x, -150.0f, 150.0f);
					update |= FUCK::SliderFloat("$PM_PositionNearFar"_T, &currentState.pos.y, -150.0f, 150.0f);
					update |= FUCK::SliderFloat("$PM_Elevation"_T, &currentState.pos.z, -150.0f, 150.0f);

					if (update) {
						auto charController = character->GetCharController();
						if (charController) {
							charController->flags.set(RE::CHARACTER_FLAGS::kNoSim);
						}
						character->SetPosition({ originalState.pos + currentState.pos }, true);
						positionChanged = true;
					}

					FUCK::EndTabItem();
				}
				FUCK::EndTabBar();
			}
		}
		FUCK::EndDisabled();
	}
}
