/*
 *   Copyright (c) 2023 Watch Dogs LLC
 *   All rights reserved.
 *   You are only allowed access to this code, if given WRITTEN permission by Watch Dogs LLC.
 */

#pragma once

#include <Foundation/Types/Bitflags.h>
#include <RendererDX12/RendererDX12DLL.h>

#include <d3dx12/d3dx12.h>
using namespace Microsoft::WRL;
struct ezD3D12AllocationCreateFlags
{
  using StorageType = ezUInt32;
  enum Enum
  {
    Committed = 0x0000001,
    Reserved = 0x0000002,
    Placed = 0x00000002,
    /// @brief Defualt are Committed Resources.
    Defualt = 0,
  };
  struct Bits
  {
    StorageType DedicatedMemory : 1;
    StorageType NeverAllocate : 1;
    StorageType Mapped : 1;
    StorageType UnusedBit3 : 1;

    StorageType UnusedBit4 : 1;
    StorageType UnusedBit5 : 1;
    StorageType UnusedBit6 : 1;
    StorageType UnusedBit7 : 1;

    StorageType UnusedBit8 : 1;
    StorageType CanAlias : 1;
    StorageType HostAccessSequentialWrite : 1;
    StorageType HostAccessRandom : 1;

    StorageType UnusedBit12 : 1;
    StorageType UnusedBit13 : 1;
    StorageType UnusedBit14 : 1;
    StorageType StrategyMinMemory : 1;

    StorageType StrategyMinTime : 1;
  };
};
EZ_DECLARE_FLAGS_OPERATORS(ezD3D12AllocationCreateFlags);

/// Start of D3D12MA Specific Implementations.

/// NOTE: We can cut down alot of time with the Memory Allocator since VMA & D3D12MA have the same structure.

struct ezD3D12MemoryUsage
{
  typedef ezUInt8 StorageType;
  enum Enum
  {
    Unknown = 0,
    GpuLazilyAllocated = 6,
    Auto = 7,
    AutoPreferDevice = 8,
    AutoPreferHost = 9,
    Default = Unknown,
  };
};

/// \brief Subset of D3D12MAAllocationCreateInfo. Duplicated for abstraction purposes.
struct ezD3D12AllocationCreateInfo
{
  ezBitflags<ezD3D12AllocationCreateFlags> m_flags;
  ezEnum<ezD3D12MemoryUsage> m_usage;
  const char* m_pUserData = nullptr;
  /// Should this be used for EditorEngineProcess?
  bool m_bExportSharedAllocation = false; // If this allocation should be exported so other processes can access it.
};

/// \brief Subset of VmaAllocationInfo. Duplicated for abstraction purposes.
struct ezD3D12AllocationInfo
{
  uint32_t m_memoryType;
  ezUInt64 m_deviceMemory;
  ezUInt64 m_offset;
  ezUInt64 m_size;
  void* m_pMappedData;
  void* m_pUserData;
  const char* m_pName;
};

struct ezD3D12ImageCreationInfo
{
  D3D12_RESOURCE_DESC rdesc = {};
  D3D12_SUBRESOURCE_DATA srdesc = {};
};

namespace D3D12MA
{
  class Allocation;
}

class IDXGIAdapter3;
class ID3D12Device;
using ezD3D12Allocation = D3D12MA::Allocation;

class EZ_RENDERERDX12_DLL ezMemoryAllocatorD3D12
{
public:
  static ezResult Initialize(IDXGIAdapter3* physicalDevice, ID3D12Device* device);
  static void DeInitialize();

  static ezResult CreateImage(ezD3D12ImageCreationInfo& ImageInfo, const ezD3D12AllocationCreateInfo allocationCreateInfo, ComPtr<ID3D12Resource>& out_image_resource, ezD3D12Allocation& out_alloc, D3D12_RESOURCE_STATES image_state = D3D12_RESOURCE_STATE_COMMON, ezD3D12AllocationInfo* pAllocInfo = nullptr);
  static void DestroyImage(ComPtr<ID3D12Resource>& image_resource, ezD3D12Allocation& alloc);

  static ezResult CreateBuffer(ezD3D12ImageCreationInfo& ImageInfo, const ezD3D12AllocationCreateInfo allocationCreateInfo, ComPtr<ID3D12Resource>& out_image_resource, ezD3D12Allocation& out_alloc, ezD3D12AllocationInfo* pAllocInfo = nullptr);
  static void DestroyBuffer(ComPtr<ID3D12Resource>& image_resource, ezD3D12Allocation& alloc);

  static ezD3D12AllocationInfo GetAllocationInfo(ezD3D12Allocation alloc);
  static void SetAllocationUserData(ezD3D12Allocation alloc, const char* pUserData);

private:
  struct Impl;
  static Impl* m_pImpl;
};
