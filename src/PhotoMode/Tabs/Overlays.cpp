#include "Overlays.h"

namespace PhotoMode
{
	void Overlays::LoadOverlays()
	{
		const std::filesystem::path overlaysPath(R"(Data\Interface\PhotoMode\Overlays)");

		std::error_code ec;
		if (!std::filesystem::exists(overlaysPath, ec)) {
			logger::info("Unable to load overlays ({})", ec.message());
			return;
		}

		std::map<std::string, std::vector<std::pair<std::string, std::string>>> imagePaths;

		std::string currentSubFolder;
		const auto  iterator = std::filesystem::recursive_directory_iterator(overlaysPath);
		for (const auto& entry : iterator) {
			if (entry.exists()) {
				if (entry.is_directory()) {
					currentSubFolder = entry.path().filename().string();
				} else if (entry.is_regular_file()) {
					if (const auto& path = entry.path(); !path.empty() && path.extension() == ".png") {
						auto fileName = path.filename().string();
						imagePaths[currentSubFolder].push_back({ path.string(), fileName.erase(fileName.size() - 4) });
					}
				}
			}
		}

		for (auto& [folder, files] : imagePaths) {
			for (auto& [path, fileName] : files) {
				// Load UI Texture
				void* tex = FUCK::LoadImage(path.c_str(), true);

				if (tex) {
					// Load CPU Image for screenshots
					auto scratch = std::make_shared<DirectX::ScratchImage>();

					// Use std::filesystem::path to handle UTF-8 -> Wide String conversion correctly on Windows
					std::filesystem::path fsPath(path);
					auto                  hr = DirectX::LoadFromWICFile(fsPath.c_str(), DirectX::WIC_FLAGS_NONE, nullptr, *scratch);

					if (SUCCEEDED(hr)) {
						overlays[folder].emplace(fileName, OverlayData{ tex, scratch });
					} else {
						// If we can't load the CPU image, release the UI texture and skip
						FUCK::ReleaseImage(tex);
					}
				}
			}
		}

		hasOverlays = !overlays.empty();

		if (hasOverlays) {
			std::uint32_t index = 0;

			for (auto& [folder, files] : imagePaths) {
				if (overlays.find(folder) == overlays.end())
					continue;

				folders.names.push_back(folder);

				folderFiles[index].names.push_back("$PM_NONE"_T);
				for (auto& [path, fileName] : files) {
					// Only add if it was successfully loaded
					if (overlays[folder].contains(fileName)) {
						folderFiles[index].names.push_back(fileName);
					}
				}

				index++;
			}
		}
	}

	void Overlays::RevertOverlays()
	{
		cachedOverlay = nullptr;
		updateOverlay = false;

		folders.index = 0;
		for (auto& files : folderFiles | std::views::values) {
			files.index = 0;
		}

		alpha = 1.0f;
	}

	OverlayData* Overlays::UpdateOverlay()
	{
		if (const auto it = overlays.find(folders.get_file()); it != overlays.end()) {
			const auto file = GetFiles().get_file();
			if (const auto fileIt = it->second.find(file); fileIt != it->second.end()) {
				return &fileIt->second;
			}
		}

		return nullptr;
	}

	std::pair<OverlayData*, float> Overlays::GetCurrentOverlay() const
	{
		return { cachedOverlay, alpha };
	}

	void Overlays::Draw()
	{
		if (!hasOverlays) {
			FUCK::TextUnformatted("$PM_NoOverlaysInstalled"_T);
		} else {
			static std::vector<std::string> categoryList;
			if (categoryList.empty()) {
				categoryList = folders.names;
			}

			std::uint8_t folderIdx = static_cast<std::uint8_t>(folders.index);
			if (FUCK::EnumStepper("$PM_Category"_T, &folderIdx, categoryList)) {
				folders.index = folderIdx;
				for (auto& [index, files] : folderFiles) {
					if (index != folders.index) {
						files.index = 0;
					}
				}
				cachedOverlay = nullptr;
				updateOverlay = false;
				alpha = 1.0f;
			}

			FUCK::Indent();
			{
				std::vector<std::string> overlayList = GetFiles().names;
				std::uint32_t            overlayIdx = GetFiles().index;

				if (FUCK::EnumStepper("$PM_Overlay"_T, &overlayIdx, overlayList)) {
					GetFiles().index = overlayIdx;
					updateOverlay = true;
					alpha = 1.0f;
				}
			}
			FUCK::Unindent();

			FUCK::SliderFloat("$PM_Intensity"_T, &alpha, 0.0f, 1.0f);
		}
	}

	void Overlays::DrawOverlays()
	{
		if (updateOverlay) {
			updateOverlay = false;
			cachedOverlay = UpdateOverlay();
		}

		if (cachedOverlay && cachedOverlay->texture) {
			FUCK::DrawBackgroundImage(cachedOverlay->texture, alpha);
		}
	}
}
