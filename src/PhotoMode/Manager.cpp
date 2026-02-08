#include "Manager.h"

#include "Hotkeys.h"
#include "Input.h"
#include "Screenshots/Manager.h"

namespace PhotoMode
{
	// ==========================================
	// ITool Interface Implementation
	// ==========================================

	bool Manager::OnAsyncInput(const void* inputEvent)
	{
		auto hotkeys = MANAGER(PhotoMode::Hotkeys);

		// Always check toggle hotkey (works when inactive too)
		if (FUCK::UpdateManagedHotkey(inputEvent, hotkeys->GetToggleHotkey())) {
			return true;
		}

		if (FUCK::ProcessManagedHotkey(inputEvent, hotkeys->GetToggleHotkey())) {
			ToggleActive();
			return true;
		}

		// Only process other hotkeys when PhotoMode is active
		if (!IsActive()) {
			return false;
		}

		auto input = MANAGER(Input);

		// Screenshot hotkey
		if (FUCK::UpdateManagedHotkey(inputEvent, hotkeys->GetScreenshotHotkey())) {
			return true;
		}

		if (FUCK::ProcessManagedHotkey(inputEvent, hotkeys->GetScreenshotHotkey())) {
			input->QueueScreenshot(false);
			return true;
		}

		// Toggle UI visibility
		if (FUCK::UpdateManagedHotkey(inputEvent, hotkeys->GetToggleMenusHotkey())) {
			return true;
		}

		if (FUCK::ProcessManagedHotkey(inputEvent, hotkeys->GetToggleMenusHotkey())) {
			ToggleUI();
			return true;
		}

		// ESC to close (also handled by WindowFlags::kCloseOnEsc)
		if (FUCK::UpdateManagedHotkey(inputEvent, hotkeys->GetEscapeHotkey())) {
			return true;
		}

		if (FUCK::ProcessManagedHotkey(inputEvent, hotkeys->GetEscapeHotkey())) {
			QuitOnEscape();
			return true;
		}

		// Only process tab/control hotkeys when UI is visible
		if (!IsHidden()) {
			// Next Tab
			if (FUCK::UpdateManagedHotkey(inputEvent, hotkeys->GetNextTabHotkey())) {
				return true;
			}

			if (FUCK::ProcessManagedHotkey(inputEvent, hotkeys->GetNextTabHotkey())) {
				NavigateTab(false);
				return true;
			}

			// Previous Tab
			if (FUCK::UpdateManagedHotkey(inputEvent, hotkeys->GetPreviousTabHotkey())) {
				return true;
			}

			if (FUCK::ProcessManagedHotkey(inputEvent, hotkeys->GetPreviousTabHotkey())) {
				NavigateTab(true);
				return true;
			}

			// Freeze Time
			if (FUCK::UpdateManagedHotkey(inputEvent, hotkeys->GetFreezeTimeHotkey())) {
				return true;
			}

			if (FUCK::ProcessManagedHotkey(inputEvent, hotkeys->GetFreezeTimeHotkey())) {
				RE::Main::GetSingleton()->freezeTime = !RE::Main::GetSingleton()->freezeTime;
				return true;
			}

			// Reset
			if (FUCK::UpdateManagedHotkey(inputEvent, hotkeys->GetResetHotkey())) {
				return true;
			}

			if (FUCK::ProcessManagedHotkey(inputEvent, hotkeys->GetResetHotkey())) {
				Revert(false);
				return true;
			}
		}

		// Don't consume - let FUCK_API handle window input
		return false;
	}

	// ==========================================
	// Manager Implementation
	// ==========================================

	void Manager::Register()
	{
		tweenMenuInstalled = GetModuleHandle(L"TweenMenuOverhaul") != nullptr;
		improvedCameraInstalled = GetModuleHandle(L"ImprovedCameraSE.dll") != nullptr;
		skyrimSoulsInstalled = GetModuleHandle(L"SkyrimSoulsRE.dll") != nullptr;

		RE::UI::GetSingleton()->AddEventSink<RE::MenuOpenCloseEvent>(this);
		logger::info("Registered for menu open/close event");

		if (tweenMenuInstalled) {
			SKSE::GetModCallbackEventSource()->AddEventSink(this);
			logger::info("Registered for mod callback event");
		}

		// Register FUCK Windows
		FUCK::RegisterWindow(&m_backgroundWindow);
		FUCK::RegisterWindow(&m_controlsWindow);
		FUCK::RegisterWindow(&m_barWindow);
	}

	void Manager::LoadMCMSettings(const CSimpleIniA& a_ini)
	{
		freeCameraSpeed = static_cast<float>(a_ini.GetDoubleValue("Settings", "fFreeCameraTranslationSpeed", freeCameraSpeed));
		freezeTimeOnStart = a_ini.GetBoolValue("Settings", "bFreezeTimeOnStart", freezeTimeOnStart);
		openFromPauseMenu = a_ini.GetBoolValue("Settings", "bOpenFromPauseMenu", openFromPauseMenu);
	}

	bool Manager::IsValid()
	{
		static constexpr std::array badMenus{
			RE::MainMenu::MENU_NAME,
			RE::MistMenu::MENU_NAME,
			RE::LoadingMenu::MENU_NAME,
			RE::FaderMenu::MENU_NAME,
			"LootMenu"sv,
			"CustomMenu"sv
		};

		const auto UI = RE::UI::GetSingleton();
		if (!UI || std::ranges::any_of(badMenus, [&](const auto& menuName) { return UI->IsMenuOpen(menuName); })) {
			return false;
		}

		if (!GetValidControlMapContext() || RE::MenuControls::GetSingleton()->InBeastForm() || RE::VATS::GetSingleton()->mode == RE::VATS::VATS_MODE::kKillCam) {
			return false;
		}

		return true;
	}

	bool Manager::GetValidControlMapContext()
	{
		const auto* controlMap = RE::ControlMap::GetSingleton();
		if (!controlMap) {
			return false;
		}

		switch (controlMap->contextPriorityStack.back()) {
		case RE::UserEvents::INPUT_CONTEXT_ID::kGameplay:
		case RE::UserEvents::INPUT_CONTEXT_ID::kTFCMode:
		case RE::UserEvents::INPUT_CONTEXT_ID::kConsole:
		case RE::UserEvents::INPUT_CONTEXT_ID::kCursor:
			return true;
		default:
			return false;
		}
	}

	bool Manager::ShouldBlockInput() const
	{
		return blockInputToPhotoMode;
	}

	bool Manager::IsActive() const
	{
		return activated;
	}

	bool Manager::IsHidden() const
	{
		return hiddenUI;
	}

	void Manager::ToggleUI()
	{
		hiddenUI = !hiddenUI;

		// Toggle window visibility in FUCK system
		m_controlsWindow.SetOpen(!hiddenUI && activated);
		m_barWindow.SetOpen(!hiddenUI && activated);

		const auto UI = RE::UI::GetSingleton();
		UI->ShowMenus(!UI->IsShowingMenus());
		RE::PlaySound("UIMenuFocus");

		if (!hiddenUI) {
			restoreLastFocusID = true;
		}
	}

	void Manager::Activate()
	{
		RE::PlaySound("UIMenuOK");

		cameraTab.GetOriginalState();
		timeTab.GetOriginalState();

		const auto player = RE::PlayerCharacter::GetSingleton();
		characterTab.emplace(player->GetFormID(), Character(player));
		cachedCharacter = player;

		filterTab.GetOriginalState();

		const auto pcCamera = RE::PlayerCamera::GetSingleton();
		originalcameraState = pcCamera->currentState ? pcCamera->currentState->id : RE::CameraState::kThirdPerson;

		menusAlreadyHidden = !RE::UI::GetSingleton()->IsShowingMenus();
		if (menusAlreadyHidden) {
			hiddenUI = true;
		}

		// disable saving
		RE::PlayerCharacter::GetSingleton()->byCharGenFlag.set(RE::PlayerCharacter::ByCharGenFlag::kDisableSaving);

		// toggle freecam
		if (originalcameraState != RE::CameraState::kFree) {
			pcCamera->ToggleFreeCameraMode(false);
			//RE::ControlMap::GetSingleton()->PushInputContext(RE::ControlMap::InputContextID::kTFCMode);
		}

		// disable controls
		TogglePlayerControls(false);

		// apply mcm settings
		FreeCamera::translateSpeed = freeCameraSpeed;
		if (freezeTimeOnStart) {
			FUCK::SetGameTimeFrozen(true);
		}

		// load default screenshot keys
		// keybindings can change?
		MANAGER(Input)->LoadDefaultKeys();

		activated = true;

		// Open FUCK Windows
		m_backgroundWindow.SetOpen(true);
		m_controlsWindow.SetOpen(!hiddenUI);
		m_barWindow.SetOpen(!hiddenUI);

		if (activeGlobal) {
			activeGlobal->value = 1.0f;
		}
	}

	void Manager::TogglePlayerControls(bool a_enable)
	{
		RE::ControlMap::GetSingleton()->ToggleControls(controlFlags, a_enable);

		if (const auto pcControls = RE::PlayerControls::GetSingleton()) {
			pcControls->readyWeaponHandler->SetInputEventHandlingEnabled(a_enable);
			pcControls->sneakHandler->SetInputEventHandlingEnabled(a_enable);
			pcControls->autoMoveHandler->SetInputEventHandlingEnabled(a_enable);
			pcControls->shoutHandler->SetInputEventHandlingEnabled(a_enable);
			pcControls->attackBlockHandler->SetInputEventHandlingEnabled(a_enable);
		}
	}

	bool Manager::OnFrameUpdate()
	{
		if (!IsValid()) {
			Deactivate();
			return false;
		}

		// Check if text input is active via FUCK
		bool wantText = FUCK::IsAnyItemActive();

		if (wantText) {
			if (!allowTextInput) {
				allowTextInput = true;
				RE::ControlMap::GetSingleton()->AllowTextInput(true);
			}
		} else if (allowTextInput) {
			allowTextInput = false;
			RE::ControlMap::GetSingleton()->AllowTextInput(false);
		}
		TogglePlayerControls(false);

		timeTab.OnFrameUpdate();

		return true;
	}

	void Manager::Deactivate()
	{
		Revert(true);

		//reset characters
		characterTab.clear();
		cachedCharacter = nullptr;

		// reset camera
		if (originalcameraState != RE::CameraState::kFree) {
			RE::PlayerCamera::GetSingleton()->ToggleFreeCameraMode(false);
			//RE::ControlMap::GetSingleton()->PopInputContext(RE::ControlMap::InputContextID::kTFCMode);
		}

		// reset controls
		allowTextInput = false;
		RE::ControlMap::GetSingleton()->AllowTextInput(false);
		TogglePlayerControls(true);

		// allow saving
		RE::PlayerCharacter::GetSingleton()->byCharGenFlag.reset(RE::PlayerCharacter::ByCharGenFlag::kDisableSaving);

		// reset variables
		hiddenUI = false;

		noItemsFocused = false;
		restoreLastFocusID = false;
		lastFocusedID = 0;

		updateKeyboardFocus = false;

		// Reset FUCK cursor if managed
		FUCK::ForceCursor(false);
		FUCK::SetGameTimeFrozen(false);

		// Close FUCK Windows
		m_backgroundWindow.SetOpen(false);
		m_controlsWindow.SetOpen(false);
		m_barWindow.SetOpen(false);

		activated = false;
		if (activeGlobal) {
			activeGlobal->value = 0.0f;
		}

		RE::PlaySound("UIMenuCancel");
	}

	void Manager::ToggleActive()
	{
		if (!IsActive()) {
			if (IsValid() && !ShouldBlockInput()) {
				Activate();
			}
		} else {
			bool wantText = FUCK::IsAnyItemActive();
			if (!wantText && !ShouldBlockInput()) {
				Deactivate();
			}
		}
	}

	void Manager::Revert(bool a_deactivate)
	{
		const std::int32_t tabIndex = a_deactivate ? -1 : currentTab;

		// Camera
		if (tabIndex == -1 || tabIndex == kCamera) {
			cameraTab.RevertState(a_deactivate);
			if (!a_deactivate) {
				FreeCamera::translateSpeed = freeCameraSpeed;
			}
			revertENB = true;
		}
		// Time/Weather
		if (tabIndex == -1 || tabIndex == kTime) {
			timeTab.RevertState();
		}

		// Character
		if (tabIndex == kCharacter) {
			if (cachedCharacter) {
				characterTab[cachedCharacter->GetFormID()].RevertState();
			}
		} else if (tabIndex == -1) {
			std::ranges::for_each(characterTab, [](auto& data) {
				data.second.RevertState();
			});
		}

		// Filters
		if (tabIndex == -1 || tabIndex == kFilters) {
			filterTab.RevertState(tabIndex == -1);
		}
		// Overlays
		if (tabIndex == -1 || tabIndex == kOverlays) {
			overlaysTab.RevertOverlays();
		}

		if (a_deactivate) {
			// reset UI
			if ((!menusAlreadyHidden || hiddenUI) && !RE::UI::GetSingleton()->IsShowingMenus()) {
				RE::UI::GetSingleton()->ShowMenus(true);
			}
			resetWindow = true;
			resetPlayerTabs = true;
		} else {
			RE::PlaySound("UIMenuOK");

			const auto notification = std::format("{}", resetAll ? "$PM_ResetNotifAll"_T : TRANSLATE(tabResetNotifs[currentTab]));
			RE::DebugNotification(notification.c_str());

			if (resetAll) {
				resetAll = false;
			}
		}
	}

	void Manager::QuitOnEscape()
	{
		if (IsHidden() || noItemsFocused) {
			Deactivate();
			RE::PlaySound("UIMenuCancel");
		}
	}

	bool Manager::GetResetAll() const
	{
		return resetAll;
	}

	void Manager::DoResetAll()
	{
		resetAll = true;
	}

	void Manager::NavigateTab(bool a_left)
	{
		const auto tabsSizeInt32 = tabs.size() > std::numeric_limits<uint32_t>::max() ?
		                               std::numeric_limits<uint32_t>::max() :
		                               static_cast<uint32_t>(tabs.size());
		if (a_left) {
			currentTab = (currentTab - static_cast<uint32_t>(1) + tabsSizeInt32) % tabsSizeInt32;
		} else {
			currentTab = (currentTab + static_cast<uint32_t>(1)) % tabsSizeInt32;
		}
		UpdateKeyboardFocus();
		RE::PlaySound("UIJournalTabsSD");
	}

	void Manager::UpdateKeyboardFocus()
	{
		updateKeyboardFocus = true;
	}

	float Manager::GetViewRoll(const float a_fallback) const
	{
		return IsActive() ? cameraTab.GetViewRoll() : a_fallback;
	}

	float Manager::GetViewRoll() const
	{
		return cameraTab.GetViewRoll();
	}

	void Manager::SetViewRoll(float a_value)
	{
		cameraTab.SetViewRoll(a_value);
	}

	void Manager::TryOpenFromTweenMenu()
	{
		if (openFromTweenMenu) {
			SKSE::GetTaskInterface()->AddTask([this]() {
				this->Activate();
				this->openFromTweenMenu = false;
			});
		}
	}

	void Manager::UpdateENBParams()
	{
		if (IsActive()) {
			cameraTab.UpdateENBParams();
		}
	}

	void Manager::RevertENBParams()
	{
		if (revertENB) {
			cameraTab.RevertENBParams();
			revertENB = false;
		}
	}

	void Manager::OnDataLoad()
	{
		overlaysTab.LoadOverlays();

		activeGlobal = RE::TESForm::LookupByEditorID<RE::TESGlobal>("PhotoMode_IsActive");
		resetRootIdle = RE::TESForm::LookupByEditorID<RE::TESIdleForm>("ResetRoot");
	}

	std::pair<OverlayData*, float> Manager::GetOverlay() const
	{
		return overlaysTab.GetCurrentOverlay();
	}

	bool Manager::IsCursorHoveringOverWindow() const
	{
		return isCursorHoveringOverWindow;
	}

	void Manager::DrawBackground()
	{
		if (!OnFrameUpdate()) {
			return;
		}

		// Render hierarchy for background items
		overlaysTab.DrawOverlays();

		if (!IsHidden()) {
			CameraGrid::Draw();
		}
	}

	void Manager::DrawControls()
	{
		FUCK::ExtendWindowPastBorder();

		if (resetWindow) {
			currentTab = kCamera;
		}

		// console already covers menu
		if (blockInputToPhotoMode) {
			FUCK::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 0.6f);
		}

		FUCK::BeginDisabled(blockInputToPhotoMode);
		{
			// Q [Tab Tab Tab Tab Tab] E
			FUCK::BeginGroup();
			{
				const auto buttonSize = ImVec2(0, 0);

				ImVec2        iconSize;
				std::uint32_t prevKey = MANAGER(Hotkeys)->PreviousTabKey();
				if (prevKey == 0)
					prevKey = 16;  // Q
				void* prevIcon = FUCK::GetIconForKey(prevKey, &iconSize);
				if (iconSize.x <= 0)
					iconSize = ImVec2(32, 32);

				FUCK::ButtonIconWithLabel("##Prev", prevIcon, iconSize, false, false);
				FUCK::SameLine();

				float availW = FUCK::GetContentRegionAvail().x;

				float btnWidth = FUCK::GetFrameHeightWithSpacing();
				float totalBtnWidth = btnWidth * 3.0f;

				const float tabWidth = (availW > totalBtnWidth) ? ((availW - totalBtnWidth) / tabs.size()) : 40.0f;

				FUCK::PushItemFlag(ItemFlags::kNoNav, true);
				FUCK::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
				FUCK::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
				FUCK::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
				FUCK::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.5f, 0.5f));

				for (std::int32_t i = 0; i < tabs.size(); ++i) {
					bool activeTab = (currentTab == i) || hoveredTabs[i] == true;
					if (!activeTab) {
						FUCK::PushStyleColor(ImGuiCol_Text, FUCK::GetStyleColorVec4(ImGuiCol_TextDisabled));
					} else {
						FUCK::PushFont(FUCK::GetFont(FUCK_Font::kLarge));
					}

					if (FUCK::Selectable(tabIcons[i], currentTab == i, 0, ImVec2(tabWidth, FUCK::GetFrameHeightWithSpacing()))) {
						currentTab = i;
					}

					hoveredTabs[i] = FUCK::IsItemHovered();
					if (!activeTab) {
						FUCK::PopStyleColor();
					} else {
						FUCK::PopFont();
					}
					FUCK::SameLine();
				}
				FUCK::PopStyleVar();
				FUCK::PopStyleColor(3);
				FUCK::PopItemFlag();

				std::uint32_t nextKey = MANAGER(Hotkeys)->NextTabKey();
				if (nextKey == 0)
					nextKey = 18;  // E
				void* nextIcon = FUCK::GetIconForKey(nextKey, &iconSize);
				if (iconSize.x <= 0)
					iconSize = ImVec2(32, 32);

				FUCK::ButtonIconWithLabel("##Next", nextIcon, iconSize, false, false);
			}
			FUCK::EndGroup();

			//		CAMERA
			// ----------------
			FUCK::CenteredText(currentTab != TAB_TYPE::kCharacter ? FUCK::Translate(tabs[currentTab]) : characterTab[cachedCharacter->GetFormID()].GetName());
			FUCK::SeparatorThick();

			// content
			FUCK::BeginChild("##PhotoModeChild", ImVec2(0, 0), false, 0);
			{
				FUCK::Spacing();

				if (restoreLastFocusID) {
					restoreLastFocusID = false;
				} else if (updateKeyboardFocus) {
					if (currentTab == TAB_TYPE::kCharacter) {
						resetPlayerTabs = true;
					}
					FUCK::SetItemDefaultFocus();
					updateKeyboardFocus = false;
				}

				switch (currentTab) {
				case TAB_TYPE::kCamera:
					{
						if (resetWindow) {
							FUCK::SetItemDefaultFocus();
							resetWindow = false;
						}
						cameraTab.Draw();
					}
					break;
				case TAB_TYPE::kTime:
					timeTab.Draw();
					break;
				case TAB_TYPE::kCharacter:
					{
						const auto consoleRef = RE::Console::GetSelectedRef();
						if (!consoleRef || !consoleRef->Is(RE::FormType::ActorCharacter) || consoleRef->IsDisabled() || consoleRef->IsDeleted() || !consoleRef->Is3DLoaded()) {
							prevCachedCharacter = cachedCharacter;
							cachedCharacter = RE::PlayerCharacter::GetSingleton();
						} else {
							prevCachedCharacter = cachedCharacter;
							cachedCharacter = consoleRef->As<RE::Actor>();
							if (!characterTab.contains(cachedCharacter->GetFormID())) {
								characterTab.emplace(cachedCharacter->GetFormID(), Character(cachedCharacter));
							}
						}

						if (cachedCharacter != prevCachedCharacter) {
							resetPlayerTabs = true;
						}

						characterTab[cachedCharacter->GetFormID()].Draw(resetPlayerTabs, true);

						if (resetPlayerTabs) {
							resetPlayerTabs = false;
						}
					}
					break;
				case TAB_TYPE::kFilters:
					filterTab.Draw();
					break;
				case TAB_TYPE::kOverlays:
					overlaysTab.Draw();
					break;
				default:
					break;
				}

				noItemsFocused = !FUCK::IsAnyItemActive();
			}
			FUCK::EndChild();
		}
		FUCK::EndDisabled();

		if (blockInputToPhotoMode) {
			FUCK::PopStyleVar();
		}
	}

	void Manager::DrawBar()
	{
		FUCK::ExtendWindowPastBorder();
		auto   hotkeys = MANAGER(Hotkeys);
		ImVec2 iconSize;

		FUCK::BeginGroup();
		{
			// 1. Screenshot
			std::uint32_t key = hotkeys->TakePhotoKey();
			void*         icon = FUCK::GetIconForKey(key, &iconSize);
			if (FUCK::ButtonIconWithLabel("$PM_TAKEPHOTO"_T, icon, iconSize, false, false)) {
				MANAGER(Input)->QueueScreenshot(false);
			}

			FUCK::SameLine();

			// 2. Hide UI
			key = hotkeys->ToggleMenusKey();
			icon = FUCK::GetIconForKey(key, &iconSize);
			if (FUCK::ButtonIconWithLabel("$PM_TOGGLEMENUS"_T, icon, iconSize, false, false)) {
				ToggleUI();
			}

			FUCK::SameLine();

			// 3. Reset / Reset All
			key = hotkeys->ResetKey();
			icon = FUCK::GetIconForKey(key, &iconSize);
			const char* label = GetResetAll() ? "$PM_RESET_ALL"_T : "$PM_RESET"_T;
			FUCK::ButtonIconWithLabel(label, icon, iconSize, false, false);

			FUCK::SameLine();

			// 4. Freeze Time
			key = hotkeys->FreezeTimeKey();
			icon = FUCK::GetIconForKey(key, &iconSize);
			if (FUCK::ButtonIconWithLabel("$PM_FREEZETIME"_T, icon, iconSize, false, false)) {
				FUCK::SetSoftPause;
			}

			FUCK::SameLine();

			// 5. Pan Camera
			key = hotkeys->PanCameraKey();
			icon = FUCK::GetIconForKey(key, &iconSize);
			FUCK::ButtonIconWithLabel("$PM_PAN_CAMERA"_T, icon, iconSize, false, false);

			FUCK::SameLine();

			// 6. Exit (ESC)
			key = Hotkeys::Manager::EscapeKey();
			icon = FUCK::GetIconForKey(key, &iconSize);
			if (FUCK::ButtonIconWithLabel("$PM_EXIT"_T, icon, iconSize, false, false)) {
				Deactivate();
			}
		}
		FUCK::EndGroup();
	}

	bool Manager::SetupJournalMenu() const
	{
		const auto menu = RE::UI::GetSingleton()->GetMenu<RE::JournalMenu>(RE::JournalMenu::MENU_NAME);
		const auto view = menu ? menu->systemTab.view : nullptr;

		RE::GFxValue page;
		if (!view || !view->GetVariable(&page, "_root.QuestJournalFader.Menu_mc.SystemFader.Page_mc")) {
			return false;
		}

		// in case someone packed the files into a BSA
		static bool dearDiaryExists = RE::BSResourceNiBinaryStream(R"(interface\deardiary_dm\config.txt)").good() || RE::BSResourceNiBinaryStream(R"(interface\deardiary\config.txt)").good();

		// Dear Diary SetShowMod function is broken af, need to do it manually
		if (dearDiaryExists) {
			RE::GFxValue categoryList;
			if (page.GetMember("CategoryList", &categoryList)) {
				RE::GFxValue entryList;
				if (categoryList.GetMember("entryList", &entryList)) {
					std::vector<std::string> elements;

					entryList.VisitMembers([&](const char*, const RE::GFxValue& a_value) {
						RE::GFxValue textVal;
						a_value.GetMember("text", &textVal);
						elements.push_back(textVal.GetString());
					});

					RE::GFxValue showModMenu;
					if (page.GetMember("_showModMenu", &showModMenu) && !showModMenu.GetBool()) {
						page.SetMember("_showModMenu", true);
					} else {
						std::erase(elements, "$MOD MANAGER");
					}

					auto index = std::ranges::contains(elements, "$QUICKSAVE") ? 3 : 2;
					elements.insert(elements.begin() + index, "$PM_Title_Menu");

					entryList.ClearElements();
					for (auto& element : elements) {
						RE::GFxValue entry;
						view->CreateObject(&entry);
						entry.SetMember("text", element.c_str());
						entryList.PushBack(entry);
					}

					categoryList.Invoke("InvalidateData");

					return true;
				}
			}

		} else {
			RE::GFxValue showModMenu;
			if (page.GetMember("_showModMenu", &showModMenu) && !showModMenu.GetBool()) {
				std::array<RE::GFxValue, 1> args;
				args[0] = true;
				if (!page.Invoke("SetShowMod", nullptr, args.data(), args.size())) {
					return false;
				}
			}

			RE::GFxValue categoryList;
			if (page.GetMember("CategoryList", &categoryList)) {
				RE::GFxValue entryList;
				if (categoryList.GetMember("entryList", &entryList)) {
					std::optional<std::uint32_t> modMenuIndex = std::nullopt;

					std::uint32_t index = 0;
					std::string   text;
					entryList.VisitMembers([&](const char*, const RE::GFxValue& a_value) {
						RE::GFxValue textVal;
						a_value.GetMember("text", &textVal);
						if (text = textVal.GetString(); text == "$MOD MANAGER") {
							modMenuIndex = index;
						}
						index++;
					});

					if (modMenuIndex) {
						RE::GFxValue entry;
						view->CreateObject(&entry);
						entry.SetMember("text", "$PM_Title_Menu");

						entryList.SetElement(*modMenuIndex, entry);
						categoryList.Invoke("InvalidateData");

						return true;
					}
				}
			}
		}

		return false;
	}

	void Manager::UpdateMouseHoveringOverWindow()
	{
		constexpr float buffer = 50.0f;
		auto            mousePos = FUCK::GetMousePos();
		auto            winPos = FUCK::GetWindowPos();
		auto            winSize = FUCK::GetWindowSize();

		isCursorHoveringOverWindow =
			mousePos.x >= winPos.x - buffer &&
			mousePos.x <= winPos.x + winSize.x + buffer &&
			mousePos.y >= winPos.y - buffer &&
			mousePos.y <= winPos.y + winSize.y + buffer;
	}

	EventResult Manager::ProcessEvent(const RE::MenuOpenCloseEvent* a_evn, RE::BSTEventSource<RE::MenuOpenCloseEvent>*)
	{
		if (!a_evn) {
			return EventResult::kContinue;
		}

		if (a_evn->menuName == RE::Console::MENU_NAME) {
			blockInputToPhotoMode = a_evn->opening;
			if (a_evn->opening) {
				if (IsActive() && IsHidden()) {
					ToggleUI();
				}
			} else if (IsActive()) {
				FUCK::ForceCursor(true);
			}
		} else if (a_evn->menuName == RE::TweenMenu::MENU_NAME) {
			if (!a_evn->opening) {
				TryOpenFromTweenMenu();
			}
		} else if (a_evn->opening) {
			if (a_evn->menuName == RE::JournalMenu::MENU_NAME) {
				if (openFromPauseMenu) {
					openFromPauseMenu = SetupJournalMenu();
				}
			} else if (a_evn->menuName == RE::ModManagerMenu::MENU_NAME) {
				if (RE::UI::GetSingleton()->IsMenuOpen(RE::JournalMenu::MENU_NAME) && openFromPauseMenu) {
					const auto msgQueue = RE::UIMessageQueue::GetSingleton();

					msgQueue->AddMessage(RE::ModManagerMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kHide, nullptr);
					msgQueue->AddMessage(RE::JournalMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kHide, nullptr);

					Activate();
				}
			}
		}

		return EventResult::kContinue;
	}

	EventResult Manager::ProcessEvent(const SKSE::ModCallbackEvent* a_evn, RE::BSTEventSource<SKSE::ModCallbackEvent>*)
	{
		if (a_evn && a_evn->eventName == "OpenTween_PhotoMode") {
			openFromTweenMenu = true;
			if (skyrimSoulsInstalled) {
				Activate();
				openFromTweenMenu = false;
			}
		}

		return EventResult::kContinue;
	}
}
