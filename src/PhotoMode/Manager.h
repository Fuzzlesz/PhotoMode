#pragma once
#include "IconsFontAwesome6.h"
#include "Tabs/Camera.h"
#include "Tabs/Character.h"
#include "Tabs/Filters.h"
#include "Tabs/Overlays.h"
#include "Tabs/Time.h"

namespace PhotoMode
{
	class Manager :
		public REX::Singleton<Manager>,
		public ITool,
		public RE::BSTEventSink<RE::MenuOpenCloseEvent>,
		public RE::BSTEventSink<SKSE::ModCallbackEvent>
	{
	public:
		// ==========================================
		// ITool Interface
		// ==========================================
		const char*		Name() const override { return "po3_PhotoMode"; }
		void			OnOpen() override { /* Called when PhotoMode activates */ }
		void			OnClose() override { /* Called when PhotoMode deactivates */ }
		void		    Draw() override { /* Not used - windows handle drawing */ }
		bool			OnAsyncInput(const void* inputEvent) override;
		virtual bool	ShowInSidebar() const { return false; }


		// ==========================================
		// Window Implementations
		// ==========================================
		class BackgroundWindow : public IWindow
		{
		public:
			BackgroundWindow(Manager* owner) :
				_owner(owner) {}

			const char* Title() const override { return "Background"; }
			void        Draw() override { _owner->DrawBackground(); }
			bool        IsOpen() const override { return _open; }
			void        SetOpen(bool a_open) override { _open = a_open; }
			WindowFlags	GetFlags() const override { return WindowFlags::kNoDecoration | WindowFlags::kNoBackground | WindowFlags::kPassInputToGame; }
			
			ImVec2 GetDefaultSize() const override { return FUCK::GetDisplaySize(); }
			ImVec2 GetDefaultPos() const override { return ImVec2(0, 0); }

			bool _open = false;

		private:
			Manager* _owner;
		};

		class ControlsWindow : public IWindow
		{
		public:
			ControlsWindow(Manager* owner) :
				_owner(owner) {}

			const char* Title() const override { return FUCK::Translate("$PM_Title_Menu"); }

			void Draw() override
			{
				FUCK::SetNextWindowPos(_pos, 8 /* ImGuiCond_Appearing */);
				FUCK::SetNextWindowSize(_size, 8 /* ImGuiCond_Appearing */);

				_owner->DrawControls();

				ImVec2 currentPos = FUCK::GetWindowPos();
				ImVec2 currentSize = FUCK::GetWindowSize();
			}

			bool		IsOpen() const override { return _open; }
			void		SetOpen(bool a_open) override { _open = a_open; }
			WindowFlags	GetFlags() const override{ return WindowFlags::kNoDecoration | WindowFlags::kCloseOnEsc; }

			ImVec2 GetDefaultPos() const override { return _pos; }
			ImVec2 GetDefaultSize() const override { return _size; }

			void UpdateState(const ImVec2& currentPos, const ImVec2& currentSize) override
			{
				_pos = currentPos;
				_size = currentSize;
			}

			bool   _open = false;
			ImVec2 _pos{ 1780.0f, 620.0f };
			ImVec2 _size{ 740.0f, 440.0f };

		private:
			Manager* _owner;
		};

		class BarWindow : public IWindow
		{
		public:
			BarWindow(Manager* owner) :
				_owner(owner) {}

			const char* Title() const override { return "Bar"; }

			void Draw() override
			{
				FUCK::SetNextWindowPos(_pos, 8 /* ImGuiCond_Appearing */);
				FUCK::SetNextWindowSize(_size, 8 /* ImGuiCond_Appearing */);

				_owner->DrawBar();

				ImVec2 currentPos = FUCK::GetWindowPos();
				ImVec2 currentSize = FUCK::GetWindowSize();
			}

			bool		IsOpen() const override { return _open; }
			void		SetOpen(bool a_open) override { _open = a_open; }
			WindowFlags	GetFlags() const override { return WindowFlags::kNoDecoration | WindowFlags::kCloseOnEsc;}

			ImVec2 GetDefaultPos() const override { return _pos; }
			ImVec2 GetDefaultSize() const override { return _size; }

			void UpdateState(const ImVec2& currentPos, const ImVec2& currentSize) override
			{
				_pos = currentPos;
				_size = currentSize;
			}

			bool   _open = false;
			ImVec2 _pos{ 780.0f, 1010.0f };
			ImVec2 _size{ 890.0f, 65.0f };

		private:
			Manager* _owner;
		};

		// ==========================================
		// Manager API
		// ==========================================

		void Register();
		void LoadMCMSettings(const CSimpleIniA& a_ini);

		bool IsValid();
		bool GetValidControlMapContext();

		bool               ShouldBlockInput() const;
		[[nodiscard]] bool IsActive() const;
		void               Activate();
		void               Deactivate();
		void               ToggleActive();
		void               Revert(bool a_deactivate = false);
		void               QuitOnEscape();

		bool GetResetAll() const;
		void DoResetAll();

		[[nodiscard]] bool IsHidden() const;
		void               ToggleUI();

		void NavigateTab(bool a_left);
		void UpdateKeyboardFocus();

		[[nodiscard]] float GetViewRoll(float a_fallback) const;
		[[nodiscard]] float GetViewRoll() const;
		void                SetViewRoll(float a_value);

		void TryOpenFromTweenMenu();

		void DrawBackground();
		void DrawControls();
		void DrawBar();

		bool OnFrameUpdate();

		void UpdateENBParams();
		void RevertENBParams();

		void                           OnDataLoad();
		std::pair<OverlayData*, float> GetOverlay() const;

		bool IsCursorHoveringOverWindow() const;

	private:
		enum TAB_TYPE : std::int8_t
		{
			kCamera,
			kTime,
			kCharacter,
			kFilters,
			kOverlays
		};

		// kMenu | kActivate | kJumping
		static constexpr auto controlFlags = static_cast<RE::ControlMap::UEFlag>(1036);

		static constexpr std::array tabs = {
			"$PM_Camera",
			"$PM_TimeWeather",
			"$PM_Player",
			"$PM_Filters",
			"$PM_Overlays"
		};
		static constexpr std::array tabIcons = {
			ICON_FA_CAMERA,
			ICON_FA_CLOCK,
			ICON_FA_PERSON,
			ICON_FA_CIRCLE_HALF_STROKE,
			ICON_FA_IMAGE
		};
		static constexpr std::array tabResetNotifs = { "$PM_ResetNotifCamera", "$PM_ResetNotifTime", "$PM_ResetNotifPlayer", "$PM_ResetNotifFilters", "$PM_ResetNotifOverlays" };

		static void        TogglePlayerControls(bool a_enable);
		[[nodiscard]] bool SetupJournalMenu() const;
		void               UpdateMouseHoveringOverWindow();

		EventResult ProcessEvent(const RE::MenuOpenCloseEvent* a_evn, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override;
		EventResult ProcessEvent(const SKSE::ModCallbackEvent* a_evn, RE::BSTEventSource<SKSE::ModCallbackEvent>*) override;

		// FUCK Windows
		BackgroundWindow m_backgroundWindow{ this };
		ControlsWindow   m_controlsWindow{ this };
		BarWindow        m_barWindow{ this };

		// members
		bool activated{ false };
		bool hiddenUI{ false };
		bool revertENB{ false };
		bool blockInputToPhotoMode{ false };

		std::int32_t                  previousTab{ kCamera };
		std::int32_t                  currentTab{ kCamera };
		std::array<bool, tabs.size()> hoveredTabs{};

		Camera cameraTab;
		Time   timeTab;

		Map<RE::FormID, Character> characterTab;
		RE::Actor*                 cachedCharacter{ nullptr };
		RE::Actor*                 prevCachedCharacter{ nullptr };

		Filters  filterTab;
		Overlays overlaysTab;

		bool updateKeyboardFocus{ false };

		RE::CameraState originalcameraState{ RE::CameraState::kThirdPerson };

		bool resetWindow{ true };
		bool resetPlayerTabs{ true };
		bool resetAll{ false };

		bool improvedCameraInstalled{ false };
		bool tweenMenuInstalled{ false };
		bool skyrimSoulsInstalled{ false };
		bool openFromTweenMenu{};

		bool menusAlreadyHidden{ false };
		bool allowTextInput{ false };

		bool noItemsFocused{ false };
		int  lastFocusedID{ 0 };
		int  lastHoveredID{ 0 };
		bool restoreLastFocusID{ false };

		float freeCameraSpeed{ 4.0f };
		bool  freezeTimeOnStart{ true };
		bool  openFromPauseMenu{ true };

		bool isCursorHoveringOverWindow{ false };

		RE::TESGlobal* activeGlobal{ nullptr };
	};
}
