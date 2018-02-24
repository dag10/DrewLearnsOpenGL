//
//  Utils.cpp
//

#include <Utils.h>

#if defined(_DIRECTX)
std::vector<wchar_t> dg::ToLPCWSTR(const std::string& str) {
  std::vector<wchar_t> ret(4096);
  MultiByteToWideChar(
    CP_ACP, 0, str.c_str(), -1, ret.data(), (int)ret.size());
  return ret;
}
#endif
