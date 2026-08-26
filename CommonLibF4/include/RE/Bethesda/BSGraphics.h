#pragma once

#include "RE/Bethesda/BSTHashMap.h"

struct ID3D11Buffer;
struct ID3D11ComputeShader;
struct ID3D11DepthStencilView;
struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11DomainShader;
struct ID3D11HullShader;
struct ID3D11PixelShader;
struct ID3D11RenderTargetView;
struct ID3D11ShaderResourceView;
struct ID3D11Texture2D;
struct ID3D11UnorderedAccessView;
struct ID3D11VertexShader;
struct IDXGISwapChain;

namespace RE
{
	enum class DXGI_MODE_SCALING;
	enum class DXGI_MODE_SCANLINE_ORDER;

	class BSD3DResourceCreator;

	struct DXGI_RATIONAL
	{
	public:
		// members
		std::uint32_t numerator;    // 00
		std::uint32_t denominator;  // 04
	};
	static_assert(sizeof(DXGI_RATIONAL) == 0x08);

	namespace BSGraphics
	{
		enum class TextureAddressMode;

		class RendererShadowState;
		class Texture;

		struct Buffer
		{
			uint64_t unk00;
			uint64_t rawVertexData;
		};

		struct TextureHeader
		{
		public:
			// members
			std::uint16_t height = 0;   // 0
			std::uint16_t width = 0;    // 2
			std::uint8_t mipCount = 0;  // 3
			std::uint8_t format = 0;    // 5
			std::uint8_t flags = 0;     // 6
			std::uint8_t tilemode = 0;  // 7
		};
		static_assert(sizeof(TextureHeader) == 0x8);

		class ConstantGroup
		{
		public:
			// members
			ID3D11Buffer* buffer{ nullptr };    // 00
			float* data{ nullptr };             // 08
			bool dataIsCPUWorkBuffer{ false };  // 10
		};
		static_assert(sizeof(ConstantGroup) == 0x18);

		class ComputeShader
		{
		public:
			// members
			std::uint32_t id{ 0 };                         // 00
			ID3D11ComputeShader* shader{ nullptr };        // 08
			std::uint32_t byteCodeSize{ 0 };               // 10
			BSGraphics::ConstantGroup constantBuffers[3];  // 18
			std::uint64_t shaderDesc{ 0 };                 // 60
			std::int8_t constantTable[32]{ 0 };            // 68
		};
		static_assert(sizeof(ComputeShader) == 0x88);

		class DomainShader
		{
		public:
			// members
			std::uint32_t id{ 0 };                              // 00
			ID3D11DomainShader* shader{ nullptr };              // 08
			std::uint32_t byteCodeSize{ 0 };                    // 10
			BSGraphics::ConstantGroup constantBuffers[3]{ 0 };  // 18
			std::uint64_t shaderDesc{ 0 };                      // 60
			std::int8_t constantTable[32]{ 0 };                 // 68
		};
		static_assert(sizeof(DomainShader) == 0x88);

		class HullShader
		{
		public:
			// members
			std::uint32_t id{ 0 };                         // 00
			ID3D11HullShader* shader{ nullptr };           // 08
			std::uint32_t byteCodeSize{ 0 };               // 10
			BSGraphics::ConstantGroup constantBuffers[3];  // 18
			std::uint64_t shaderDesc{ 0 };                 // 60
			std::int8_t constantTable[32]{ 0 };            // 68
		};
		static_assert(sizeof(HullShader) == 0x88);

		class PixelShader
		{
		public:
			// members
			std::uint32_t id{ 0 };                         // 00
			ID3D11PixelShader* shader{ nullptr };          // 08
			BSGraphics::ConstantGroup constantBuffers[3];  // 10
			std::int8_t constantTable[32]{ 0 };            // 58
		};
		static_assert(sizeof(PixelShader) == 0x78);

		class VertexShader
		{
		public:
			// members
			std::uint32_t id{ 0 };                         // 00
			ID3D11VertexShader* shader{ nullptr };         // 08
			std::uint32_t byteCodeSize{ 0 };               // 10
			BSGraphics::ConstantGroup constantBuffers[3];  // 18
			std::uint64_t shaderDesc{ 0 };                 // 60
			std::int8_t constantTable[32]{ 0 };            // 68
		};
		static_assert(sizeof(VertexShader) == 0x88);

		class CubeMapRenderTarget
		{
		public:
			// members
			ID3D11Texture2D* texture;           // 00
			ID3D11RenderTargetView* rtView[6];  // 08
			ID3D11ShaderResourceView* srView;   // 38
		};
		static_assert(sizeof(CubeMapRenderTarget) == 0x40);

		class DepthStencilTarget
		{
		public:
			// members
			ID3D11Texture2D* texture;                               // 00
			ID3D11DepthStencilView* dsView[4];                      // 08
			ID3D11DepthStencilView* dsViewReadOnlyDepth[4];         // 28
			ID3D11DepthStencilView* dsViewReadOnlyStencil[4];       // 48
			ID3D11DepthStencilView* dsViewReadOnlyDepthStencil[4];  // 68
			ID3D11ShaderResourceView* srViewDepth;                  // 88
			ID3D11ShaderResourceView* srViewStencil;                // 90
		};
		static_assert(sizeof(DepthStencilTarget) == 0x98);

		class RenderTarget
		{
		public:
			// members
			ID3D11Texture2D* texture;              // 00
			ID3D11Texture2D* copyTexture;          // 08
			ID3D11RenderTargetView* rtView;        // 10
			ID3D11ShaderResourceView* srView;      // 18
			ID3D11ShaderResourceView* copySRView;  // 20
			ID3D11UnorderedAccessView* uaView;     // 28
		};
		static_assert(sizeof(RenderTarget) == 0x30);

		class RendererWindow
		{
		public:
			void* hwnd;
			std::int32_t windowX;
			std::int32_t windowY;
			std::int32_t windowWidth;
			std::int32_t windowHeight;
			IDXGISwapChain* swapChain;
			RenderTarget swapChainRenderTarget;
		};
		static_assert(sizeof(RendererWindow) == 0x50);

		class RendererData
		{
		public:
			[[nodiscard]] static RendererData* GetSingleton()
			{
				REL::Relocation<RendererData**> singleton{ REL::ID(1235449, 2704429) };
				return *singleton;
			}

			// members
			RendererShadowState* shadowState;             // 0000
			BSD3DResourceCreator* resourceCreator;        // 0008
			std::uint32_t adapter;                        // 0010
			DXGI_RATIONAL desiredRefreshRate;             // 0014
			DXGI_RATIONAL actualRefreshRate;              // 001C
			DXGI_MODE_SCALING scaleMode;                  // 0024
			DXGI_MODE_SCANLINE_ORDER scanlineMode;        // 0028
			std::int32_t fullScreen;                      // 002C
			bool appFullScreen;                           // 0030
			bool borderlessWindow;                        // 0031
			bool vsync;                                   // 0032
			bool initialized;                             // 0033
			bool requestWindowSizeChange;                 // 0034
			std::uint32_t newWidth;                       // 0038
			std::uint32_t newHeight;                      // 003C
			std::uint32_t presentInterval;                // 0040
			ID3D11Device* device;                         // 0048
			ID3D11DeviceContext* context;                 // 0050
			RendererWindow renderWindow[32];              // 0058
			RenderTarget renderTargets[101];              // 0A58
			DepthStencilTarget depthStencilTargets[13];   // 1D48
			CubeMapRenderTarget cubeMapRenderTargets[2];  // 2500
			std::byte rendererLock[0x25A8 - 0x2580];      // 2580 - TODO
			const char* className;                        // 25A8
			void* instance;                               // 25B0
			bool nvapiEnabled;                            // 25B8
		};
		static_assert(sizeof(RendererData) == 0x25C0);

		class Renderer
		{
		public:
			using ResetRenderTargets_t = void (*)();

			void IncRef(Buffer* vertexBuffer)
			{
				using func_t = decltype(&BSGraphics::Renderer::IncRef);
				REL::Relocation<func_t> func{ REL::ID(1337764, 2276869) };
				return func(this, vertexBuffer);
			}
			void DecRef(Buffer* vertexBuffer)
			{
				using func_t = decltype(&BSGraphics::Renderer::DecRef);
				REL::Relocation<func_t> func{ REL::ID(194808, 2276870) };
				return func(this, vertexBuffer);
			}

			// members
			bool skipNextPresent;                     // 00
			ResetRenderTargets_t resetRenderTargets;  // 08
			RendererData data;                        // 10
		};
		static_assert(sizeof(Renderer) == 0x25D0);

		struct Vertex
		{
			enum Attribute : std::uint8_t
			{
				VA_POSITION = 0x0,
				VA_TEXCOORD0 = 0x1,
				VA_TEXCOORD1 = 0x2,
				VA_NORMAL = 0x3,
				VA_BINORMAL = 0x4,
				VA_COLOR = 0x5,
				VA_SKINNING = 0x6,
				VA_LANDDATA = 0x7,
				VA_EYEDATA = 0x8,

				VA_COUNT = 9
			};

			enum Flags : std::uint16_t
			{
				VF_VERTEX = 1 << VA_POSITION,
				VF_UV = 1 << VA_TEXCOORD0,
				VF_UV_2 = 1 << VA_TEXCOORD1,
				VF_NORMAL = 1 << VA_NORMAL,
				VF_TANGENT = 1 << VA_BINORMAL,
				VF_COLORS = 1 << VA_COLOR,
				VF_SKINNED = 1 << VA_SKINNING,
				VF_LANDDATA = 1 << VA_LANDDATA,
				VF_EYEDATA = 1 << VA_EYEDATA,
				VF_FULLPREC = 0x400
			};

			enum Masks : std::uint64_t
			{
				DESC_MASK_VERT = 0xFFFFFFFFFFFFFFF0,
				DESC_MASK_UVS = 0xFFFFFFFFFFFFFF0F,
				DESC_MASK_NBT = 0xFFFFFFFFFFFFF0FF,
				DESC_MASK_SKCOL = 0xFFFFFFFFFFFF0FFF,
				DESC_MASK_DATA = 0xFFFFFFFFFFF0FFFF,
				DESC_MASK_OFFSET = 0xFFFFFF0000000000,
				DESC_MASK_FLAGS = ~(DESC_MASK_OFFSET)
			};
		};

		class VertexDesc
		{
		public:
			[[nodiscard]] bool HasFlag(Vertex::Flags a_flag) const
			{
				return ((desc >> 44) & a_flag) != 0;
			}
			void SetFlag(Vertex::Flags a_flag)
			{
				desc |= (static_cast<uint64_t>(a_flag) << 44);
			}
			void ClearFlag(Vertex::Flags a_flag)
			{
				desc &= ~(static_cast<uint64_t>(a_flag) << 44);
			}

			[[nodiscard]] std::uint32_t GetAttributeOffset(Vertex::Attribute a_attribute) const
			{
				return (desc >> (4 * static_cast<uint8_t>(a_attribute) + 2)) & 0x3C;
			}
			void SetAttributeOffset(Vertex::Attribute a_attribute, std::uint32_t a_offset)
			{
				if (a_attribute != Vertex::Attribute::VA_POSITION) {
					const uint64_t lhs = static_cast<uint64_t>(a_offset) << (4 * static_cast<uint8_t>(a_attribute) + 2);
					const uint64_t rhs = desc & ~static_cast<uint64_t>(15 << (4 * static_cast<uint8_t>(a_attribute) + 4));

					desc = lhs | rhs;
				}
			}
			void ClearAttributeOffsets()
			{
				desc &= Vertex::Masks::DESC_MASK_OFFSET;
			}

			[[nodiscard]] Vertex::Flags GetFlags() const
			{
				return static_cast<Vertex::Flags>((desc & Vertex::Masks::DESC_MASK_OFFSET) >> 44);
			}
			void SetFlags(Vertex::Flags a_flags)
			{
				desc |= (static_cast<uint64_t>(a_flags) << 44) | (desc & Vertex::Masks::DESC_MASK_FLAGS);
			}

			[[nodiscard]] std::uint32_t GetSize()
			{
				return (desc & 0xF) * 4;
			}

		private:
			// members
			std::uint64_t desc;  // 00
		};
		static_assert(sizeof(VertexDesc) == 0x8);

		struct TriShape
		{
			VertexDesc vertexDesc;
			Buffer* buffer08;
			Buffer* buffer10;
		};
	};

	namespace BSShaderTechniqueIDMap
	{
		template <class T>
		struct HashPolicy
		{
			[[nodiscard]] std::uint32_t operator()(const T a_shader) const noexcept
			{
				assert(a_shader != nullptr);
				return a_shader->id;
			}
		};

		template <class T>
		struct ComparePolicy
		{
			[[nodiscard]] bool operator()(const T a_lhs, const T a_rhs) const noexcept
			{
				assert(a_lhs != nullptr);
				assert(a_rhs != nullptr);
				return a_lhs->id == a_rhs->id;
			}
		};

		template <class T>
		using MapType = BSTSet<T, HashPolicy<T>, ComparePolicy<T>>;
	}
}
