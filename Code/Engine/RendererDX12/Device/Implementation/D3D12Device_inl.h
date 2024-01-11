
EZ_ALWAYS_INLINE ID3D12Device3* ezGALDeviceDX12::GetDXDevice() const
{
  /*
    Q: Why are you using ID3D12Device3 ieztead of ID3D12Device1?
    A: Allows us to use a few newer features that are better for parallelization, and some newer Raytracing functioez. we can fallback to a ID3D12Device1, but it shouldnt be a problem with DX12 Supported GPU's.
  */
  return m_pDevice3;
}
EZ_ALWAYS_INLINE IDXGIFactory3* ezGALDeviceDX12::GetDXGIFactory() const
{
  return m_pDXGIFactory;
}

EZ_ALWAYS_INLINE const ezGALFormatLookupTableDX12& ezGALDeviceDX12::GetFormatLookupTable() const
{
  return m_FormatLookupTable;
}

inline ID3D12Query* ezGALDeviceDX12::GetTimestamp(ezGALTimestampHandle hTimestamp)
{
  if (hTimestamp.m_uiIndex < m_Timestamps.GetCount())
  {
    return m_Timestamps[static_cast<ezUInt32>(hTimestamp.m_uiIndex)];
  }

  return nullptr;
}
