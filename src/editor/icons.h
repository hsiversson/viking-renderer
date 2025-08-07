#pragma once

#if ENABLE_EDITOR
namespace vkr::Render
{
	class TextureView;
}

namespace vkr::Editor
{
	enum IconType : uint8_t
	{
		EDITOR_ICON_VKR_SHADED,
		EDITOR_ICON_VKR_BLACK,
		EDITOR_ICON_VKR_WHITE,
		EDITOR_ICON_VKR_LIGHT,
		EDITOR_ICON_VKR_DARK,

		EDITOR_ICON_MINUS_WHITE,
		EDITOR_ICON_CROSS_WHITE,
		EDITOR_ICON_SQUARE_WHITE,
		EDITOR_ICON_SQUARES_WHITE,

		EDITOR_ICON_COUNT
	};

	struct Icon
	{
		Ref<Render::TextureView> m_Texture;
	};

	class Icons
	{
	public:
		Icons() = default;
		~Icons() = default;

		bool Init();

		const Icon& GetIcon(IconType type) const;

	private:
		bool LoadIcon(const std::filesystem::path& path, Icon& outIcon);

		std::array<Icon, EDITOR_ICON_COUNT> m_Icons;
	};
}
#endif //ENABLE_EDITOR