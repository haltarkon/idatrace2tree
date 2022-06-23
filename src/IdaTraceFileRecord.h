#pragma once

#include <sstream>
#include <string>
#include <utility>

struct IdaTraceFileRecord
{
  std::string msThread;
  std::string msAddress;
  std::string msAddress_segment; //
  std::string msAddress_func; //
  std::string msAddress_shift; //
  std::string msInstruction;
  std::string msInstruction_name; //
  std::string msInstruction_operands; //
  std::string msResult;
  std::string msResult_clean;
  std::string msResult_func; //
  std::string msResult_comment; //
  std::string msResult_module; //
  std::string msResult_other; //

  IdaTraceFileRecord() = default;

  IdaTraceFileRecord(std::string sThread, std::string sAddress, std::string sInstruction, std::string sResult)
    : msThread(std::move(sThread))
    , msAddress(std::move(sAddress))
    , msInstruction(std::move(sInstruction))
    , msResult(std::move(sResult))
  {
    splitAddress();
    splitInstruction();
    splitResult();
  }

  static size_t getFunctionOffsetPos(const std::string& in)
  {
    auto findPlus = in.rfind('+');
    if (findPlus == std::string::npos)
    {
      findPlus = 0;
    }
    auto findColon = in.rfind(':');
    if (findColon == std::string::npos)
    {
      findColon = 0;
    }
    if (findColon == 0 && findPlus == 0)
    {
      return std::string::npos;
    }
    return std::max(findPlus, findColon);
  }

  void splitAddress()
  {
    const auto findMax = getFunctionOffsetPos(msAddress);
    if (findMax == std::string::npos)
    {
      return;
    }

    if (msAddress.length() > 6 && msAddress.substr(0, 6) == ".text:")
    {
      msAddress_segment = ".text:";
      msAddress_func = msAddress.substr(6, findMax - 6);
    }
    else
    {
      msAddress_func = msAddress.substr(0, findMax);
    }

    msAddress_shift = msAddress.substr(findMax);
  }

  void splitInstruction()
  {
    std::istringstream instr(msInstruction);
    std::getline(instr, msInstruction_name, ' ');
    std::getline(instr, msInstruction_operands);
  }

  void splitResult()
  {
    msResult_comment.clear();
    msResult_other.clear();

    std::istringstream instr(msResult);
    std::getline(instr, msResult_func, ' ');
    const size_t iMangled = msResult_func.length();

    std::string curr;
    bool bMangledFound = false;
    while (std::getline(instr, curr, ' '))
    {
      if (curr.length() > iMangled
        && curr.substr(0, iMangled) == msResult_func)
      {
        bMangledFound = true;
        msResult_other += curr.substr(iMangled);
        continue;
      }

      if (bMangledFound)
      {
        msResult_other += " " + curr;
      }
      else
      {
        msResult_comment += " " + curr;
      }
    }

    msResult_clean = msResult_comment + " " + msResult_other;

    if (msInstruction_name == "retn" || msInstruction_name == "bnd")
    {
      msResult_other = msResult_other.substr(0, getFunctionOffsetPos(msResult_other));
    }

    const size_t iColPos = msResult_other.find(':');
    if (iColPos != std::string::npos)
    {
      const std::string begin = msResult_other.substr(0, 0 + iColPos);
      if (begin != "public" && begin != "private" && begin != "protected" && begin.find(' ') == std::string::npos)
      {
        msResult_module = begin;
        msResult_other = msResult_other.substr(iColPos + 1);
      }
    }

  }

  static std::string getFunctionNameOnly(const std::string& in)
  {
    const size_t iBracket = in.find('(');
    if (iBracket == std::string::npos)
    {
      return in;
    }

    std::string sBegin = in.substr(0, 0 + iBracket);

    const size_t iSpace = sBegin.rfind(' ');
    if (iSpace == std::string::npos)
    {
      return sBegin;
    }

    std::string sResult = sBegin.substr(iSpace);
    
    //if (sResult.find('>') != std::string::npos)
    //{
    //  return sBegin;
    //}

    return sResult;
  }

  static std::string getWithoutAccessKeyword(const std::string& in)
  {
    const char sPublic[] = "public: ";
    constexpr size_t iPublic = sizeof(sPublic) - 1;
    const char sPrivate[] = "private: ";
    constexpr size_t iPrivate = sizeof(sPrivate) - 1;
    const char sProtected[] = "protected: ";
    constexpr size_t iProtected = sizeof(sProtected) - 1;

    if (in.length() > iPublic && in.substr(0, iPublic) == sPublic)
    {
      return in.substr(iPublic);
    }
    else if (in.length() > iProtected && in.substr(0, iProtected) == sProtected)
    {
      return in.substr(iProtected);
    }
    else if (in.length() > iPrivate && in.substr(0, iPrivate) == sPrivate)
    {
      return in.substr(iPrivate);
    }
    return in;
  }

  std::string getFunctionNameFromInstruction(bool bRemoveAccessKeyword = true)
  {
    const char sCall[] = "call    ";
    constexpr size_t iCall = sizeof(sCall) - 1;
    const char sCallImport[] = "call    cs:__declspec(dllimport) ";
    constexpr size_t iCallImport = sizeof(sCallImport) - 1;

    if (msInstruction.length() <= iCall || msInstruction.substr(0, iCall) != sCall)
    {
      return msInstruction;
    }

    std::string out;

    if (msInstruction.length() > iCallImport && msInstruction.substr(0, iCallImport) == sCallImport)
    {
      out = msInstruction.substr(iCallImport);
    }
    else
    {
      out = msInstruction.substr(iCall);
    }

    if (bRemoveAccessKeyword)
    {
      return getWithoutAccessKeyword(out);
    }

    return out;
  }

  static bool readLine(std::istream& stream, IdaTraceFileRecord& record, const char cSeparator = '\t')
  {
    std::string sRow;
    if (!std::getline(stream, sRow))
    {
      return false;
    }

    std::istringstream row(sRow);
    std::string cells[4];
    for (auto& cell : cells)
    {
      if (!std::getline(row, cell, cSeparator))
      {
        return false;
      }
    }

    record = IdaTraceFileRecord(cells[0], cells[1], cells[2], cells[3]);
    return true;
  }

};
