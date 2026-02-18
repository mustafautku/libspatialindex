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

class PQEntry {
	public:
		PQEntry(id_type nodeId, IShape* nodeMbr, uint32_t dataSize,const IShape& query) :
				m_nodeId(nodeId), m_nodeMbr(nodeMbr), m_dataSize(dataSize) {
			Region q;
			query.getMBR(q);  // need to take a "copy". Otherwise I lost the query region in the following iterations.

			m_queries.push_back(q);  // again another copy of q is stored. Because q will die.
		}

		struct SortDecending {
			bool operator()(const PQEntry *e1, const PQEntry *e2) {
//				if (e1->m_dataSize * e1->m_queries.size()
//						< e2->m_dataSize * e2->m_queries.size())
//					return true;
//				else
//					return false;
				double area1=e1->m_nodeMbr->getArea();
				double area2=e2->m_nodeMbr->getArea();
				if (e1->m_dataSize * e1->m_queries.size() / area1
						< e2->m_dataSize * e2->m_queries.size()  / area2)
					return true;
				else
					return false;
			}
		};

		id_type m_nodeId;
		IShape* m_nodeMbr;  // dyn. allocated.
		uint32_t m_dataSize;
		std::vector< Region> m_queries;  // NOT dynamically alocated objects. Yo do not need to delete while clearing the PQEntry!!
};

class MyVisitorForWL : public IVisitorWithQuery
{
public:
	size_t m_indexIO{0};
	size_t m_leafIO{0};
//	std::priority_queue<PQEntry, std::vector<PQEntry>, PQEntry::SortDecending> pq;
	std::vector<PQEntry*> nodes_vector;  // Since we sort, vector of pointers is better.
public:
	MyVisitorForWL()= default;

    void visitNode(const INode& n) override {
		if (n.isLeaf()) {
			m_leafIO++;
		} else
			m_indexIO++;
    }
	void visitNodeWithQuery(const INode& n, const IShape& query)
	{
		if (n.isLeaf()) {
			m_leafIO++;

			bool found=false;
			vector<PQEntry*>::iterator ptr;
			for (ptr = nodes_vector.begin(); ptr < nodes_vector.end(); ptr++)
				if((*ptr)->m_nodeId == n.getIdentifier()){
//					if(ptr->m_nodeId == 269)
//						std::cout<<""<<std::endl;

					Region q;
					query.getMBR(q);  // need to take a copy. Otherwise I lost the query region in the following iterations.
					(*ptr)->m_queries.push_back(q);
					// the other followings are needed only for the 1st query in the WL. May need to fixed..with dirty flag i.e to prevent unnecessary updates..
					(*ptr)->m_dataSize = n.getChildrenCount();
					found=true;
					break;

					// Test. Always updated NodeMBR. Because we already access nodeMBRs after loading and we clear nodesVector after each querying period.
//					IShape *nodeMBR;
//					n.getShape(&nodeMBR);
//					Region nodeMBRinLibrary;
//					nodeMBR->getMBR(nodeMBRinLibrary);
//					Region nodeMBRinQ;
//					(*ptr)->m_nodeMbr->getMBR(nodeMBRinQ);
//					assert(nodeMBRinLibrary == nodeMBRinQ);
//					break;

					// Thus no need to make the following update.  Read the previous comment!!
//					delete (*ptr)->m_nodeMbr;
//					(*ptr)->m_nodeMbr = cMBR;
				}
			if(!found){
				IShape *nodeMBR;
				n.getShape(&nodeMBR);  // takes a "copy". Dyn. allocation. We cannot take the address. This would mean to access library's address area. Here, we are in the client-side.
				nodes_vector.push_back(new PQEntry(n.getIdentifier(),nodeMBR ,n.getChildrenCount(),query));
			}
		}
		else m_indexIO++;
	}

	void visitData(const IData& d) override
	{
		IShape* pS;
		d.getShape(&pS);
			// do something.
		delete pS;  // yes we have to delete, because it the copy of coming data MBR.

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
	void printPQ(){
		cerr << " the PQ size: " << nodes_vector.size() << endl;
		vector<PQEntry*>::iterator ptr;
		for (ptr= nodes_vector.begin(); ptr != nodes_vector.end();ptr++)  // begin is the iterator (pointer) to the first element
			cerr << "Node Id: "<< (*ptr) ->m_nodeId << ", data size: " << (*ptr) ->m_dataSize << ", query size: " << (*ptr) ->m_queries.size() << ", Weigth:" << (*ptr) ->m_dataSize * (*ptr) ->m_queries.size() << endl;
	}
};


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

//void runTest(ISpatialIndex*,IVisitor&,ifstream&,bool);

int main(int argc, char** argv)
{
	try
	{
		if (argc != 6)
		{
			cerr << "Usage: " << argv[0] << " query_file tree_file cache_size query_type [intersection | ..] artificialSplit?" << endl;
			return -1;
		}


		string queries = argv[1];
		ifstream fin(queries);

		if (!fin) {
			cerr << "Cannot open query file " << argv[1] << "." << endl;
			return -1;
		//			std::stringstream ss(queries);
		//			ss >> x1 >> y1 >> x2 >> y2;
		//			op = QUERY;
		}

		uint32_t queryType = 0;
		uint32_t cache_size = atoi(argv[3]);  // max. number of idx-pages in the buffer
		if (strcmp(argv[4], "intersection") == 0) queryType = 0;
		else
		{
			cerr << "Unknown query type." << endl;
			return -1;
		}

		bool artificialSplit = atoi(argv[5]) ? true : false;

		string baseName = argv[2];
		IStorageManager* diskfile = StorageManager::loadDiskStorageManager(baseName);
			// this will try to locate and open an already existing storage manager.

		StorageManager::IBuffer* file = StorageManager::createNewRandomEvictionsBuffer(*diskfile, cache_size, false);  // original 10
			// applies a main memory random buffer on top of the persistent storage manager
			// (LRU buffer, etc can be created the same way).

		// If we need to open an existing tree stored in the storage manager, we only
		// have to specify the index identifier as follows
		ISpatialIndex* tree = RTree::loadRTree(*file, 1);
		Tools::PropertySet currentProp;
		Tools::Variant var;
		tree->getIndexProperties(currentProp);
		var=currentProp.getProperty("TreeVariant");

		if(var.m_val.lVal == RV_RSTAR){
			MyVisitor vis;
			uint32_t op;
			double x1, y1, x2, y2;
			size_t count = 0;
			size_t indexIO = 0;
			size_t leafIO = 0;
			id_type id;

			double plow[2], phigh[2];

			auto t0 = std::chrono::high_resolution_clock::now(); //utku
			auto t1 = std::chrono::high_resolution_clock::now(); //utku

			if (fin) fin >> op >> id >> x1 >> y1 >> x2 >> y2;
			while(true)
			{
				if (op == QUERY)
				{
					plow[0] = x1; plow[1] = y1;
					phigh[0] = x2; phigh[1] = y2;

		//					if (queryType == 0)
		//					{
					Region r = Region(plow, phigh, 2);
					tree->intersectsWithQuery(r, vis); 	// this will find all data that "intersects" with the query range. RangeQueryType in RTreee is set as "IntersectionQuery" by default.
		//					}
					indexIO += vis.m_indexIO;
					leafIO += vis.m_leafIO;
						// example of the Visitor pattern usage, for calculating how many nodes
						// were visited.
		//				cout << vis.nodes_vector.front()->m_queries[0];
				}
				else if (op == INSERT)
				{
					plow[0] = x1; plow[1] = y1;
					phigh[0] = x2;
					phigh[1] = y2;
					Region r = Region(plow, phigh, 2);

					std::ostringstream os;
					os << r;
					std::string data = os.str();
					tree->insertData(0, 0, r, id);
				}
				if(!fin )
					break;
				else {
					uint32_t currentmode=op;
					fin >> op >> id >> x1 >> y1 >> x2 >> y2;
					if(op == 1 && op !=currentmode ){  // end of querying.
						cout<<endl;
					}
					if (!fin.good())
						break; // skip newlines, etc.
					}
			}
		}
		else if(var.m_val.lVal == RV_INCREMENTAL){
			MyVisitorForWL vis;
			uint32_t op;
			double x1, y1, x2, y2;
			size_t count = 0;
			size_t indexIO = 0;
			size_t leafIO = 0;
			id_type id;

			double plow[2], phigh[2];

			auto t0 = std::chrono::high_resolution_clock::now(); //utku
			auto t1 = std::chrono::high_resolution_clock::now(); //utku

			if (fin) fin >> op >> id >> x1 >> y1 >> x2 >> y2;
			while(true)
			{
				if (op == QUERY)
				{
					plow[0] = x1; plow[1] = y1;
					phigh[0] = x2; phigh[1] = y2;

		//			if (queryType == 0)
		//			{
					Region r = Region(plow, phigh, 2);
					tree->intersectsWithQuery(r, vis); 	// this will find all data that "intersects" with the query range. RangeQueryType in RTreee is set as "IntersectionQuery" by default.
		//			}
					indexIO += vis.m_indexIO;
					leafIO += vis.m_leafIO;
						// example of the Visitor pattern usage, for calculating how many nodes
						// were visited.
		//				cout << vis.nodes_vector.front()->m_queries[0];
				}
				else if (op == INSERT)
				{
					plow[0] = x1; plow[1] = y1;
					phigh[0] = x2;
					phigh[1] = y2;
					Region r = Region(plow, phigh, 2);

					std::ostringstream os;
					os << r;
					std::string data = os.str();
					tree->insertData(0, 0, r, id);
				}

				if(!fin )
					break;
				else {
					uint32_t currentmode=op;
					fin >> op >> id >> x1 >> y1 >> x2 >> y2;
					if (!fin.good())
						break; // skip newlines, etc.
					if(op == 1 && op !=currentmode ){  // end of querying.
						auto t2 = std::chrono::high_resolution_clock::now();
						auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();  // milliseconds
		//					std::cerr << "Time elapsed (msec) for query execution is : "<< duration << std::endl;
						cerr << " End of Querying data with current total latency (i.e. thrput): " << duration << endl;

						vis.printPQ();

						if(artificialSplit){
							std::make_heap(vis.nodes_vector.begin(),vis.nodes_vector.end(), PQEntry::SortDecending());  // NOT COMPLETE SORT. We just need the best one..
							vis.printPQ();
							IShape *cMBR = 	(vis.nodes_vector.front())->m_nodeMbr;   // front return a ref to the first element, which is a pointer here.
							if((vis.nodes_vector.front())->m_dataSize >10){
								cerr << " !!!!!Splitting the Current max: " << (vis.nodes_vector.front())->m_nodeId << endl;
								tree->insertData(0,0,*cMBR,-1);  // SPLIT point..Send IShape& again. this time library takes the "copy" inside.
							}
							else
								cerr << " NO Splitting ! " <<endl;


							bool ret = tree->isIndexValid();
							if (ret == false){
								std::cerr << "ERROR: Structure is invalid!"
										<< std::endl;
								exit(-1);
							}
							else
								std::cerr << "The stucture seems O.K." << std::endl;

							vector<PQEntry*>::iterator ptr;
							for (ptr = vis.nodes_vector.begin(); ptr < vis.nodes_vector.end(); ptr++){
								(*ptr)->m_queries.clear();  // queries are stored statically. Vector calls Region's destuctor and deletes coordinates area.
								delete (*ptr)->m_nodeMbr;  // this is already "copy" of library's leaf-node MBR.
							}
							vis.nodes_vector.clear();
						}
						cerr << " -----------------------  " << endl;
						t1 = std::chrono::high_resolution_clock::now(); //utku
						cout<<endl;
					}
					else if(op == 2 && op !=currentmode ){
						auto t2 = std::chrono::high_resolution_clock::now();
						auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
						cerr<< "                        End of Loading data with current total latency: "<< duration << endl;
						cerr << " ------------------ " << endl;
						t1 = std::chrono::high_resolution_clock::now(); //utku
					}
				}
			}
		}
		else{
			cerr << "Invalid TREE type." << endl;
			return -1;
		}


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


