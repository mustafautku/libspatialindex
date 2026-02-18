/******************************************************************************
 * Project:  libspatialindex - A C++ library for spatial indexing
 * Author:   Marios Hadjieleftheriou, mhadji@gmail.com
 ******************************************************************************
 * Copyright (c) 2002, Marios Hadjieleftheriou
 *
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
******************************************************************************/

#include <cstring>

#include <spatialindex/SpatialIndex.h>

#include "RTree.h"
#include "Node.h"
#include "Index.h"
#include "Leaf.h"

using namespace SpatialIndex;
using namespace SpatialIndex::RTree;

Leaf::~Leaf()
= default;

Leaf::Leaf(SpatialIndex::RTree::RTree* pTree, id_type id): Node(pTree, id, 0, pTree->m_leafCapacity)
{
}

Leaf::Leaf(SpatialIndex::RTree::RTree* pTree, id_type id, uint32_t numChildren): Node(pTree, id, 0, numChildren)
{
}

NodePtr Leaf::chooseSubtree(const Region&, uint32_t, std::stack<id_type>&)
{
	// should make sure to relinquish other PoolPointer lists that might be pointing to the
	// same leaf.
	return NodePtr(this, &(m_pTree->m_leafPool));
}

NodePtr Leaf::findLeaf(const Region& mbr, id_type id, std::stack<id_type>&)
{
	for (uint32_t cChild = 0; cChild < m_children; ++cChild)
	{
		// should make sure to relinquish other PoolPointer lists that might be pointing to the
		// same leaf.
		if (m_pIdentifier[cChild] == id && mbr == *(m_ptrMBR[cChild])) return NodePtr(this, &(m_pTree->m_leafPool));
	}

	return NodePtr();
}

void Leaf::split(uint32_t dataLength, uint8_t* pData, Region& mbr, id_type id, NodePtr& pLeft, NodePtr& pRight)
{
	++(m_pTree->m_stats.m_u64Splits);

	std::vector<uint32_t> g1, g2;

	switch (m_pTree->m_treeVariant)
	{
		case RV_LINEAR:
		case RV_QUADRATIC:
			rtreeSplit(dataLength, pData, mbr, id, g1, g2);
			break;
		case RV_RSTAR:
		case RV_INCREMENTAL:
			if(id >= 0) rstarSplit(dataLength, pData, mbr, id, g1, g2);
			else Tools::NotSupportedException("Leaf::split: Tree variant not supported."); //rstarSplitByQuery(dataLength, pData, mbr, id, g1, g2);  // query triggers split.
			break;
		default:
			throw Tools::NotSupportedException("Leaf::split: Tree variant not supported.");
	}

	pLeft = m_pTree->m_leafPool.acquire();
	pRight = m_pTree->m_leafPool.acquire();

//	if (pLeft.get() == nullptr)
//		pLeft = NodePtr(new Leaf(m_pTree, -1), &(m_pTree->m_leafPool));
//	else
//		std::cerr << "used already exist" << std::endl;
//	if (pRight.get() == nullptr)
//		pRight = NodePtr(new Leaf(m_pTree, -1), &(m_pTree->m_leafPool));
//	else
//			std::cerr << "used already exist" << std::endl;

	if (pLeft.get() == nullptr){
		if(g1.size() <= m_pTree->m_leafCapacity)
			pLeft = NodePtr(new Leaf(m_pTree, -1), &(m_pTree->m_leafPool));
		else
			pLeft = NodePtr(new Leaf(m_pTree, -1,g1.size()), &(m_pTree->m_leafPool));
	}
// 	else{
// //		std::cerr << "used already exist mem: Needed:" << g1.size() << " , Acquired mem:"<< pLeft.get()->m_capacity << std::endl;
// 		if (g1.size() > pLeft.get()->m_capacity) {
// //			std::cerr << "         !!!!Refresh mem !!"<< std::endl;
// 			m_pTree->m_leafPool.refresh(pLeft.get(), g1.size()+1);
// 			//				n = NodePtr(new Leaf(this, -1,numChildren), &m_leafPool);
// 			//n = m_leafPool.acquire(numChildren);  // Search in the pool for this specific size.  another try maybe later.
// 		}
// 	}
	if (pRight.get() == nullptr){
			if(g2.size() <= m_pTree->m_leafCapacity)
				pRight = NodePtr(new Leaf(m_pTree, -1), &(m_pTree->m_leafPool));
			else
				pRight = NodePtr(new Leaf(m_pTree, -1,g2.size()), &(m_pTree->m_leafPool));
	}
// 	else{
// //		std::cerr << "used already exist mem: Needed:" << g2.size() << " , Acquired mem:"<< pRight.get()->m_capacity << std::endl;
// 		if (g2.size() > pRight.get()->m_capacity) {
// //			std::cerr << "         !!!!Refresh mem !!"<< std::endl;
// 			m_pTree->m_leafPool.refresh(pRight.get(), g2.size() + 1);
// 			//				n = NodePtr(new Leaf(this, -1,numChildren), &m_leafPool);
// 			//n = m_leafPool.acquire(numChildren);  // Search in the pool for this specific size.  another try maybe later.
// 		}
// 	}

	pLeft->m_nodeMBR = m_pTree->m_infiniteRegion;
	pRight->m_nodeMBR = m_pTree->m_infiniteRegion;

	uint32_t cIndex;

	for (cIndex = 0; cIndex < g1.size(); ++cIndex)
	{
		if( m_pIdentifier[g1[cIndex]] >= 0)
			pLeft->insertEntry(m_pDataLength[g1[cIndex]], m_pData[g1[cIndex]], *(m_ptrMBR[g1[cIndex]]), m_pIdentifier[g1[cIndex]]);
//		else
//			std::cerr << "caugth query" << std::endl;
		// we don't want to delete the data array from this node's destructor!
		m_pData[g1[cIndex]] = nullptr;
	}

	for (cIndex = 0; cIndex < g2.size(); ++cIndex)
	{
		if( m_pIdentifier[g2[cIndex]] >= 0)
			pRight->insertEntry(m_pDataLength[g2[cIndex]], m_pData[g2[cIndex]], *(m_ptrMBR[g2[cIndex]]), m_pIdentifier[g2[cIndex]]);
//		else
//					std::cerr << "caugth query" << std::endl;
		// we don't want to delete the data array from this node's destructor!
		m_pData[g2[cIndex]] = nullptr;
	}
}

void Leaf::deleteData(const Region& mbr, id_type id, std::stack<id_type>& pathBuffer)
{
	uint32_t child;

	for (child = 0; child < m_children; ++child)
	{
		if (m_pIdentifier[child] == id && mbr == *(m_ptrMBR[child])) break;
	}

	deleteEntry(child);
	m_pTree->writeNode(this);

	std::stack<NodePtr> toReinsert;
	NodePtr ptrThis(this, &(m_pTree->m_leafPool));
	condenseTree(toReinsert, pathBuffer, ptrThis);
	ptrThis.relinquish();

	// re-insert eliminated nodes.
	while (! toReinsert.empty())
	{
		NodePtr n = toReinsert.top(); toReinsert.pop();
		m_pTree->deleteNode(n.get());

		for (uint32_t cChild = 0; cChild < n->m_children; ++cChild)
		{
			// keep this in the for loop. The tree height might change after insertions.
			uint8_t* overflowTable = new uint8_t[m_pTree->m_stats.m_u32TreeHeight];
			memset(overflowTable, 0, m_pTree->m_stats.m_u32TreeHeight);
			m_pTree->insertData_impl(n->m_pDataLength[cChild], n->m_pData[cChild], *(n->m_ptrMBR[cChild]), n->m_pIdentifier[cChild], n->m_level, overflowTable);
			n->m_pData[cChild] = nullptr;
			delete[] overflowTable;
		}
		if (n.get() == this) n.relinquish();
	}
}

