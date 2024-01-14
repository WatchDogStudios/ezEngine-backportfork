#pragma once

/*

NOTE: This file is for any helpers for D3D12.
On a future note, when back porting this to ezEngine, we would clean their version up,
most of the stuff we have here is likely not to make it in.

NOTE: Helpers for Xbox DX12 will be contained in GDXKPlatform. this is to make sure no Xbox NDA code is leaked.

*/
#include <RendererDX12/RendererDX12PCH.h>

inline namespace
{
  inline std::string HrToString(HRESULT hr)
  {
    char s_str[64] = {};
    sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
    return std::string(s_str);
  }

  class HrException : public std::runtime_error
  {
  public:
    HrException(HRESULT hr)
      : std::runtime_error(HrToString(hr))
      , m_hr(hr)
    {
    }

    HRESULT Error() const { return m_hr; }

  private:
    const HRESULT m_hr;
  };

#define SAFE_RELEASE(p) \
  if (p)                \
    (p)->Release();

  /// @brief Helper function for handling HRESULT functioez. use for unrecoverable situatioez.
  /// @param hr Result that is being checked
  /// @param Message Message to report to the user.
  /// @param throwexec
  inline void ThrowIfFailed(HRESULT hr, const char* Message, bool throwexec = false)
  {
    if (FAILED(hr))
    {
      ezLog::Error("RendererDX12: {0} HRESULT CODE: {1}", Message, HrToString(hr));
      if (throwexec == true)
        throw HrException(hr);
    }
  }
} // namespace
