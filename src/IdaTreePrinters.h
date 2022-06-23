#pragma once

#include <ostream>
#include <string>
#include <vector>

#include "IdaTraceFileRecord.h"
#include "PrettyPrintUtils.h"
#include "TraceCallTree.h"

// Printer
struct IdaTreeTabbedPrinterContext
{
  std::ostream* pOutput = nullptr;
  int iDepthPrev = 0;
  std::vector<std::string> filterSkipResult;
};

bool treeTabbedTextPrinter(CallTreeNode<IdaTraceFileRecord>& info, int iDepth, void* pContext)
{
  IdaTreeTabbedPrinterContext* pPrinterContext = static_cast<IdaTreeTabbedPrinterContext*>(pContext);

  for (auto& filter : pPrinterContext->filterSkipResult)
  {
    if (info.value.msResult.find(filter) != std::string::npos)
    {
      return false;
    }
  }

  const auto iDepthDiff = iDepth - pPrinterContext->iDepthPrev;
  for (auto i = 0; i < iDepthDiff; ++i)
  {
    *(pPrinterContext->pOutput) << ind(iDepth + i - 1) << "{" << std::endl;
  }
  for (auto i = iDepthDiff; i < 0; ++i)
  {
    *(pPrinterContext->pOutput) << ind(pPrinterContext->iDepthPrev + iDepthDiff - i - 1) << "}" << std::endl;
  }

  if (info.value.msInstruction_name == "call")
  {
    *(pPrinterContext->pOutput) << ind(iDepth) << info.value.msResult_module << ":" << IdaTraceFileRecord::getWithoutAccessKeyword(info.value.msResult_other) << std::endl;
  }
  else
  {
    *(pPrinterContext->pOutput) << ind(iDepth) << info.value.getFunctionNameFromInstruction()
      // << " /* " << info.value.msResult_other << " */ "
      << std::endl;
  }

  pPrinterContext->iDepthPrev = iDepth;

  //*(pPrinterContext->pOutput) << ind(iDepth) << info.msAddress << "\t" << info.msInstruction << "\t" << info.msResult << endl; // TODO: Remove
  return true;
}

// Printer
struct IdaTreeDotPrinterContext
{
  std::ostream* pOutput = nullptr;
  int iDepthPrev = 0;
  std::vector<std::string> filterSkipResult;
  std::vector<std::string> filterColumns;

  std::map<std::string, std::list<CallTreeNode<IdaTraceFileRecord>*> > mapModuleNodes;

  std::string sPrev;
};

bool treeDotTextPrinter(CallTreeNode<IdaTraceFileRecord>& info, int iDepth, void* pContext)
{
  IdaTreeDotPrinterContext* pPrinterContext = static_cast<IdaTreeDotPrinterContext*>(pContext);

  // Remove unwanted
  for (auto& filter : pPrinterContext->filterSkipResult)
  {
    if (info.value.msResult.find(filter) != std::string::npos)
    {
      return false;
    }
  }

  // Skip return
  if (info.value.msInstruction_name == "retn")
  {
    return true;
  }

  // Add columns by user specified filters or module names
  if (pPrinterContext->filterColumns.empty())
  {
    if (!info.value.msResult_module.empty())
    {
      pPrinterContext->mapModuleNodes[info.value.msResult_module].push_back(&info);
    }
  }
  else
  {
    for (auto& filter : pPrinterContext->filterColumns)
    {
      if (info.value.msResult_clean.find(filter) != std::string::npos)
      {
        pPrinterContext->mapModuleNodes[filter].push_back(&info);
      }
    }
  }

  // Deal with depth differences: add sub-graphs or close them
  const auto iDepthDiff = iDepth - pPrinterContext->iDepthPrev;
  for (auto i = 0; i < iDepthDiff; ++i)
  {
    std::string sLabel = IdaTraceFileRecord::getFunctionNameOnly(info.pParent->value.msResult_other);
    if (sLabel.length() > 50)
    {
      sLabel = sLabel.substr(0, 50) + "...";
    }
    *(pPrinterContext->pOutput) << ind(iDepth + i - 1) << "subgraph cluster_" << info.pParent << " {" << std::endl;
    *(pPrinterContext->pOutput) << ind(iDepth + i) << "label = \"" << sLabel << "\";" << std::endl;
    *(pPrinterContext->pOutput) << ind(iDepth + i) << "tooltip = \"" << info.pParent->value.msResult_other << "\";" << std::endl;
    *(pPrinterContext->pOutput) << ind(iDepth + i) << "style=filled;" << std::endl;

    *(pPrinterContext->pOutput) << ind(iDepth + i) << "fillcolor = \"" << ((iDepth + i) % 7) + 1 << "\";" << std::endl;
    *(pPrinterContext->pOutput) << ind(iDepth + i) << "colorscheme=greys9;" << std::endl;
  }
  for (auto i = iDepthDiff; i < 0; ++i)
  {
    *(pPrinterContext->pOutput) << ind(pPrinterContext->iDepthPrev + iDepthDiff - i - 1) << "}" << std::endl;
  }

  // Define properties of node
  const std::string sName = "instr_" + toStr(&info);
  std::string sLabel;
  std::string sTooltip;
  std::string sColor = "#fed9a6";
  if (!info.value.msResult_module.empty())
  {
    sColor = "#decbe4";
  }
  if (info.value.msInstruction_name == "call")
  {
    sTooltip = info.value.msResult_clean + "\\n\\n"
      + info.value.msAddress + "\\n\\n"
      + info.value.msInstruction;
    sLabel = IdaTraceFileRecord::getFunctionNameOnly(info.value.msResult_other);
  }
  else
  {
    sTooltip = info.value.msInstruction;
    sLabel = info.value.getFunctionNameFromInstruction();
  }

  if (sLabel.length() > 50)
  {
    sLabel = sLabel.substr(0, 50) + "...";
  }

  // Add node and edge
  *(pPrinterContext->pOutput) << ind(iDepth) << sName << "["
    << "label=\"" << sLabel << "\","
    << "fillcolor=\"" << sColor << "\","
    << "tooltip=\"" << sTooltip << "\","
    << "];" << std::endl;
  *(pPrinterContext->pOutput) << ind(iDepth) << pPrinterContext->sPrev << " ->" << sName << ";" << endl;

  // Store this record for future references
  pPrinterContext->sPrev = sName;
  pPrinterContext->iDepthPrev = iDepth;

  return true;
}

bool treeDotTextPrinterHeader(void* pContext)
{
  IdaTreeDotPrinterContext* pPrinterContext = static_cast<IdaTreeDotPrinterContext*>(pContext);
  *(pPrinterContext->pOutput) << "digraph {" << endl;
  *(pPrinterContext->pOutput) << "  " << "graph [newrank=true,ranksep=\"0.15\"];" << endl;
  *(pPrinterContext->pOutput) << "  " << "node [newrank=true,shape=box style=filled];" << endl << endl;

  *(pPrinterContext->pOutput) << "  " << "Begin[];" << endl << endl;
  ++pPrinterContext->iDepthPrev;

  for (auto& col : pPrinterContext->filterColumns)
  {
    pPrinterContext->mapModuleNodes[col];
  }

  return true;
}

bool treeDotTextPrinterFooter(void* pContext)
{
  IdaTreeDotPrinterContext* pPrinterContext = static_cast<IdaTreeDotPrinterContext*>(pContext);

  for (auto i = pPrinterContext->iDepthPrev; i > 1; --i)
  {
    *(pPrinterContext->pOutput) << ind(i) << "}" << std::endl;
  }

  int iDepth = 1;
  int iColor = 0;

  *(pPrinterContext->pOutput) << ind(iDepth) << pPrinterContext->sPrev << "->" << "End;" << endl;

  for (auto& moduleRec : pPrinterContext->mapModuleNodes)
  {
    iColor = iColor % 9 + 1;

    *(pPrinterContext->pOutput) << ind(iDepth) << "subgraph cluster_" << &moduleRec.first << " {" << std::endl;
    *(pPrinterContext->pOutput) << ind(iDepth + 1) << "label = \"" << moduleRec.first << "\";" << std::endl;
    //*(pPrinterContext->pOutput) << ind(iDepth + 1) << "tooltip = \"" << info.pParent->value.msResult_other << "\";" << std::endl;
    *(pPrinterContext->pOutput) << ind(iDepth + 1) << "style=filled;" << std::endl;
    *(pPrinterContext->pOutput) << ind(iDepth + 1) << "fillcolor = \"" << iColor << "\";" << std::endl;
    *(pPrinterContext->pOutput) << ind(iDepth + 1) << "colorscheme=bugn9;" << std::endl;

    *(pPrinterContext->pOutput) << ind(iDepth + 1) << "firtsFor_" << &moduleRec.first << "[style=invis];" << std::endl;

    for (auto& node : moduleRec.second)
    {
      std::string sLabel;
      std::string sTooltip;

      sTooltip = node->value.msResult_clean + "\\n\\n"
        + node->value.msAddress + "\\n\\n"
        + node->value.msInstruction;
      sLabel = IdaTraceFileRecord::getFunctionNameOnly(node->value.msResult_other);
      if (sLabel.length() > 50)
      {
        sLabel = sLabel.substr(0, 50) + "...";
      }

      *(pPrinterContext->pOutput) << ind(iDepth + 1) << "extern_" << node << "["
        << "label=\"" << sLabel << "\","
        << "tooltip=\"" << sTooltip << "\","
        << "];" << std::endl;
    }

    *(pPrinterContext->pOutput) << ind(iDepth) << "}" << std::endl;

    for (auto& node : moduleRec.second)
    {
      *(pPrinterContext->pOutput) << ind(iDepth) << "{rank=same;" << "extern_" << node << ";" << "instr_" << node << "};" << endl;
    }
    *(pPrinterContext->pOutput) << ind(iDepth) << "{rank=min;" << "firtsFor_" << &moduleRec.first << "};" << endl;
  }

  *(pPrinterContext->pOutput) << "}" << std::endl;

  return true;
}
