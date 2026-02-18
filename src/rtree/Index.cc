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

#include <limits>

#include <cstring>
#include <spatialindex/SpatialIndex.h>
#include "RTree.h"
#include "Node.h"
#include "Leaf.h"
#include "Index.h"

#include <math.h>
#include <morton.h>

using namespace SpatialIndex;
using namespace SpatialIndex::RTree;

Index::~Index()
= default;

Index::Index(SpatialIndex::RTree::RTree* pTree, id_type id, uint32_t level) : Node(pTree, id, level, pTree->m_indexCapacity)
{
}

NodePtr Index::chooseSubtree(const Region& mbr, uint32_t insertionLevel, std::stack<id_type>& pathBuffer)
{
	if (m_level == insertionLevel) return NodePtr(this, &(m_pTree->m_indexPool));

	pathBuffer.push(m_identifier);

	uint32_t child = 0;

	switch (m_pTree->m_treeVariant)
	{
		case RV_LINEAR:
		case RV_QUADRATIC:
			child = findLeastEnlargement(mbr);
			break;
		case RV_RSTAR:
		case RV_INCREMENTAL:
			if (m_level == 1)
			{
				// if this node points to leaves...
				child = findLeastOverlap(mbr);
			}
			else
			{
				child = findLeastEnlargement(mbr);
			}
		break;
		default:
			throw Tools::NotSupportedException("Index::chooseSubtree: Tree variant not supported.");
	}
	assert(child != std::numeric_limits<uint32_t>::max());

	NodePtr n = m_pTree->readNode(m_pIdentifier[child]);
	NodePtr ret = n->chooseSubtree(mbr, insertionLevel, pathBuffer);
	assert(n.unique());
	if (ret.get() == n.get()) n.relinquish();

	return ret;
}

NodePtr Index::findLeaf(const Region& mbr, id_type id, std::stack<id_type>& pathBuffer)
{
	pathBuffer.push(m_identifier);

	for (uint32_t cChild = 0; cChild < m_children; ++cChild)
	{
		if (m_ptrMBR[cChild]->containsRegion(mbr))
		{
			NodePtr n = m_pTree->readNode(m_pIdentifier[cChild]);
			NodePtr l = n->findLeaf(mbr, id, pathBuffer);
			if (n.get() == l.get()) n.relinquish();
			if (l.get() != nullptr) return l;
		}
	}

	pathBuffer.pop();

	return NodePtr();
}

void Index::split(uint32_t dataLength, uint8_t* pData, Region& mbr, id_type id, NodePtr& ptrLeft, NodePtr& ptrRight)
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
		case RV_INCREMENTAL:  // may need to fix here!!
			rstarSplit(dataLength, pData, mbr, id, g1, g2);
			break;
		default:
			throw Tools::NotSupportedException("Index::split: Tree variant not supported.");
	}

	ptrLeft = m_pTree->m_indexPool.acquire();
	ptrRight = m_pTree->m_indexPool.acquire();

	if (ptrLeft.get() == nullptr) ptrLeft = NodePtr(new Index(m_pTree, m_identifier, m_level), &(m_pTree->m_indexPool));
	if (ptrRight.get() == nullptr) ptrRight = NodePtr(new Index(m_pTree, -1, m_level), &(m_pTree->m_indexPool));

	ptrLeft->m_nodeMBR = m_pTree->m_infiniteRegion;
	ptrRight->m_nodeMBR = m_pTree->m_infiniteRegion;

	uint32_t cIndex;

	for (cIndex = 0; cIndex < g1.size(); ++cIndex)
	{
		ptrLeft->insertEntry(0, nullptr, *(m_ptrMBR[g1[cIndex]]), m_pIdentifier[g1[cIndex]]);
	}

	for (cIndex = 0; cIndex < g2.size(); ++cIndex)
	{
		ptrRight->insertEntry(0, nullptr, *(m_ptrMBR[g2[cIndex]]), m_pIdentifier[g2[cIndex]]);
	}
}

void Index::splitOF(NodePtr*& leafNodes, uint64_t numOfLeafs, NodePtr& ptrLeft, NodePtr& ptrRight)
{
	++(m_pTree->m_stats.m_u64Splits);

	std::vector<uint32_t> g1, g2;

	switch (m_pTree->m_treeVariant)
	{
		case RV_LINEAR:
		case RV_QUADRATIC:
//			rtreeSplit(dataLength, pData, mbr, id, g1, g2);
			break;
		case RV_RSTAR:
		case RV_INCREMENTAL:  // may need to fix here!!
			rstarOFSplit(leafNodes,numOfLeafs, g1, g2);
			break;
		default:
			throw Tools::NotSupportedException("Index::split: Tree variant not supported.");
	}

	ptrLeft = m_pTree->m_indexPool.acquire();
	ptrRight = m_pTree->m_indexPool.acquire();

	if (ptrLeft.get() == nullptr) ptrLeft = NodePtr(new Index(m_pTree, m_identifier, m_level), &(m_pTree->m_indexPool));
	if (ptrRight.get() == nullptr) ptrRight = NodePtr(new Index(m_pTree, -1, m_level), &(m_pTree->m_indexPool));

	ptrLeft->m_nodeMBR = m_pTree->m_infiniteRegion;
	ptrRight->m_nodeMBR = m_pTree->m_infiniteRegion;

	uint32_t cIndex;

	for (cIndex = 0; cIndex < g1.size(); ++cIndex)
	{
		if(g1[cIndex] >= m_children){
			assert (g1[cIndex]-m_children+1 > 0);  // should never access leafNodes[0]. It is already in the node..
			ptrLeft->insertEntry(0, nullptr, leafNodes[g1[cIndex]-m_children+1].get()->m_nodeMBR, leafNodes[g1[cIndex]-m_children+1].get()->m_identifier);
		}
		else
			ptrLeft->insertEntry(0, nullptr, *(m_ptrMBR[g1[cIndex]]), m_pIdentifier[g1[cIndex]]);

	}

	for (cIndex = 0; cIndex < g2.size(); ++cIndex)
	{
		if(g2[cIndex] >= m_children){
			assert (g2[cIndex]-m_children+1 > 0);  // should never access leafNodes[0]. It is already in the node..
			ptrRight->insertEntry(0, nullptr, leafNodes[g2[cIndex]-m_children+1].get()->m_nodeMBR, leafNodes[g2[cIndex]-m_children+1].get()->m_identifier);
		}
		else
			ptrRight->insertEntry(0, nullptr, *(m_ptrMBR[g2[cIndex]]), m_pIdentifier[g2[cIndex]]);
	}
}

uint32_t Index::findLeastEnlargement(const Region& r) const
{
	double area = std::numeric_limits<double>::infinity();
	uint32_t best = std::numeric_limits<uint32_t>::max();

	RegionPtr t = m_pTree->m_regionPool.acquire();

	for (uint32_t cChild = 0; cChild < m_children; ++cChild)
	{
		m_ptrMBR[cChild]->getCombinedRegion(*t, r);

		double a = m_ptrMBR[cChild]->getArea();
		double enl = t->getArea() - a;

		if (enl < area)
		{
			area = enl;
			best = cChild;
		}
		else if (enl == area)
		{
			// this will rarely happen, so compute best area on the fly only
			// when necessary.
			if (enl == std::numeric_limits<double>::infinity()
			    || a < m_ptrMBR[best]->getArea()) best = cChild;
		}
	}

	return best;
}

uint32_t Index::findLeastOverlap(const Region& r) const
{
	OverlapEntry** entries = new OverlapEntry*[m_children];

	double leastOverlap = std::numeric_limits<double>::max();
	double me = std::numeric_limits<double>::max();
	OverlapEntry* best = nullptr;

	// find combined region and enlargement of every entry and store it.
	for (uint32_t cChild = 0; cChild < m_children; ++cChild)
	{
		try
		{
			entries[cChild] = new OverlapEntry();
		}
		catch (...)
		{
			for (uint32_t i = 0; i < cChild; ++i) delete entries[i];
			delete[] entries;
			throw;
		}

		entries[cChild]->m_index = cChild;
		entries[cChild]->m_original = m_ptrMBR[cChild];
		entries[cChild]->m_combined = m_pTree->m_regionPool.acquire();
		m_ptrMBR[cChild]->getCombinedRegion(*(entries[cChild]->m_combined), r);
		entries[cChild]->m_oa = entries[cChild]->m_original->getArea();
		entries[cChild]->m_ca = entries[cChild]->m_combined->getArea();
		entries[cChild]->m_enlargement = entries[cChild]->m_ca - entries[cChild]->m_oa;

		if (entries[cChild]->m_enlargement < me)
		{
			me = entries[cChild]->m_enlargement;
			best = entries[cChild];
		}
		else if (entries[cChild]->m_enlargement == me && entries[cChild]->m_oa < best->m_oa)
		{
			best = entries[cChild];
		}
	}

	if (me < -std::numeric_limits<double>::epsilon() || me > std::numeric_limits<double>::epsilon())
	{
		uint32_t cIterations;

		if (m_children > m_pTree->m_nearMinimumOverlapFactor)
		{
			// sort entries in increasing order of enlargement.
			::qsort(entries, m_children,
							sizeof(OverlapEntry*),
							OverlapEntry::compareEntries);
			assert(entries[0]->m_enlargement <= entries[m_children - 1]->m_enlargement);

			cIterations = m_pTree->m_nearMinimumOverlapFactor;
		}
		else
		{
			cIterations = m_children;
		}

		// calculate overlap of most important original entries (near minimum overlap cost).
		for (uint32_t cIndex = 0; cIndex < cIterations; ++cIndex)
		{
			double dif = 0.0;
			OverlapEntry* e = entries[cIndex];

			for (uint32_t cChild = 0; cChild < m_children; ++cChild)
			{
				if (e->m_index != cChild)
				{
					double f = e->m_combined->getIntersectingArea(*(m_ptrMBR[cChild]));
					if (f != 0.0) dif += f - e->m_original->getIntersectingArea(*(m_ptrMBR[cChild]));
				}
			} // for (cChild)

			if (dif < leastOverlap)
			{
				leastOverlap = dif;
				best = entries[cIndex];
			}
			else if (dif == leastOverlap)
			{
				if (e->m_enlargement == best->m_enlargement)
				{
					// keep the one with least area.
					if (e->m_original->getArea() < best->m_original->getArea()) best = entries[cIndex];
				}
				else
				{
					// keep the one with least enlargement.
					if (e->m_enlargement < best->m_enlargement) best = entries[cIndex];
				}
			}
		} // for (cIndex)
	}

	uint32_t ret = best->m_index;

	for (uint32_t cChild = 0; cChild < m_children; ++cChild)
	{
		delete entries[cChild];
	}
	delete[] entries;

	return ret;
}


//utku:
void Index::adjustTree(id_type child_id,const Region& child_mbr, std::stack<id_type>& pathBuffer, bool force)
{
	++(m_pTree->m_stats.m_u64Adjustments);

	// find entry pointing to old node;
	uint32_t child;
	for (child = 0; child < m_children; ++child)
	{
		if (m_pIdentifier[child] == child_id) break;
	}

	// MBR needs recalculation if either:
	//   1. the NEW child MBR is not contained.
	//   2. the OLD child MBR is touching.
	bool bContained = m_nodeMBR.containsRegion(child_mbr);
	bool bTouches = m_nodeMBR.touchesRegion(*(m_ptrMBR[child]));
	bool bRecompute = (! bContained || (bTouches && m_pTree->m_bTightMBRs));

	*(m_ptrMBR[child]) =child_mbr;

	if (bRecompute || force)
	{
		for (uint32_t cDim = 0; cDim < m_nodeMBR.m_dimension; ++cDim)
		{
			m_nodeMBR.m_pLow[cDim] = std::numeric_limits<double>::max();
			m_nodeMBR.m_pHigh[cDim] = -std::numeric_limits<double>::max();

			for (uint32_t cChild = 0; cChild < m_children; ++cChild)
			{
				m_nodeMBR.m_pLow[cDim] = std::min(m_nodeMBR.m_pLow[cDim], m_ptrMBR[cChild]->m_pLow[cDim]);
				m_nodeMBR.m_pHigh[cDim] = std::max(m_nodeMBR.m_pHigh[cDim], m_ptrMBR[cChild]->m_pHigh[cDim]);
			}
		}
	}

	m_pTree->writeNode(this);

	if ((bRecompute || force) && (! pathBuffer.empty()))
	{
		id_type cParent = pathBuffer.top(); pathBuffer.pop();
		NodePtr ptrN = m_pTree->readNode(cParent);
		Index* p = static_cast<Index*>(ptrN.get());
		p->adjustTree(this->m_identifier,this->m_nodeMBR, pathBuffer, force);
	}
}

void Index::adjustTree(Node* n, std::stack<id_type>& pathBuffer, bool force)
{
	++(m_pTree->m_stats.m_u64Adjustments);

	// find entry pointing to old node;
	uint32_t child;
	for (child = 0; child < m_children; ++child)
	{
		if (m_pIdentifier[child] == n->m_identifier) break;
	}

	// MBR needs recalculation if either:
	//   1. the NEW child MBR is not contained.
	//   2. the OLD child MBR is touching.
	bool bContained = m_nodeMBR.containsRegion(n->m_nodeMBR);
	bool bTouches = m_nodeMBR.touchesRegion(*(m_ptrMBR[child]));
	bool bRecompute = (! bContained || (bTouches && m_pTree->m_bTightMBRs));

	*(m_ptrMBR[child]) = n->m_nodeMBR;

	if (bRecompute || force)
	{
		for (uint32_t cDim = 0; cDim < m_nodeMBR.m_dimension; ++cDim)
		{
			m_nodeMBR.m_pLow[cDim] = std::numeric_limits<double>::max();
			m_nodeMBR.m_pHigh[cDim] = -std::numeric_limits<double>::max();

			for (uint32_t cChild = 0; cChild < m_children; ++cChild)
			{
				m_nodeMBR.m_pLow[cDim] = std::min(m_nodeMBR.m_pLow[cDim], m_ptrMBR[cChild]->m_pLow[cDim]);
				m_nodeMBR.m_pHigh[cDim] = std::max(m_nodeMBR.m_pHigh[cDim], m_ptrMBR[cChild]->m_pHigh[cDim]);
			}
		}
	}

	m_pTree->writeNode(this);

	if ((bRecompute || force) && (! pathBuffer.empty()))
	{
		id_type cParent = pathBuffer.top(); pathBuffer.pop();
		NodePtr ptrN = m_pTree->readNode(cParent);
		Index* p = static_cast<Index*>(ptrN.get());
		p->adjustTree(this, pathBuffer, force);
	}
}

void Index::adjustTree(id_type child_id1,id_type child_id2, Region& child_mbr1, Region& child_mbr2, std::stack<id_type>& pathBuffer, uint8_t* overflowTable)
{
	++(m_pTree->m_stats.m_u64Adjustments);

	// find entry pointing to old node;
	uint32_t child;
	for (child = 0; child < m_children; ++child)
	{
		if (m_pIdentifier[child] == child_id1) break;
	}

	// MBR needs recalculation if either:
	//   1. either child MBR is not contained.
	//   2. the OLD child MBR is touching.
	bool bContained1 = m_nodeMBR.containsRegion(child_mbr1);
	bool bContained2 = m_nodeMBR.containsRegion(child_mbr2);  // sanki buna gerek yok gibi. Zaten sonra insert edecek ???
	bool bContained = bContained1 && bContained2;
	bool bTouches = m_nodeMBR.touchesRegion(*(m_ptrMBR[child]));
	bool bRecompute = (! bContained || (bTouches && m_pTree->m_bTightMBRs));

	*(m_ptrMBR[child]) = child_mbr1;

	if (bRecompute)
	{
		for (uint32_t cDim = 0; cDim < m_nodeMBR.m_dimension; ++cDim)
		{
			m_nodeMBR.m_pLow[cDim] = std::numeric_limits<double>::max();
			m_nodeMBR.m_pHigh[cDim] = -std::numeric_limits<double>::max();

			for (uint32_t cChild = 0; cChild < m_children; ++cChild)
			{
				m_nodeMBR.m_pLow[cDim] = std::min(m_nodeMBR.m_pLow[cDim], m_ptrMBR[cChild]->m_pLow[cDim]);
				m_nodeMBR.m_pHigh[cDim] = std::max(m_nodeMBR.m_pHigh[cDim], m_ptrMBR[cChild]->m_pHigh[cDim]);
			}
		}
	}

	// No write necessary here. insertData will write the node if needed.
	//m_pTree->writeNode(this);

	bool bAdjusted = insertData(0, nullptr, child_mbr2, child_id2, pathBuffer, overflowTable);

	// if n2 is contained in the node and there was no split or reinsert,
	// we need to adjust only if recalculation took place.
	// In all other cases insertData above took care of adjustment.
	if ((! bAdjusted) && bRecompute && (! pathBuffer.empty()))
	{
		id_type cParent = pathBuffer.top(); pathBuffer.pop();
		NodePtr ptrN = m_pTree->readNode(cParent);
		Index* p = static_cast<Index*>(ptrN.get());
		p->adjustTree(this, pathBuffer);
	}
}

void Index::adjustTree(Node* n1, Node* n2, std::stack<id_type>& pathBuffer, uint8_t* overflowTable)
{
	++(m_pTree->m_stats.m_u64Adjustments);

	// find entry pointing to old node;
	uint32_t child;
	for (child = 0; child < m_children; ++child)
	{
		if (m_pIdentifier[child] == n1->m_identifier) break;
	}

	// MBR needs recalculation if either:
	//   1. either child MBR is not contained.
	//   2. the OLD child MBR is touching.
	bool bContained1 = m_nodeMBR.containsRegion(n1->m_nodeMBR);
	bool bContained2 = m_nodeMBR.containsRegion(n2->m_nodeMBR);
	bool bContained = bContained1 && bContained2;
	bool bTouches = m_nodeMBR.touchesRegion(*(m_ptrMBR[child]));
	bool bRecompute = (! bContained || (bTouches && m_pTree->m_bTightMBRs));

	*(m_ptrMBR[child]) = n1->m_nodeMBR;

	if (bRecompute)
	{
		for (uint32_t cDim = 0; cDim < m_nodeMBR.m_dimension; ++cDim)
		{
			m_nodeMBR.m_pLow[cDim] = std::numeric_limits<double>::max();
			m_nodeMBR.m_pHigh[cDim] = -std::numeric_limits<double>::max();

			for (uint32_t cChild = 0; cChild < m_children; ++cChild)
			{
				m_nodeMBR.m_pLow[cDim] = std::min(m_nodeMBR.m_pLow[cDim], m_ptrMBR[cChild]->m_pLow[cDim]);
				m_nodeMBR.m_pHigh[cDim] = std::max(m_nodeMBR.m_pHigh[cDim], m_ptrMBR[cChild]->m_pHigh[cDim]);
			}
		}
	}

	// No write necessary here. insertData will write the node if needed.
	//m_pTree->writeNode(this);

	bool bAdjusted = insertData(0, nullptr, n2->m_nodeMBR, n2->m_identifier, pathBuffer, overflowTable);

	// if n2 is contained in the node and there was no split or reinsert,
	// we need to adjust only if recalculation took place.
	// In all other cases insertData above took care of adjustment.
	if ((! bAdjusted) && bRecompute && (! pathBuffer.empty()))
	{
		id_type cParent = pathBuffer.top(); pathBuffer.pop();
		NodePtr ptrN = m_pTree->readNode(cParent);
		Index* p = static_cast<Index*>(ptrN.get());
		p->adjustTree(this, pathBuffer);
	}
}

void Index::adjustTree(NodePtr*& newNodes, uint64_t numOfNodes, std::stack<id_type>& pathBuffer, uint8_t* overflowTable)
{
	++(m_pTree->m_stats.m_u64Adjustments);

	// find entry pointing to old node;  I assgined old leaf's id to the firstLeaf. but only id. All new leaf nodeMBR are updated..
	Node* firstNode=newNodes[0].get();
	uint32_t child;
	for (child = 0; child < m_children; ++child)
	{
		if (m_pIdentifier[child] == firstNode->m_identifier) break;
	}

	// MBR needs recalculation if either:
	//   1. either child MBR is not contained.
	//   2. the OLD child MBR is touching.

	// Simdilik bu assertion saglaniyor. O yuzden comment ile kapatıyorum..Ta ki zaman almasın..Tasarımdaki bir degisiklikte tekrar burayı kullanırsın..
	bool bContained = m_nodeMBR.containsRegion(firstNode->m_nodeMBR);
//	for(uint64_t i=1;i<numOfLeafs;i++){
//	       bContained = m_nodeMBR.containsRegion(leafNodes[i].get()->m_nodeMBR);
//	       if(!bContained) {
//	    	   bool bTouches = m_nodeMBR.touchesRegion(leafNodes[i].get()->m_nodeMBR);
//	    	   break;
//	       }
//	}
//	assert( bContained == true);  // cunku bir ekleme neticesinde bu noktaya gelmis degilim..Su an sorgu modundadyım ve split_ditribute yapmaya calisiyorum..

	bool bTouches = m_nodeMBR.touchesRegion(*(m_ptrMBR[child]));
	bool bRecompute = (! bContained || (bTouches && m_pTree->m_bTightMBRs));

	*(m_ptrMBR[child]) = firstNode->m_nodeMBR;  // eski leafNodeMBR is being lost here..

	// bunu yapmaya gerek yok gibi. Tam anlasilmiyor. en azindan validate ediyor. Kucuk bir ihtimal query latecy e etkisi var mı? olsa da cok az kanaatim..
	if (bRecompute)
	{
		for (uint32_t cDim = 0; cDim < m_nodeMBR.m_dimension; ++cDim)
		{
			m_nodeMBR.m_pLow[cDim] = std::numeric_limits<double>::max();
			m_nodeMBR.m_pHigh[cDim] = -std::numeric_limits<double>::max();

			for (uint32_t cChild = 0; cChild < m_children; ++cChild)
			{
				m_nodeMBR.m_pLow[cDim] = std::min(m_nodeMBR.m_pLow[cDim], m_ptrMBR[cChild]->m_pLow[cDim]);
				m_nodeMBR.m_pHigh[cDim] = std::max(m_nodeMBR.m_pHigh[cDim], m_ptrMBR[cChild]->m_pHigh[cDim]);
			}
		}
	}

	// No write necessary here. insertData will write the node if needed.
	//m_pTree->writeNode(this);

	if( m_children + numOfNodes -1 <= m_capacity){  // eklediğimde en fazla m_capacity olabilirr.
		uint64_t curidx=1;
//		bool b = false;
		for(;curidx<numOfNodes;curidx++){
					// No need to check this.. bool b = m_nodeMBR.containsRegion(mbr);
					insertEntry(0, nullptr, newNodes[curidx].get()->m_nodeMBR, newNodes[curidx].get()->m_identifier);
		}
		m_pTree->writeNode(this);  // we need this. Because we will redistribute the remaining leafs throughtout the m_level=1
		return;// we are done..
	}
	else if ( m_children + numOfNodes -1 < m_capacity * 1.8){
				NodePtr n;
				NodePtr nn;
				this->splitOF(newNodes, numOfNodes, n, nn);
				assert (n->m_children + nn->m_children  == m_children + numOfNodes -1 );
				if (pathBuffer.empty())
				{
					n->m_level = m_level;
					nn->m_level = m_level;
					n->m_identifier = -1;
					nn->m_identifier = -1;
					m_pTree->writeNode(n.get());
					m_pTree->writeNode(nn.get());

					NodePtr ptrR = m_pTree->m_indexPool.acquire();
					if (ptrR.get() == nullptr)
					{
						ptrR = NodePtr(new Index(m_pTree, m_pTree->m_rootID, m_level + 1), &(m_pTree->m_indexPool));
					}
					else
					{
						//ptrR->m_pTree = m_pTree;
						ptrR->m_identifier = m_pTree->m_rootID;
						ptrR->m_level = m_level + 1;
						ptrR->m_nodeMBR = m_pTree->m_infiniteRegion;
					}

					ptrR->insertEntry(0, nullptr, n->m_nodeMBR, n->m_identifier);
					ptrR->insertEntry(0, nullptr, nn->m_nodeMBR, nn->m_identifier);

					m_pTree->writeNode(ptrR.get());

					m_pTree->m_stats.m_nodesInLevel[m_level] = 2;
					m_pTree->m_stats.m_nodesInLevel.push_back(1);
					m_pTree->m_stats.m_u32TreeHeight = m_level + 2;
				}
				else
				{
					n->m_level = m_level;
					nn->m_level = m_level;
					n->m_identifier = m_identifier;
					nn->m_identifier = -1;

					m_pTree->writeNode(n.get());
					m_pTree->writeNode(nn.get());

					id_type cParent = pathBuffer.top(); pathBuffer.pop();
					NodePtr ptrN = m_pTree->readNode(cParent);
					Index* p = static_cast<Index*>(ptrN.get());
					p->adjustTree(n.get(), nn.get(), pathBuffer, overflowTable);
				}
		return;
	}

	/* SOL #1: All nodes generated STR-style and propagaate to upwards..No reinserts!!
				// external sort prepare!
				uint32_t pageSize=10000;
				uint32_t bufferPages=100;
				uint32_t bleaf = static_cast<uint32_t>(std::floor(static_cast<double>( m_pTree->m_leafCapacity* m_pTree->m_fillFactor)));

				std::shared_ptr<ExternalSorter> es = std::shared_ptr<ExternalSorter>(new ExternalSorter(pageSize, bufferPages));

				uint32_t cIndex;

				for (cIndex = 0; cIndex < m_children; ++cIndex)
				{
					es->insert(new ExternalSorter::Record(*m_ptrMBR[cIndex] , m_pIdentifier[cIndex], 0, nullptr, 0));
				}

				for (cIndex = 1; cIndex < numOfNodes ; ++cIndex)  // boylece geri kalan "numOfLeafs -1" tanesini paketliyorum..indis lere dikkat..
				{
					es->insert(new ExternalSorter::Record(newNodes[cIndex].get()->m_nodeMBR, newNodes[cIndex].get()->m_identifier, 0, nullptr, 0));  // dim=0 e gore sıralama..
				}
				es->sort();

				uint64_t P = static_cast<uint64_t>(std::ceil(static_cast<double>(es->getTotalEntries()) / static_cast<double>(bleaf)));

				NodePtr* newNodes2=new NodePtr[P];
				m_pTree->m_stats.m_u64Splits += (P-1);

				// utku:  use a dummy NodePtr to transfer m_level and m_identifier of the OFed index node..!!!
				NodePtr curNode=m_pTree->m_indexPool.acquire();
				if (curNode.get() == nullptr) curNode = NodePtr(new Index(m_pTree, -1, m_level), &(m_pTree->m_indexPool));
				curNode->m_level=m_level;
				curNode->m_identifier=m_identifier;
				m_pTree->generateOFNodes(es,newNodes2, curNode);


				if (!pathBuffer.empty())
				{
					id_type cParent = pathBuffer.top(); pathBuffer.pop();
					NodePtr ptrN = m_pTree->readNode(cParent);
					Index* p = static_cast<Index*>(ptrN.get());
					p->adjustTree(newNodes2, P,pathBuffer, overflowTable);
					delete[] newNodes2;
				}
				else{// we are at root !!  continue to STR-tree building levels.!!
					exit(-1);   // need a new STR-tree
				}
*/



/* SOL #2:: */
			std::vector<uint32_t> vReinsert, vKeep;
			this->reinsertOFData(newNodes, numOfNodes, vReinsert, vKeep);

			uint32_t pageSize=10000;
			uint32_t bufferPages=100;
			//uint32_t leafCapacity=90;  //pTree->m_leafCapacity
			double fillFactor=0.99;
			uint32_t bleaf = static_cast<uint32_t>(std::floor(static_cast<double>( m_pTree->m_leafCapacity* fillFactor)));

			std::shared_ptr<ExternalSorter> es = std::shared_ptr<ExternalSorter>(new ExternalSorter(pageSize, bufferPages));

			uint32_t lReinsert = static_cast<uint32_t>(vReinsert.size());
			uint32_t lKeep = static_cast<uint32_t>(vKeep.size());

			uint32_t cIndex;


			for (cIndex = 0; cIndex < lKeep; ++cIndex)
			{
				if(vKeep[cIndex] >= m_children){
					es->insert(new ExternalSorter::Record(newNodes[vKeep[cIndex]-m_children+1].get()->m_nodeMBR, newNodes[vKeep[cIndex]-m_children+1].get()->m_identifier, 0, nullptr, 0));  // dim=0 e gore sıralama..
					assert (vKeep[cIndex]-m_children+1 > 0); // should never access newNodes[0]
				}
				else
					es->insert(new ExternalSorter::Record(*m_ptrMBR[vKeep[cIndex]] , m_pIdentifier[vKeep[cIndex]], 0, nullptr, 0));
			}
			es->sort();

			uint64_t P = static_cast<uint64_t>(std::ceil(static_cast<double>(es->getTotalEntries()) / static_cast<double>(bleaf)));

			NodePtr* newNodes2=new NodePtr[P];
			m_pTree->m_stats.m_u64Splits += (P-1);

			// utku:  use a dummy NodePtr to transfer m_level and m_identifier of the OFed index node..!!!
			NodePtr curNode=m_pTree->m_indexPool.acquire();
			if (curNode.get() == nullptr) curNode = NodePtr(new Index(m_pTree, -1, m_level), &(m_pTree->m_indexPool));
			curNode->m_level=m_level;
			curNode->m_identifier=m_identifier;
			m_pTree->generateOFNodes(es,newNodes2, curNode);

			// DEBUG::			Ornek olcum : 3 seviyeli 40000 yukelemeli bir agacta 110 / 138  tanesini   "buradaki P tane yeni düğümün dışındaki yerlere" dağıttı..
//			std::cout <<std::endl << "level:"<<m_level<< ": ";
//			for(uint64_t i=0;i<P;i++)
//				std::cout << newNodes2[i]->m_identifier << ",";
//			std::cout <<" ?:"<<std::endl;

			if (!pathBuffer.empty())
			{
				id_type cParent = pathBuffer.top(); pathBuffer.pop();
				NodePtr ptrN = m_pTree->readNode(cParent);
				Index* p = static_cast<Index*>(ptrN.get());
				p->adjustTree(newNodes2, P,pathBuffer, overflowTable);
				delete[] newNodes2;
			}
			else{// we are at root !!  continue to STR-tree building levels.!!
				exit(-1);   // need a new STR-tree
			}




			// now reinserts...
			// vReinsert   holds entries by ascending distance. thus start scan from the last. This ensure the possiblity to distribute..
			overflowTable[m_level] = 1;  // signal that ı am redistrbuting !!!
			for (cIndex = 0; cIndex < lReinsert; ++cIndex)
//			for (cIndex = lReinsert-1; cIndex>=0 ; --cIndex)  //  Boyle yaparsam calismiyor.. bilmiyorum neden?
			{
				if(vReinsert[cIndex] >= m_children){
					assert (vReinsert[cIndex]-m_children+1 > 0);  // should never access leafNodes[0]. It is already in the node..
					m_pTree->insertData_impl(
										0, nullptr,
										newNodes[vReinsert[cIndex]-m_children+1].get()->m_nodeMBR, newNodes[vReinsert[cIndex]-m_children+1].get()->m_identifier,
										m_level, overflowTable);
				}
				else{
					m_pTree->insertData_impl(
										0, nullptr,
										*m_ptrMBR[vReinsert[cIndex]] , m_pIdentifier[vReinsert[cIndex]],
										m_level, overflowTable);
				}
			}



	/* SOl #3 :: ONLY REDISTRIBUTE..
//	bool bAdjusted = insertData(0, nullptr, n2->m_nodeMBR, n2->m_identifier, pathBuffer, overflowTable);
	uint64_t curLeafidx=1;
	for(;curLeafidx<numOfLeafs;curLeafidx++){
		if ( m_children < m_capacity)  // no need to mention m_OFchildren!! here is index..
			{
				// No need to check this.. bool b = m_nodeMBR.containsRegion(mbr);
				insertEntry(0, nullptr, leafNodes[curLeafidx].get()->m_nodeMBR, leafNodes[curLeafidx].get()->m_identifier);
			}
		else break;
	}
	m_pTree->writeNode(this);  // we need this. Because we will redistribute the remaining leafs throughtout the m_level=1
	if(curLeafidx == numOfLeafs){
//	m_pTree->writeNode(this);
	return;}// we are done..

	assert(m_level==1);
	overflowTable[m_level] = 1;  // signal that ı am redistrbuting !!!
	for(;curLeafidx<numOfLeafs;curLeafidx++){
//		bool bAdjusted = insertData(0, nullptr, leafNodes[curLeafidx].get()->m_nodeMBR, leafNodes[curLeafidx].get()->m_identifier, pathBuffer, overflowTable);
		m_pTree->insertData_impl(
						0, nullptr,
						leafNodes[curLeafidx].get()->m_nodeMBR, leafNodes[curLeafidx].get()->m_identifier,
						m_level, overflowTable);
//		if ((! bAdjusted) && bRecompute && (! pathBuffer.empty()))
//			{
//				id_type cParent = pathBuffer.top(); pathBuffer.pop();
//				NodePtr ptrN = m_pTree->readNode(cParent);
//				Index* p = static_cast<Index*>(ptrN.get());
//				p->adjustTree(this, pathBuffer);
//			}
	}

	**/
}

