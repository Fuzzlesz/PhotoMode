#pragma once

namespace PhotoMode::Hotkeys
{
	class Manager : public REX::Singleton<Manager>
	{
	public:
		void LoadHotKeys(const CSimpleIniA& a_ini);

		// Legacy methods for compatibility (still used by some code)
		void TogglePhotoMode(RE::InputEvent* const* a_event);

		std::uint32_t        ResetKey() const;
		std::uint32_t        TakePhotoKey() const;
		std::uint32_t        ToggleMenusKey() const;
		std::uint32_t        NextTabKey() const;
		std::uint32_t        PreviousTabKey() const;
		std::uint32_t        FreezeTimeKey() const;
		std::uint32_t        PanCameraKey() const;
		static std::uint32_t EscapeKey();

		// ManagedHotkey accessors for FUCK_API integration
		FUCK::ManagedHotkey& GetToggleHotkey()		{ return _toggleHotkey; }
		FUCK::ManagedHotkey& GetScreenshotHotkey()	{ return _screenshotHotkey; }
		FUCK::ManagedHotkey& GetEscapeHotkey()		{ return _escapeHotkey; }
		FUCK::ManagedHotkey& GetToggleMenusHotkey() { return _toggleMenusHotkey; }
		FUCK::ManagedHotkey& GetNextTabHotkey()		{ return _nextTabHotkey; }
		FUCK::ManagedHotkey& GetPreviousTabHotkey() { return _previousTabHotkey; }
		FUCK::ManagedHotkey& GetFreezeTimeHotkey()	{ return _freezeTimeHotkey; }
		FUCK::ManagedHotkey& GetResetHotkey()		{ return _resetHotkey; }
		FUCK::ManagedHotkey& GetPanCameraHotkey()	{ return _panCameraHotkey; }

	private:
		// Legacy hotkey structures (kept for backward compatibility)
		struct Key
		{
			void          LoadKeys(const CSimpleIniA& a_ini, std::string_view a_setting);
			std::uint32_t GetKey() const;

			std::uint32_t Keyboard() const;
			std::uint32_t GamePad() const;

			// Allow Manager to set defaults directly
			friend class Manager;

		private:
			std::uint32_t keyboard{ 0 };
			std::uint32_t gamePad{ 0 };
		};

		struct KeyCombo
		{
			void LoadKeys(const CSimpleIniA& a_ini);

			bool                    IsInvalid() const;
			std::set<std::uint32_t> GetKeys() const;

			bool ProcessKeyPress(RE::InputEvent* const* a_event, std::function<void()> a_callback);

			struct KeyComboImpl
			{
				void LoadKeys(const CSimpleIniA& a_ini, std::string_view a_setting);

				std::int32_t primary{ -1 };
				std::int32_t modifier{ -1 };

				std::set<std::uint32_t> keys{};
			};

			KeyComboImpl keyboard;
			KeyComboImpl gamePad;

			bool triggered{ false };
		} togglePhotoMode;  // Legacy, now also synced to _toggleHotkey

		// Legacy hotkey keys
		Key nextTab;
		Key previousTab;
		Key takePhoto;
		Key toggleMenus;
		Key reset;
		Key freezeTime;
		Key panCamera;

		FUCK::ManagedHotkey _toggleHotkey		{ 0, 0, -1, -1, -1, -1 };
		FUCK::ManagedHotkey _screenshotHotkey	{ 0, 0, -1, -1, -1, -1 };
		FUCK::ManagedHotkey _escapeHotkey		{ 1, 0, -1, -1, -1, -1 };	// 1 = Esc
		FUCK::ManagedHotkey _toggleMenusHotkey	{ 0, 0, -1, -1, -1, -1 };
		FUCK::ManagedHotkey _nextTabHotkey		{ 0, 0, -1, -1, -1, -1 };
		FUCK::ManagedHotkey _previousTabHotkey	{ 0, 0, -1, -1, -1, -1 };
		FUCK::ManagedHotkey _freezeTimeHotkey	{ 0, 0, -1, -1, -1, -1 };
		FUCK::ManagedHotkey _resetHotkey		{ 0, 0, -1, -1, -1, -1 };
		FUCK::ManagedHotkey _panCameraHotkey	{ 0, 0, -1, -1, -1, -1 };
	};
}
namespace Hotkeys = PhotoMode::Hotkeys;
