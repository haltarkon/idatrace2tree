#pragma once

#include <list>
#include <memory>

template<class T>
struct CallTreeNode;

template<class T>
using CallTreeCallback = bool(*)(CallTreeNode<T>&, int iDepth, void* pContext);

template<class T>
struct CallTreeNode
{
  CallTreeNode* pParent = nullptr;
  std::list<CallTreeNode> childs;
  T value;

  CallTreeNode* appendNode(T record)
  {
    CallTreeNode node;
    node.pParent = this;
    node.value = record;
    childs.push_back(node);
    return &childs.back();
  }

  bool traverse(CallTreeCallback<T> callback, int iDepth, void* pContext)
  {
    if (!callback(*this, iDepth, pContext))
    {
      return false;
    }

    ++iDepth;

    for (auto& child : childs)
    {
      child.traverse(callback, iDepth, pContext);
    }
    return true;
  }
};

template<class T>
struct CallTree
{
  std::unique_ptr<CallTreeNode<T>> pRoot = nullptr;

  bool traverse(CallTreeCallback<T> callback, int iDepth, void* pContext)
  {
    return pRoot->traverse(callback, iDepth, pContext);
  }
};
