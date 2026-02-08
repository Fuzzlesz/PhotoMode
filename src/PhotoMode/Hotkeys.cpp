#include "Hotkeys.h"
#include "Input.h"
#include "Manager.h"

namespace PhotoMode::Hotkeys
{
	void Manager::LoadHotKeys(const CSimpleIniA& a_ini)
	{
		togglePhotoMode.LoadKeys(a_ini);

		reset.LoadKeys(a_ini, "iReset");
		takePhoto.LoadKeys(a_ini, "iTakePhoto");
		toggleMenus.LoadKeys(a_ini, "iToggleMenus");
		nextTab.LoadKeys(a_ini, "iNextTab");
		previousTab.LoadKeys(a_ini, "iPreviousTab");
		freezeTime.LoadKeys(a_ini, "iFreezeTime");
		panCamera.LoadKeys(a_ini, "iPanCamera");

		// Sync to ManagedHotkey format

		// Toggle hotkey (from KeyCombo)
		if (!togglePhotoMode.keyboard.keys.empty()) {
			auto it = togglePhotoMode.keyboard.keys.begin();
			_toggleHotkey.kKey = *it;
			++it;
			_toggleHotkey.kMod1 = (it != togglePhotoMode.keyboard.keys.end()) ? static_cast<std::int32_t>(*it) : -1;
			++it;
			_toggleHotkey.kMod2 = (it != togglePhotoMode.keyboard.keys.end()) ? static_cast<std::int32_t>(*it) : -1;
		} else {
			_toggleHotkey.kKey = 0;
			_toggleHotkey.kMod1 = -1;
			_toggleHotkey.kMod2 = -1;
		}

		if (!togglePhotoMode.gamePad.keys.empty()) {
			auto it = togglePhotoMode.gamePad.keys.begin();
			_toggleHotkey.gKey = *it;
			++it;
			_toggleHotkey.gMod1 = (it != togglePhotoMode.gamePad.keys.end()) ? static_cast<std::int32_t>(*it) : -1;
			++it;
			_toggleHotkey.gMod2 = (it != togglePhotoMode.gamePad.keys.end()) ? static_cast<std::int32_t>(*it) : -1;
		} else {
			_toggleHotkey.gKey = 0;
			_toggleHotkey.gMod1 = -1;
			_toggleHotkey.gMod2 = -1;
		}

		// Sync Individual Hotkeys
		_screenshotHotkey.kKey = takePhoto.Keyboard();
		_screenshotHotkey.gKey = takePhoto.GamePad();

		_toggleMenusHotkey.kKey = toggleMenus.Keyboard();
		_toggleMenusHotkey.gKey = toggleMenus.GamePad();

		_nextTabHotkey.kKey = nextTab.Keyboard();
		_nextTabHotkey.gKey = nextTab.GamePad();

		_previousTabHotkey.kKey = previousTab.Keyboard();
		_previousTabHotkey.gKey = previousTab.GamePad();

		_freezeTimeHotkey.kKey = freezeTime.Keyboard();
		_freezeTimeHotkey.gKey = freezeTime.GamePad();

		_resetHotkey.kKey = reset.Keyboard();
		_resetHotkey.gKey = reset.GamePad();

		_panCameraHotkey.kKey = panCamera.Keyboard();
		_panCameraHotkey.gKey = panCamera.GamePad();

		logger::info("Hotkeys Loaded: Reset={}, TakePhoto={}, FreezeTime={}",
			reset.GetKey(), takePhoto.GetKey(), freezeTime.GetKey());
	}

	void Manager::TogglePhotoMode(RE::InputEvent* const* a_event)
	{
		if (togglePhotoMode.IsInvalid()) {
			return;
		}

		togglePhotoMode.ProcessKeyPress(a_event, []() {
			MANAGER(PhotoMode)->ToggleActive();
		});
	}

	void Manager::Key::LoadKeys(const CSimpleIniA& a_ini, std::string_view a_setting)
	{
		keyboard = a_ini.GetLongValue("Controls", std::format("{}Key", a_setting).c_str(), keyboard);
		gamePad = a_ini.GetLongValue("Controls", std::format("{}GamePad", a_setting).c_str(), gamePad);
	}

	std::uint32_t Manager::Key::GetKey() const
	{
		return MANAGER(Input)->IsInputKBM() ? keyboard : gamePad;
	}

	std::uint32_t Manager::Key::Keyboard() const
	{
		return keyboard;
	}

	std::uint32_t Manager::Key::GamePad() const
	{
		return gamePad;
	}

	void Manager::KeyCombo::KeyComboImpl::LoadKeys(const CSimpleIniA& a_ini, std::string_view a_setting)
	{
		primary = a_ini.GetLongValue("Controls", a_setting.data(), primary);
		modifier = a_ini.GetLongValue("Controls", std::format("{}Modifier", a_setting).c_str(), modifier);

		keys.clear();
		if (primary != -1) {
			keys.insert(primary);
		}
		if (modifier != -1) {
			keys.insert(modifier);
		}
	}

	void Manager::KeyCombo::LoadKeys(const CSimpleIniA& a_ini)
	{
		keyboard.LoadKeys(a_ini, "iOpenPhotoModeKey");
		gamePad.LoadKeys(a_ini, "iOpenPhotoModeGamePad");
	}

	bool Manager::KeyCombo::IsInvalid() const
	{
		return keyboard.keys.empty() && gamePad.keys.empty();
	}

	std::set<std::uint32_t> Manager::KeyCombo::GetKeys() const
	{
		auto device = MANAGER(Input)->GetInputDevice();
		return (device == Input::DEVICE::kKeyboard || device == Input::DEVICE::kMouse) ? keyboard.keys : gamePad.keys;
	}

	bool Manager::KeyCombo::ProcessKeyPress(RE::InputEvent* const* a_event, std::function<void()> a_callback)
	{
		std::set<std::uint32_t> pressed;

		for (auto event = *a_event; event; event = event->next) {
			const auto button = event->AsButtonEvent();
			if (!button || !button->HasIDCode()) {
				continue;
			}
			if (button->IsPressed()) {
				auto key = button->GetIDCode();
				switch (button->GetDevice()) {
				case RE::INPUT_DEVICE::kKeyboard:
					break;
				case RE::INPUT_DEVICE::kMouse:
					key += SKSE::InputMap::kMacro_MouseButtonOffset;
					break;
				case RE::INPUT_DEVICE::kGamepad:
					key = SKSE::InputMap::GamepadMaskToKeycode(key);
					break;
				default:
					continue;
				}
				pressed.insert(key);
			}
		}

		if (!pressed.empty() && (pressed == keyboard.keys || pressed == gamePad.keys)) {
			if (!triggered) {
				triggered = true;
				a_callback();
			}
		} else {
			triggered = false;
		}

		return triggered;
	}

	std::uint32_t Manager::ResetKey() const
	{
		return reset.GetKey();
	}

	std::uint32_t Manager::TakePhotoKey() const
	{
		return takePhoto.GetKey();
	}

	std::uint32_t Manager::ToggleMenusKey() const
	{
		return toggleMenus.GetKey();
	}

	std::uint32_t Manager::NextTabKey() const
	{
		return nextTab.GetKey();
	}

	std::uint32_t Manager::PreviousTabKey() const
	{
		return previousTab.GetKey();
	}

	std::uint32_t Manager::FreezeTimeKey() const
	{
		return freezeTime.GetKey();
	}

	std::uint32_t Manager::PanCameraKey() const
	{
		return panCamera.GetKey();
	}

	std::uint32_t Manager::EscapeKey()
	{
		return MANAGER(Input)->IsInputKBM() ? KEY::kEscape : SKSE::InputMap::kGamepadButtonOffset_B;
	}
}
