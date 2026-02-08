#pragma once

namespace PhotoMode
{
	struct OverlayData
	{
		void*                                  texture{ nullptr };
		std::shared_ptr<DirectX::ScratchImage> image{ nullptr };
	};

	class Overlays
	{
	public:
		void LoadOverlays();
		void RevertOverlays();

		OverlayData*                   UpdateOverlay();
		std::pair<OverlayData*, float> GetCurrentOverlay() const;

		void Draw();
		void DrawOverlays();

	private:
		// members
		struct FileIndex
		{
			const std::string& get_file()
			{
				return names[index];
			}

			// members
			std::vector<std::string> names;
			std::uint32_t            index;
		};

		FileIndex& GetFiles()
		{
			return folderFiles[folders.index];
		}

		// folder, file -> data
		StringMap<StringMap<OverlayData>> overlays{};
		OverlayData*                      cachedOverlay{ nullptr };
		bool                              updateOverlay{ false };
		bool                              hasOverlays{ false };

		FileIndex                     folders{};
		Map<std::uint32_t, FileIndex> folderFiles{};

		float alpha{ 1.0f };
	};
}
