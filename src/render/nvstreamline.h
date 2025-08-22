
#pragma once

namespace sl
{
	struct FrameToken;
	struct float2;
	struct float3;
	struct float4;
	struct float4x4;
}

namespace vkr::Render
{
	class Device;

	enum NvStreamlineFeature
	{
		DLSS,
		DLSS_RR
	};

	class NvStreamline
	{
	public:
		NvStreamline();
		~NvStreamline();
		bool Init();
		bool Shutdown();
		bool SetDevice(const Device* device);
		bool IsFeatureAvailable(NvStreamlineFeature feature);
		const sl::FrameToken* GetFrameToken(uint32_t frameIndex) const;

		static void Convert(sl::float2& aOut, const Vector2f& aVector);
		static void Convert(sl::float3& aOut, const Vector3f& aVector);
		static void Convert(sl::float4& aOut, const Vector4f& aVector);
		static void Convert(sl::float4x4& aOut, const Mat44& aMatrix);

		static void Convert(Vector2f& aOut, const sl::float2& aVector);
		static void Convert(Vector3f& aOut, const sl::float3& aVector);
		static void Convert(Vector4f& aOut, const sl::float4& aVector);
		static void Convert(Mat44& aOut, const sl::float4x4& aMatrix);
	private:
		struct PImpl;
		UniquePtr<PImpl> m_pImpl;
	};
}