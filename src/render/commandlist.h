#pragma once
#include "render/rendercommon.h"
#include "render/context.h"

namespace vkr::Render
{
	enum CommandSignatureType
	{
		COMMAND_SIGNAUTRE_DRAW,
		COMMAND_SIGNAUTRE_DRAW_INDEXED,
		COMMAND_SIGNAUTRE_DISPATCH,
	};

	// Currently only supports draw and dispatch command signatures, no multi argument signatures, but this can be extended in the future if needed
	class CommandSignature 
	{
	public:
		CommandSignature(CommandSignatureType type);

		ID3D12CommandSignature* GetD3DCommandSignature() const;

		uint64_t GetRequiredArgumentBufferSize() const;
		CommandSignatureType GetType() const;

	private:
		ComPtr<ID3D12CommandSignature> m_CommandSignature;
		const CommandSignatureType m_Type;
	};

	class CommandList
	{
	public:
		CommandList(ContextType type);

		void Open();
		void Close();

		ContextType GetType() const;

		ID3D12GraphicsCommandList* GetD3DCommandList() const;

	private:
		ComPtr<ID3D12GraphicsCommandList> m_CommandList;
		ComPtr<ID3D12CommandAllocator> m_Allocator;
		const ContextType m_Type;
	};

	class CommandListPool
	{
	public:
		struct PendingCommandLists
		{
			std::vector<Ref<CommandList>> m_CommandLists;
			Fence m_Event;
		};

	public:
		CommandListPool(ContextType type);
		~CommandListPool();

		Ref<CommandList> GetCommandList();
		void ReturnCommandList(Ref<CommandList> commandList, Fence event);
		void ReturnCommandList(const PendingCommandLists& pendingCommandLists);

		ContextType GetType() const;

	private:
		void CheckPendingCommandLists();

		std::queue<PendingCommandLists> m_PendingCommandLists;
		std::vector<Ref<CommandList>> m_FreeCommandLists;
		std::mutex m_PendingListMutex;
		std::mutex m_FreeListMutex;

		const ContextType m_Type;
	};
}