#include "Camera.h"

#include "ENB/ENB.h"

namespace PhotoMode
{
	void Camera::ENBDOF::Get()
	{
		enabled = ENB::GetParameter<bool>("enbseries.ini", "EFFECT", "EnableDepthOfField");
	}

	void Camera::ENBDOF::SetParameter(std::uint32_t a_type) const
	{
		if ((a_type & kEnable) != 0) {
			ENB::SetParameter(enabled, "enbseries.ini", "EFFECT", "EnableDepthOfField");
		}
	}

	void Camera::OriginalState::Get()
	{
		fov = RE::PlayerCamera::GetSingleton()->worldFOV;
		translateSpeed = FreeCamera::translateSpeed;

		vanillaDOF.blurMultiplier = DOF::blurMultiplier;
		vanillaDOF.nearDist = DOF::nearDist;
		vanillaDOF.nearRange = DOF::nearRange;
		vanillaDOF.farDist = DOF::farDist;
		vanillaDOF.farRange = DOF::farRange;

		/*if (ENB::IsInstalled()) {
			enbDOF.Get();
		}*/
	}

	void Camera::OriginalState::Revert(bool a_deactivate) const
	{
		RE::PlayerCamera::GetSingleton()->worldFOV = fov;

		DOF::blurMultiplier = vanillaDOF.blurMultiplier;
		DOF::nearDist = vanillaDOF.nearDist;
		DOF::nearRange = vanillaDOF.nearRange;
		DOF::farDist = vanillaDOF.farDist;
		DOF::farRange = vanillaDOF.farRange;

		if (a_deactivate) {
			FreeCamera::translateSpeed = translateSpeed;
		}
	}

	void Camera::GetOriginalState()
	{
		// revertENB = false;
		originalState.Get();
	}

	void Camera::RevertState(bool a_deactivate)
	{
		originalState.Revert(a_deactivate);

		// revert view roll
		currentViewRoll = 0.0f;

		// revert grid
		CameraGrid::gridType = CameraGrid::GridType::kDisabled;

		// revert DOF
		if (const auto& effect = RE::ImageSpaceManager::GetSingleton()->effects[RE::ImageSpaceManager::ImageSpaceEffectEnum::DepthOfField]) {
			static_cast<RE::ImageSpaceEffectDepthOfField*>(effect)->enabled = true;
		}

		/*if (ENB::IsInstalled()) {
			revertENB = true;
		}*/
	}

	void Camera::Draw()
	{
		// Convert gridTypes array to vector for EnumStepper
		static std::vector<std::string> gridTypesList(CameraGrid::gridTypes.begin(), CameraGrid::gridTypes.end());
		if (FUCK::EnumStepper("$PM_Grid"_T, &CameraGrid::gridType, gridTypesList)) {
			// Reset defaults on change
			CameraGrid::gridRows = 3;
			CameraGrid::gridCols = 3;
			CameraGrid::gridRotation = 0.0f;
			CameraGrid::spiralScale = 1.0f;
			CameraGrid::spiralTurns = 6.0f;
		}

		// Grid Configuration based on selected type
		if (CameraGrid::gridType != CameraGrid::GridType::kDisabled) {
			FUCK::Indent();

			// Color config
			FUCK::ColorEdit3("$PM_GridColor"_T, CameraGrid::overlayColor, 0);
			FUCK::SliderFloat("$PM_GridThickness"_T, &CameraGrid::overlayThickness, 0.5f, 10.0f, "%.1f px");

			switch (CameraGrid::gridType) {
			case CameraGrid::GridType::kGrid:
			case CameraGrid::GridType::kRuleOfThirds:  // Allow customization for RoT too
				FUCK::SliderInt("$PM_GridRows"_T, &CameraGrid::gridRows, 0, 50);
				FUCK::SliderInt("$PM_GridCols"_T, &CameraGrid::gridCols, 0, 50);
				FUCK::SliderFloat("$PM_Rotation"_T, &CameraGrid::gridRotation, -45.0f, 45.0f, "%.0f deg");
				break;

			case CameraGrid::GridType::kGoldenSpiral:
				{
					const char* basicAnchors[] = {
						"$PM_Anchor_BR"_T, "$PM_Anchor_BL"_T,
						"$PM_Anchor_TL"_T, "$PM_Anchor_TR"_T,
						"$PM_Anchor_Center"_T
					};
					FUCK::Combo("$PM_SpiralAnchor"_T, &CameraGrid::spiralAnchor, basicAnchors, 5);
					FUCK::Checkbox("$PM_ShowSquares"_T, &CameraGrid::showSquares);
					FUCK::SliderFloat("$PM_SpiralScale"_T, &CameraGrid::spiralScale, 0.1f, 5.0f);
					FUCK::SliderFloat("$PM_Rotation"_T, &CameraGrid::gridRotation, -180.0f, 180.0f);
					FUCK::SliderFloat("$PM_SpiralTurns"_T, &CameraGrid::spiralTurns, 1.0f, 20.0f);
				}
				break;
			case CameraGrid::GridType::kTriangle:
				FUCK::Checkbox("$PM_Mirror"_T, &CameraGrid::triMirror);
				break;
			case CameraGrid::GridType::kGoldenRatio:
				FUCK::SliderInt("$PM_Subdivisions"_T, &CameraGrid::subDivs, 0, 3);
				break;
			default:
				break;
			}
			FUCK::Unindent();
		}

		FUCK::SliderFloat("$PM_FieldOfView"_T, &RE::PlayerCamera::GetSingleton()->worldFOV, 5.0f, 150.0f);

		currentViewRollDegrees = RE::rad_to_deg(currentViewRoll);
		if (FUCK::SliderFloat("$PM_ViewRoll"_T, &currentViewRollDegrees, -90.0f, 90.0f)) {
			currentViewRoll = RE::deg_to_rad(currentViewRollDegrees);
		}

		FUCK::SliderFloat("$PM_TranslateSpeed"_T,
			&FreeCamera::translateSpeed,  // fFreeCameraTranslationSpeed:Camera
			0.1f, 50.0f);

		/*if (ENB::IsEnabled()) {
			lastDOF = curDOF;
			curDOF.Get();

			ImGui::CheckBox("$PM_DepthOfField"_T, &curDOF.enabled);

		else 

		const auto& effect = RE::ImageSpaceManager::GetSingleton()->effects[RE::ImageSpaceManager::ImageSpaceEffectEnum::DepthOfField];

		if (effect && !ENB::IsEnabled()) {
			const auto dofEffect = static_cast<RE::ImageSpaceEffectDepthOfField*>(effect);

			ImGui::CheckBox("$PM_DepthOfField"_T, &dofEffect->enabled);

			ImGui::BeginDisabled(!dofEffect->enabled);
			{
				ImGui::Indent();
				{
					ImGui::Slider("$PM_DOF_Strength"_T, &DOF::blurMultiplier, 0.0f, 1.0f);
					ImGui::Slider("$PM_DOF_Distance"_T, &DOF::nearDist, 0.0f, 1000.0f);
					ImGui::Slider("$PM_DOF_Range"_T, &DOF::nearRange, 0.0f, 1000.0f);
				}
				ImGui::Unindent();
			}
			ImGui::EndDisabled();
		}*/

		// Camera position management
		cameraPositions.Draw();
	}

	void Camera::UpdateENBParams()
	{
		if (curDOF.enabled != lastDOF.enabled) {
			curDOF.SetParameter(ENBDOF::kEnable);
			lastDOF.enabled = curDOF.enabled;
		}
	}

	void Camera::RevertENBParams()
	{
		if (revertENB) {
			originalState.enbDOF.SetParameter(ENBDOF::kAll);
			revertENB = false;
		}
	}

	void CameraGrid::Draw()
	{
		if (gridType == kDisabled) {
			return;
		}

		// Convert float[4] to U32 for API
		ImU32 col = ImGui::ColorConvertFloat4ToU32(ImVec4(overlayColor[0], overlayColor[1], overlayColor[2], overlayColor[3]));

		switch (gridType) {
		case kRuleOfThirds:
			// Rule of thirds is just a 3x3 grid
			FUCK::DrawOverlay(FUCK_Overlay::kGrid, overlayThickness, col, 3.0f, 3.0f, gridRotation, 0.0f);
			break;
		case kDiagonal:
			{
				//TODO: REIMPLEMENT SOMEWEHRE
			}
			break;
		case kTriangle:
			FUCK::DrawOverlay(FUCK_Overlay::kTriangle, overlayThickness, col, triMirror ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f);
			break;
		case kGoldenRatio:
			FUCK::DrawOverlay(FUCK_Overlay::kGoldenRatio, overlayThickness, col, static_cast<float>(subDivs), 0.0f, 0.0f, 0.0f);
			break;
		case kGoldenSpiral:
			// paramA = anchor + showSquares offset
			{
				float pA = static_cast<float>(spiralAnchor) + (showSquares ? 10.0f : 0.0f);
				FUCK::DrawOverlay(FUCK_Overlay::kGoldenSpiral, overlayThickness, col, pA, spiralTurns, gridRotation, spiralScale);
			}
			break;
		case kGrid:
			FUCK::DrawOverlay(FUCK_Overlay::kGrid, overlayThickness, col, (float)gridRows, (float)gridCols, gridRotation, 0.0f);
			break;
		default:
			break;
		}
	}
}
