
#include "CmdOpts.h"
#include "IdaTraceFileRecord.h"
#include "TraceCallTree.h"
#include "PrettyPrintUtils.h"
#include "IdaTreePrinters.h"

#include <iostream>
#include <fstream>
#include <stack>

void fixStack(std::list<CallTreeNode<IdaTraceFileRecord>*>& stack, const IdaTraceFileRecord& record)
{
  for (auto itCurr = stack.rbegin(); itCurr != stack.rend(); ++itCurr)
  {
    if ((*itCurr)->value.msAddress_func == record.msResult_other)
    {
      auto itCurrF = std::next(itCurr).base();
      auto iDist = std::distance(itCurrF, stack.end());

      if (iDist > 1)
      {
        std::cout << iDist << " : " << record.msResult_other << endl;
        std::cout << record.msAddress << endl << record.msInstruction << endl << record.msResult << endl;
      }

      stack.erase(std::next(itCurrF), stack.end());
      return;
    }
  }
}

int main(int argc, const char* argv[])
{
  struct CurrOpts
  {
    std::string sInputFile{};
    std::string sOutputFile{};
    std::string sFiltersFile{};
    std::string sColumnsFile{};
    std::string sType{ "all" };
  };

  auto parser = CmdOpts<CurrOpts>::Create({
      {"--input", &CurrOpts::sInputFile },
      {"--output", &CurrOpts::sOutputFile },
      {"--filters", &CurrOpts::sFiltersFile },
      {"--columns", &CurrOpts::sColumnsFile },
      {"--type", &CurrOpts::sType },
    });

  const auto options = parser->parse(argc, argv);

  std::cout << "input file = " << options.sInputFile << endl;
  std::cout << "output file = " << options.sOutputFile << endl;
  std::cout << "filters file = " << options.sFiltersFile << endl;
  std::cout << "requested type = " << options.sType << endl;

  std::vector<std::string> filterSkipResult;
  std::ifstream fileFilters(options.sFiltersFile);
  std::string sRow;
  while (std::getline(fileFilters, sRow))
  {
    if (!sRow.empty() && sRow.substr(0, 2) != "//")
    {
      filterSkipResult.push_back(sRow);
    }
  }
  fileFilters.close();

  std::vector<std::string> filterColumns;
  std::ifstream fileColumnsFilters(options.sColumnsFile);
  std::string sRow1;
  while (std::getline(fileColumnsFilters, sRow1))
  {
    if (!sRow1.empty() && sRow1.substr(0, 2) != "//")
    {
      filterColumns.push_back(sRow1);
    }
  }
  fileColumnsFilters.close();

  std::ifstream fileInput(options.sInputFile);

  /* Collect tree */
  CallTree<IdaTraceFileRecord> tree;
  tree.pRoot.reset(new CallTreeNode<IdaTraceFileRecord>());

  std::list<CallTreeNode<IdaTraceFileRecord>*> stack;
  stack.push_back(tree.pRoot.get());

  IdaTraceFileRecord recordPrev, recordCurr;
  CallTreeNode<IdaTraceFileRecord>* pNodePrev, * pNodeCurr;
  while (IdaTraceFileRecord::readLine(fileInput, recordCurr))
  {
    if (recordPrev.msInstruction_name == "call" && recordCurr.msResult_func != recordPrev.msResult_func)
    {
      stack.push_back(pNodePrev);
    }

    pNodeCurr = stack.back()->appendNode(recordCurr);

    if (recordCurr.msInstruction_name == "retn" || recordCurr.msInstruction_name == "bnd")
    {
      fixStack(stack, recordCurr);

      if (stack.size() > 1)
      {
        stack.pop_back();
      }
      else
      {
        pNodeCurr->appendNode(IdaTraceFileRecord("error", "error", "error", "error"));
      }
    }

    pNodePrev = pNodeCurr;
    recordPrev = recordCurr;
  }
  fileInput.close();

  /* Traverse and print */
  if (options.sType == "all" || options.sType == "text")
  {
    std::ofstream fileOutput(options.sOutputFile);
    IdaTreeTabbedPrinterContext context;
    context.pOutput = &fileOutput;
    context.iDepthPrev = 0;
    context.filterSkipResult = filterSkipResult;
    tree.traverse(&treeTabbedTextPrinter, context.iDepthPrev, &context);
    fileOutput.close();
  }
  if (options.sType == "all" || options.sType == "dot")
  {
    std::ofstream fileOutput(options.sOutputFile);
    IdaTreeDotPrinterContext context;
    context.pOutput = &fileOutput;
    context.iDepthPrev = 0;
    context.sPrev = "Begin";
    context.filterSkipResult = filterSkipResult;
    context.filterColumns = filterColumns;

    treeDotTextPrinterHeader(&context);
    tree.traverse(&treeDotTextPrinter, context.iDepthPrev, &context);
    treeDotTextPrinterFooter(&context);

    fileOutput.close();
  }


  // done
}

