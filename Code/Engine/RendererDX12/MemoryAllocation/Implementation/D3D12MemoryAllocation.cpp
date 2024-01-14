/*
 *   Copyright (c) 2023 Watch Dogs LLC
 *   All rights reserved.
 *   You are only allowed access to this code, if given WRITTEN permission by Watch Dogs LLC.
 */
#include <RendererDX12/MemoryAllocation/D3D12MA.h>
#include <RendererDX12/MemoryAllocation/D3D12MemoryAllocation.h>
#include <RendererDX12/RendererDX12PCH.h>
#include <RendererDX12/RendererDX12Helpers.h>

EZ_CHECK_AT_COMPILETIME(D3D12MA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT == ezD3D12AllocationCreateFlags::DedicatedMemory);
EZ_CHECK_AT_COMPILETIME(D3D12MA_ALLOCATION_CREATE_NEVER_ALLOCATE_BIT == ezD3D12AllocationCreateFlags::NeverAllocate);
EZ_CHECK_AT_COMPILETIME(D3D12MA_ALLOCATION_CREATE_MAPPED_BIT == ezD3D12AllocationCreateFlags::Mapped);
EZ_CHECK_AT_COMPILETIME(D3D12MA_ALLOCATION_CREATE_CAN_ALIAS_BIT == ezD3D12AllocationCreateFlags::CanAlias);
EZ_CHECK_AT_COMPILETIME(D3D12MA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT == ezD3D12AllocationCreateFlags::HostAccessSequentialWrite);
EZ_CHECK_AT_COMPILETIME(D3D12MA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT == ezD3D12AllocationCreateFlags::HostAccessRandom);
EZ_CHECK_AT_COMPILETIME(D3D12MA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT == ezD3D12AllocationCreateFlags::StrategyMinMemory);
EZ_CHECK_AT_COMPILETIME(D3D12MA_ALLOCATION_CREATE_STRATEGY_MIN_TIME_BIT == ezD3D12AllocationCreateFlags::StrategyMinTime);

EZ_CHECK_AT_COMPILETIME(D3D12MA_MEMORY_USAGE_UNKNOWN == ezD3D12MemoryUsage::Unknown);
EZ_CHECK_AT_COMPILETIME(D3D12MA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED == ezD3D12MemoryUsage::GpuLazilyAllocated);
EZ_CHECK_AT_COMPILETIME(D3D12MA_MEMORY_USAGE_AUTO == ezD3D12MemoryUsage::Auto);
EZ_CHECK_AT_COMPILETIME(D3D12MA_MEMORY_USAGE_AUTO_PREFER_DEVICE == ezD3D12MemoryUsage::AutoPreferDevice);
EZ_CHECK_AT_COMPILETIME(D3D12MA_MEMORY_USAGE_AUTO_PREFER_HOST == ezD3D12MemoryUsage::AutoPreferHost);
struct ExportedSharedPool
{
  D3D12MA::Pool m_pool;
  /// NOTE: Not completely sure for the D3D12 Equalvent of: vk::ExportMemoryAllocateInfo.
  /// Creating a Shared Heap will help in this case, but again, not completely sure.
};
struct ezMemoryAllocatorD3D12::Impl
{
  Impl();
  D3D12MA::Allocator m_allocator;
  ezMutex m_exportedSharedPoolsMutex;
  ezHashTable<uint32_t, ExportedSharedPool> m_exportedSharedPools;
};

ezMemoryAllocatorD3D12::Impl* ezMemoryAllocatorD3D12::m_pImpl = nullptr;

ezResult ezMemoryAllocatorD3D12::Initialize(IDXGIAdapter3* physicalDevice, ID3D12Device* device)
{
  EZ_ASSERT_DEV(m_pImpl == nullptr, "ezMemoryAllocatorD3D12::Initialize was already called");
  m_pImpl = EZ_DEFAULT_NEW(Impl);

  /// Create D3D12 MA Allocator.
  D3D12MA::ALLOCATOR_DESC desc = {};

  desc.pDevice = device;
  desc.pAdapter = physicalDevice;

  HRESULT hr = D3D12MA::CreateAllocator(&desc, (D3D12MA::Allocator**)&m_pImpl->m_allocator);
  if (FAILED(hr))
  {
    ezLog::Error("D3D12MemoryAllocator: Failed to create AMDD3D12MA Allocator Object. is the DX12 Device or Adapter valid? HRESULT:{0}", HrToString(hr));
    EZ_DEFAULT_DELETE(m_pImpl);
    return EZ_FAILURE;
  }
  return EZ_SUCCESS;
}

void ezMemoryAllocatorD3D12::DeInitialize()
{
  EZ_ASSERT_DEV(m_pImpl != nullptr, "nsMemoryAllocatorD3D12 is not initialized. it is either already destroyed or invalid.");
  for (auto it : m_pImpl->m_exportedSharedPools)
  {
    it.Value().m_pool.Release();
  }
  m_pImpl->m_exportedSharedPools.Clear();
  m_pImpl->m_allocator.Release();
  EZ_DEFAULT_DELETE(m_pImpl);
}

ezResult ezMemoryAllocatorD3D12::CreateImage(ezD3D12ImageCreationInfo& ImageInfo, const ezD3D12AllocationCreateInfo allocationCreateInfo, ComPtr<ID3D12Resource>& out_image_resource, ezD3D12Allocation& out_alloc, D3D12_RESOURCE_STATES image_state = D3D12_RESOURCE_STATE_COMMON, ezD3D12AllocationInfo* pAllocInfo /*= nullptr*/)
{
  D3D12MA::ALLOCATION_DESC allocdesc = {};
  allocdesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
  if (allocationCreateInfo.m_bExportSharedAllocation)
  {
    allocdesc.Flags |= D3D12MA::ALLOCATION_FLAG_COMMITTED;
  }
  D3D12MA::POOL_DESC pooldesc = {};
  pooldesc.HeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
  HRESULT hr = m_pImpl->m_allocator.CreateResource(&allocdesc, &ImageInfo.rdesc, image_state, NULL, (D3D12MA::Allocation**)& out_alloc, IID_PPV_ARGS(&out_image_resource));
  if (FAILED(hr))
  {
    ezLog::Error("D3D12MemoryAllocator: Failed to create AMDD3D12MA Allocator Object. is the DX12 Device or Adapter valid? HRESULT:{0}", HrToString(hr));
    return EZ_FAILURE;
  }
  /*
  * No need for this anymore.
  else
  {
    out_image_resource = out_alloc.GetResource();
    return EZ_SUCCESS;
  }
  */
}

void ezMemoryAllocatorD3D12::DestroyImage(ComPtr<ID3D12Resource>& image_resource, ezD3D12Allocation& alloc)
{
  /// NOTE: Resource MUST BE DESTROYED IN THIS ORDER.
  image_resource->Release();
  alloc.Release();
}

ezResult ezMemoryAllocatorD3D12::CreateBuffer(ezD3D12ImageCreationInfo& ImageInfo, const ezD3D12AllocationCreateInfo allocationCreateInfo, ComPtr<ID3D12Resource>& out_image_resource, ezD3D12Allocation& out_alloc, ezD3D12AllocationInfo* pAllocInfo /*= nullptr*/)
{
}
