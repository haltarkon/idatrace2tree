#pragma once

#include <string>

// Pretty print
inline std::string ind(int iIndent, const char cForward = ' ', const char cBackward = '_')
{
  if (iIndent < 0)
  {
    return std::string(-iIndent * 2, cBackward);
  }
  return std::string(iIndent * 2, cForward);
}

template<class T>
inline std::string toStr(const T* ptr)
{
  std::stringstream ss;
  ss << ptr;  
  return ss.str(); 
}
