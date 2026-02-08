#pragma once

namespace PhotoMode
{
	class Filters
	{
	public:
		void GetOriginalState();
		void RevertState(bool a_fullReset);

		void Draw();

	private:
		// members
		RE::ImageSpaceBaseData     imageSpaceData{};
		RE::TESImageSpaceModifier* currentImod{ nullptr };
		bool                       imodPlayed{ false };
	};
}
