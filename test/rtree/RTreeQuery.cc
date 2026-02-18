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

// NOTE: Please read README.txt before browsing this code.
#include <chrono>
#include <cstring>

// include library header file.
#include <spatialindex/SpatialIndex.h>

using namespace SpatialIndex;
using namespace SpatialIndex::RTree;  // need this to use Data. Otherwise you should use RTree:Data ...
using namespace std;

#define INSERT 1
#define DELETE 0
#define QUERY 2

// example of a Visitor pattern.
// findes the index and leaf IO for answering the query and prints
// the resulting data IDs to stdout.
class MyVisitor : public IVisitor
{
public:
	size_t m_indexIO{0};
	size_t m_leafIO{0};

public:
    MyVisitor() = default;

	void visitNode(const INode& n) override
	{
		if (n.isLeaf()) m_leafIO++;
		else m_indexIO++;
	}

	void visitData(const IData& d) override
	{
		IShape* pS;
		d.getShape(&pS);
			// do something.
		delete pS;

		// data should be an array of characters representing a Region as a string.
		uint8_t* pData = nullptr;
		uint32_t cLen = 0;
		d.getData(cLen, &pData);
		// do something.
		//string s = reinterpret_cast<char*>(pData);
		//cout << s << endl;
		delete[] pData;

		cout << d.getIdentifier()<<endl;
			// the ID of this data entry is an answer to the query. I will just print it to stdout.
	}

	void visitData(std::vector<const IData*>& v) override
	{
		cout << v[0]->getIdentifier() << " " << v[1]->getIdentifier() << endl;
	}
};

// Strategy for level-by-level printing:
// Program returns with InvalidPageException: Unknown page id 66. Because tries to read Page 66 which is the left-most child at the leaf.
// No problem this strategy is only for debugging..
class MyQueryStrategy1: public SpatialIndex::IQueryStrategy {
private:
	std::queue<id_type> ids;
	uint32_t curr_level{100};
public:
	void getNextEntry(const IEntry& entry, id_type& nextEntry, bool& hasNext) {
		const INode* n = dynamic_cast<const INode*>(&entry);

		if (n->getLevel() != curr_level) {
			curr_level = n->getLevel();
			std::cerr << "\nLevel: " << n->getLevel() << ": "; // unccomment to see all nodes at @level
		}
		for (uint32_t cChild = 0; cChild < n->getChildrenCount(); cChild++) {
			if (curr_level > 0){
				ids.push(n->getChildIdentifier(cChild));
				std::cerr << n->getChildIdentifier(cChild) << ",";
			}
			else if(curr_level == 0 && (cChild == 0 || cChild == n->getChildrenCount()-1)) std::cerr << n->getChildIdentifier(cChild) << "-";
		}


		std::cerr << " | ";   // unccomment to see all nodes at @level
		if (!ids.empty()) {
					nextEntry = ids.front();
					ids.pop();
					hasNext = true;
		}
		else {
			hasNext = false;
			std::cerr << std::endl;
		}
	}
};

// example of a Strategy pattern.
// find the total indexed space managed by the index (the MBR of the root).
class MyQueryStrategy2 : public IQueryStrategy
{
public:
	Region m_indexedSpace;

public:
	void getNextEntry(const IEntry& entry, id_type& /* nextEntry */, bool& hasNext) override
	{
		// the first time we are called, entry points to the root.

		// stop after the root.
		hasNext = false;

		IShape* ps;
		entry.getShape(&ps);
		ps->getMBR(m_indexedSpace);
		delete ps;
	}
};

// Strategy for printing Queries hold in the leaves.
// class MyQueryStrategy3: public SpatialIndex::IQueryStrategy {
// private:
// 	std::queue<id_type> ids;
// 	uint32_t query_count=0;
// public:
// 	void getNextEntry(const IEntry& entry, id_type& nextEntry, bool& hasNext) {
// 		const INodeQuery* n = dynamic_cast<const INodeQuery*>(&entry);
// 		// traverse only leaves
// 		if (n->getLevel() == 0) {
// 			std::cerr << " Leaf Node:"<<n->getIdentifier()<< " Number Of Queries Hold: "<< n->getQryChildrenCount() << ": ";
// 			for (auto i=0;i<n->getQryChildrenCount();i++){
// 				IShape* pS;
// 				n->getQryChildShape(i, &pS);
// 				Region r;
// 				pS->getMBR(r);
// 				std::cerr << r << ", ";
// 				query_count++;
// 			}
// 			std::cerr <<std::endl;
// 		}
// 		else{
// 			for (uint32_t cChild = 0; cChild < n->getChildrenCount();
// 								cChild++) {
// 							ids.push(n->getChildIdentifier(cChild));
// 						}
// 		}
// 		if (!ids.empty()) {
// 					nextEntry = ids.front();
// 					ids.pop();
// 					hasNext = true;
// 		}
// 		else {
// 			hasNext = false;
// 			std::cerr <<" ----> Total number of queries stored: " << query_count <<std::endl;
// 		}
// 	}
// };

class MyContainedInStrategy : public IQueryStrategy
{
private:
	const IShape* query;
	IVisitor* vis;
	queue<id_type> ids;

public:
	MyContainedInStrategy (const IShape& query,IVisitor& v){
		this->query=&query;
		this->vis=&v;
	}
	void getNextEntry(const IEntry& entry, id_type& nextEntry , bool& hasNext) override
	{
		// the first time we are called, entry points to the root.

		// I know  a Node object is sent.
		const INode* n = dynamic_cast<const INode*>(&entry);
		vis->visitNode(*n);

		IShape *cMBR;
		n->getShape(&cMBR);  // this is the current Node's MBR.

		// Always contains unless at the first iteration (i.e. we are in root) the query is out of coverage.
		if(!(cMBR->containsShape(*query))){
			cout << "query is out of indexed space " << endl;
			hasNext = false;
			return;
		}

		for (uint32_t cChild = 0; cChild < n->getChildrenCount(); cChild++) {

			n->getChildShape(cChild, &cMBR);
			if (cMBR->containsShape(*query)){  // Shape's functions are enough. No need to cast to Region..
				if (n->isIndex())  // No need to check n != nullptr
					ids.push(n->getChildIdentifier(cChild));
				else{

					//cout << n->getChildIdentifier(cChild)<<endl;
					// instead "visit" data!! Maybe we need to access object's details..

					uint8_t *childData=nullptr;  // must start with nullptr!
					uint32_t len;
					n->getChildData(cChild,len, &childData);  // already nullptr for now.

					IShape *childShape=nullptr;
					n->getChildShape(cChild, &childShape);

					// I cannot use PointerPools here. They are inside src/Rtree.h
//					RegionPtr mbr = m_regionPool.acquire();
//					shape.getMBR(*mbr);
					// Maybe I can use RegionPtr which is inside spatialindex/Region.h
//					RegionPtr* mbrr;
//					childShape->getMBR(**mbrr);

					// Both above is not available..
					Region* childMBR = dynamic_cast<Region*>(childShape);  // Rtree always hold MBR of the object

					Data data = Data(0, childData,*childMBR, n->getChildIdentifier(cChild));

					vis->visitData(data);

					if(childData != nullptr) delete childData;
					if(childShape != nullptr) delete childShape;
				}
			}
		}

		if (!ids.empty()) {
			nextEntry = ids.front();
			ids.pop();
			hasNext = true;
		} else {
			hasNext = false;
		}
		delete cMBR;
	}
};

int main(int argc, char** argv)
{
	try
	{
		if (argc != 5)
		{
			cerr << "Usage: " << argv[0] << " query_file tree_file cache_size query_type [intersection | 10NN | selfjoin | containedIn | printTree | printQueries ]." << endl;
			return -1;
		}

		uint32_t queryType = 0;
		uint32_t cache_size = atoi(argv[3]);  // max. number of idx-pages in the buffer
		if (strcmp(argv[4], "intersection") == 0) queryType = 0;
		else if (strcmp(argv[4], "10NN") == 0) queryType = 1;
		else if (strcmp(argv[4], "selfjoin") == 0) queryType = 2;
		else if (strcmp(argv[4], "containedIn") == 0) queryType = 3;  // Find objects that contains the query shape (point or region)
		else if (strcmp(argv[4], "internalNodesquery") == 0) queryType = 4;
		else if (strcmp(argv[4], "containsWhat") == 0) queryType = 5;  // Find objects that contains region query
		else if (strcmp(argv[4], "printTree") == 0) queryType = 6;  // Use this for debug tree internal..
		else if (strcmp(argv[4], "printQueries") == 0) queryType = 7;  // Use this for debug queries hold in the leaves..
		else
		{
			cerr << "Unknown query type." << endl;
			return -1;
		}

		string baseName = argv[2];
		IStorageManager* diskfile = StorageManager::loadDiskStorageManager(baseName);
			// this will try to locate and open an already existing storage manager.

		StorageManager::IBuffer* file = StorageManager::createNewRandomEvictionsBuffer(*diskfile, cache_size, false);  // original 10
			// applies a main memory random buffer on top of the persistent storage manager
			// (LRU buffer, etc can be created the same way).

		// If we need to open an existing tree stored in the storage manager, we only
		// have to specify the index identifier as follows
		ISpatialIndex* tree = RTree::loadRTree(*file, 1);

		// For debug
		if(queryType == 6){  // ignore queryfile..
			MyQueryStrategy1 qs;
			tree->queryStrategy(qs);
			cerr << *tree;
			bool ret = tree->isIndexValid();
			if (ret == false)
				std::cerr << "ERROR: Structure is invalid!" << std::endl;
			else
				std::cerr << "The stucture seems O.K." << std::endl;
			return 0;
		} // For debug..
		// else if(queryType == 7){
		// 	MyQueryStrategy3 qs;
		// 	tree->queryStrategy(qs);
		// 	cerr << *tree;
		// 	bool ret = tree->isIndexValid();
		// 	if (ret == false)
		// 		std::cerr << "ERROR: Structure is invalid!" << std::endl;
		// 	else
		// 		std::cerr << "The stucture seems O.K." << std::endl;
		// 	return 0;
		// }



		uint32_t op;
		string queries = argv[1];
		ifstream fin(queries);
		double x1, y1, x2, y2;
		if (!fin) {
			//			cerr << "Cannot open query file " << argv[1] << "." << endl;
			//			return -1;
			std::stringstream ss(queries);
			ss >> x1 >> y1 >> x2 >> y2;
			op = QUERY;

		}

		size_t count = 0;
		size_t indexIO = 0;
		size_t leafIO = 0;
		id_type id;

		double plow[2], phigh[2];

		auto t1 = std::chrono::high_resolution_clock::now(); //utku


		if (fin) fin >> op >> id >> x1 >> y1 >> x2 >> y2;
		while(true)
		{

			if (op == QUERY)
			{
				plow[0] = x1; plow[1] = y1;
				phigh[0] = x2; phigh[1] = y2;

				MyVisitor vis;
				if (queryType == 0)
				{
					Region r = Region(plow, phigh, 2);
					tree->intersectsWithQuery(r, vis); 	// this will find all data that "intersects" with the query range. RangeQueryType in RTreee is set as "IntersectionQuery" by default.
				}
				else if (queryType == 1)
				{
					Point p = Point(plow, 2);
					tree->nearestNeighborQuery(10, p, vis); // this will find the 10 nearest neighbors.
				}
				else if (queryType == 2)
				{
					Region r = Region(plow, phigh, 2);
					tree->selfJoinQuery(r, vis);
				}
				else if (queryType == 3)  // Find objects that contains the query shape (point or region)
				{
//					Point p = Point(plow, 2);
					Region r = Region(plow, phigh, 2);
					MyContainedInStrategy mystrategy(r,vis);
					tree->queryStrategy(mystrategy);
				}
				else if (queryType == 4)
				{
					Region r = Region(plow, phigh, 2);
					tree->internalNodesQuery(r,vis);
					// Retrieve internal nodes AND leaf nodes (ids) that is contained by query.
					// Selected Internal nodes are displayed 1 time, leaf node ids are displayed as number of times that shows the number of their children contained by the query.
					// Where to use ?
					// Has lockguard ? Why?
				}
				else //if (queryType == 5)
				{
					Region r = Region(plow, phigh, 2);
					tree->containsWhatQuery(r, vis); // this will find all data that is "contained" by the query range. Identical with intersection if RangeQueryType is set to ContainmentQuery.
				}

				indexIO += vis.m_indexIO;
				leafIO += vis.m_leafIO;
					// example of the Visitor pattern usage, for calculating how many nodes
					// were visited.
			}
			else
			{
				cerr << "This is not a query operation, or Query file is empty." << endl;
			}



			if ((count % 1000) == 0) cerr << count << endl;

			count++;

//			if (count == 14 )
//				std::cerr << "contrl" <<std::endl;
			if(!fin )
				break;
			else {
				fin >> op >> id >> x1 >> y1 >> x2 >> y2;
				if (!fin.good())
					break; // skip newlines, etc.
			}

			cout<< endl;  // run32-spatial-validation.py icin bunu yaptım. Sorgu sonuclarini ayırd etmeliyiz..
		}
		// utku:
		auto t2 = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast < std::chrono::milliseconds
				> (t2 - t1).count();
		std::cerr << "Time elapsed (msec) for query execution is : " << duration
				<< std::endl;

		cerr << *tree;
		cerr << "Operations: " << count << endl;
		cerr << "Index IO: " << indexIO << endl;
		cerr << "Leaf IO: " << leafIO << endl;
		cerr << "Buffer hits: " << file->getHits() << endl;
		// following is for a more elaborate stats. If you execute this code, it will traverse all the tree, it will impair you calculations..
//		MyQueryStrategy2 qs;
//		tree->queryStrategy(qs);
//		cerr << "Indexed space: " << qs.m_indexedSpace << endl;

		delete tree;
		delete file;
		delete diskfile;
			// delete the buffer first, then the storage manager
			// (otherwise the the buffer will fail writting the dirty entries).
	}
	catch (Tools::Exception& e)
	{
		cerr << "******ERROR******" << endl;
		std::string s = e.what();
		cerr << s << endl;
		return -1;
	}
	catch (...)
	{
		cerr << "******ERROR******" << endl;
		cerr << "other exception" << endl;
		return -1;
	}

	return 0;
}
