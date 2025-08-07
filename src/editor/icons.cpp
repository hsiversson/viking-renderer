#include "icons.h"

#if ENABLE_EDITOR
#include "render/device.h"

namespace vkr::Editor
{
	bool Icons::Init()
	{
		//if (!LoadIcon("../../../content/icons/icon_vkr_shaded.dds", m_Icons[EDITOR_ICON_VKR_SHADED])) return false;
		//if (!LoadIcon("../../../content/icons/icon_vkr_black.dds", m_Icons[EDITOR_ICON_VKR_BLACK])) return false;
		if (!LoadIcon("../../../content/icons/icon_vkr_white.dds", m_Icons[EDITOR_ICON_VKR_WHITE])) return false;
		//if (!LoadIcon("../../../content/icons/icon_vkr_light.dds", m_Icons[EDITOR_ICON_VKR_LIGHT])) return false;
		//if (!LoadIcon("../../../content/icons/icon_vkr_dark.dds", m_Icons[EDITOR_ICON_VKR_DARK])) return false;

		if (!LoadIcon("../../../content/icons/icon_minus_white.dds", m_Icons[EDITOR_ICON_MINUS_WHITE])) return false;
		if (!LoadIcon("../../../content/icons/icon_cross_white.dds", m_Icons[EDITOR_ICON_CROSS_WHITE])) return false;
		if (!LoadIcon("../../../content/icons/icon_square_white.dds", m_Icons[EDITOR_ICON_SQUARE_WHITE])) return false;
		if (!LoadIcon("../../../content/icons/icon_squares_white.dds", m_Icons[EDITOR_ICON_SQUARES_WHITE])) return false;

		return true;
	}

	const Icon& Icons::GetIcon(IconType type) const
	{
		return m_Icons[type];
	}

	bool Icons::LoadIcon(const std::filesystem::path& path, Icon& outIcon)
	{
		Ref<Render::Texture> icon = Render::GetDevice()->LoadTexture(path);
		if (!icon)
		{
			return false;
		}

		outIcon.m_Texture = Render::GetDevice()->CreateTextureView({}, icon);
		return outIcon.m_Texture != nullptr;
	}
}

#endif //ENABLE_EDITOR