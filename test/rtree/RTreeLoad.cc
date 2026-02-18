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

#include <cstring>
#include <ctime>
#include <chrono>

// include library header file.
#include <spatialindex/SpatialIndex.h>

using namespace SpatialIndex;
using namespace std::chrono;

#define INSERT 1
#define DELETE 0
#define QUERY 2

#define NDEBUG  // use it for debugging in test ( not for libarary )!!  If you want to debug s/t in this test, define it (uncomment), then it will execute assertions.
// But NOT those in library. To (de)activate this in libarary, you have configure cmake. Read "myNotes for cmake"..


// Strategy for traversing LEAVES ONLY. write Leaves' MBR to std output, cout. Bash script redirect cout to a "pltDynLevel0" file
class MyQueryStrategy3: public SpatialIndex::IQueryStrategy {

private:
	std::queue<id_type> ids;
	uint32_t curr_level{100};
public:
	void getNextEntry(const IEntry& entry, id_type& nextEntry, bool& hasNext) {
		IShape* ps;
		entry.getShape(&ps);
		Region* pr = dynamic_cast<Region*>(ps);

		const INode* n = dynamic_cast<const INode*>(&entry);
		// traverse only index nodes at levels 1 and higher.
		if (n != 0 && n->getLevel() > 0) {
			if( n->getLevel() != curr_level){
				curr_level = n->getLevel();
//				std::cerr << "\nLevel: "<< n->getLevel()<< ": ";   // unccomment to see all nodes at @level
			}
			for (uint32_t cChild = 0; cChild < n->getChildrenCount();
					cChild++) {
				ids.push(n->getChildIdentifier(cChild));
//				std::cerr << n->getChildIdentifier(cChild) <<", ";  // unccomment to see all nodes at @level
			}
		}
		else{
//			std::cout << n->getChildrenCount() << std::endl;  // Always at most "capacity" entries at 1-by-1 Load.
			// for python -style modificaiotn
//			std::cout << (double)pr->m_pLow[0] << " " << (double)pr->m_pLow[1] << std::endl;
//			std::cout << (double)pr->m_pHigh[0] << " " << (double)pr->m_pLow[1] << std::endl;
//			std::cout << (double)pr->m_pHigh[0] << " " << (double)pr->m_pHigh[1] << std::endl;
//			std::cout << (double)pr->m_pLow[0] << " " << (double)pr->m_pHigh[1] << std::endl;
//			std::cout << (double)pr->m_pLow[0] << " " << (double)pr->m_pLow[1] << std::endl << std::endl;
			std::cout << 0 << "  " << 0 << "   " << (double)pr->m_pLow[0] << " " << (double)pr->m_pLow[1]<< " "
					  << (double)pr->m_pHigh[0] << " " << (double)pr->m_pHigh[1] << std::endl;
		}
//		std::cerr << " | ";   // unccomment to see all nodes at @level
		if (!ids.empty()) {
					nextEntry = ids.front();
					ids.pop();
					hasNext = true;
		}
		else hasNext = false;
		delete ps;
	}
};

int main(int argc, char** argv)
{
	id_type indexIdentifier;
	try
	{

//		if (argc != 8)
//		{													//  1		2			3				4			5		6			7              8
//			std::cerr << "Usage: " << argv[0] << "         input_file tree_file   idx_pageSize  cache_size capacity isPlotting. isIncremental  indexIdentifier" << std::endl;
//			return -1;
//		}


		bool isPlot;
		bool onlyPlotting = false;
		std::ifstream fin(argv[1]);
		if (!fin) {
			std::cerr << "Cannot open data file..Plotting? " << argv[1] << "."<< std::endl;
			isPlot = atoi(argv[6])?true:false;
			if(argc != 9 || !isPlot){   // you must provide idxIdentifier when ONLY plotting.
				std::cerr << " Wrong usage!! " << std::endl;
				return -1;
			}
			onlyPlotting = true;
		}

		std::string baseName = argv[2];
		uint32_t idx_pageSize = atoi(argv[3]);  // i.e. idx node size. = number of system pages in the index node. (sys-unit-page=4K (assume system page size is 4K))
		uint32_t cache_size = atoi(argv[4]); // max. number of idx-pages in the buffer
		uint32_t capacity = atoi(argv[5]);
		isPlot = atoi(argv[6])?true:false;  // sometimes plot after loading
		bool isIncremental = atoi(argv[7]) ? true : false;

		ISpatialIndex* tree{nullptr};
		IStorageManager *diskfile{nullptr};
		StorageManager::IBuffer *mybuffer;

		if (argc == 8 ) {

			std::cerr << " You are  INITIAL loading to a NEW tree !! " << std::endl;
			// Create a new storage manager with the provided base name and a 4K page size.
			diskfile =
					StorageManager::createNewDiskStorageManager(baseName,
							idx_pageSize * 4096);  // original 4096

			mybuffer =
					StorageManager::createNewRandomEvictionsBuffer(*diskfile,
							cache_size, false);  // original 10
			// applies a main memory random buffer on top of the persistent storage manager
			// (LRU buffer, etc can be created the same way)
			// Create a new, empty, RTree with dimensionality 2, minimum load 70%, using "file" as
			// the StorageManager and the RSTAR splitting policy.
			if(isIncremental)
				tree = RTree::createNewRTree(*mybuffer, 0.7, capacity, capacity,2, SpatialIndex::RTree::RV_INCREMENTAL, indexIdentifier);
			else
				tree = RTree::createNewRTree(*mybuffer, 0.7, capacity, capacity,2, SpatialIndex::RTree::RV_RSTAR, indexIdentifier);
		}
		else if(argc == 9){
			// ignore  "idx_pageSize"  and "capacity"
			// have not read indexIdentifier
			indexIdentifier=atoi(argv[8]);

			diskfile = StorageManager::loadDiskStorageManager(baseName); // if cahne something use returnDiskStorageManager
			mybuffer = StorageManager::createNewRandomEvictionsBuffer(
					*diskfile, capacity, false);

			Tools::PropertySet currentProp;
			Tools::Variant var;

			var.m_varType = Tools::VT_LONGLONG;
			var.m_val.llVal = indexIdentifier;
			currentProp.setProperty("IndexIdentifier", var);

			if (onlyPlotting) { // there is no input data, no loading..
				tree = RTree::returnRTree(*mybuffer, currentProp);
				std::cerr << " Plotting an EXISTING tree !! " << std::endl;
				MyQueryStrategy3 qs;
				tree->queryStrategy(qs);

				IStatistics *mystats;//=new MyStatistics();
				tree->getStatistics(&mystats);
				uint32_t numData = mystats->getNumberOfData();
//				uint32_t treeHeigth = mystats->getTreeHeight();
				uint32_t numNodes = mystats->getNumberOfNodes();
				std::cerr << "numData: "<< numData << std::endl;  // DO NOT CHANGE "numData", scripts uses it
				std::cerr << "numNodes: "<< numNodes << std::endl;
//				std::cerr << "treeHeigth: "<< treeHeigth << std::endl;
				delete tree;
				delete mybuffer;
				delete diskfile;
				return 0;
			}

			tree = RTree::returnRTree(*mybuffer, currentProp);

		}
		else
			std::cerr << "Wrong Usage !! " << std::endl;

		high_resolution_clock::time_point t1,t2,tstart,tend;
		duration<double> time_span;

		size_t count = 0;
		id_type id;
		uint32_t op;
		double x1, x2, y1, y2;
		double plow[2], phigh[2];

		#ifndef NDEBUG
		assert(count==0);
		#endif

		tstart = std::chrono::high_resolution_clock::now(); //utku

		t1 = std::chrono::high_resolution_clock::now(); //utku
		while (fin)
		{
			fin >> op >> id >> x1 >> y1 >> x2 >> y2;
			if (! fin.good()) continue; // skip newlines, etc.

			if (op == INSERT)
			{
				plow[0] = x1; plow[1] = y1;
				phigh[0] = x2; phigh[1] = y2;
				Region r = Region(plow, phigh, 2);

				std::ostringstream os;
				os << r;
				std::string data = os.str();
					// associate some data with this region. I will use a string that represents the
					// region itself, as an example.
					// NOTE: It is not necessary to associate any data here. A null pointer can be used. In that
					// case you should store the data externally. The index will provide the data IDs of
					// the answers to any query, which can be used to access the actual data from the external
					// storage (e.g. a hash table or a database table, etc.).
					// Storing the data in the index is convinient and in case a clustered storage manager is
					// provided (one that stores any node in consecutive pages) performance will improve substantially,
					// since disk accesses will be mostly sequential. On the other hand, the index will need to
					// manipulate the data, resulting in larger overhead. If you use a main memory storage manager,
					// storing the data externally is highly recommended (clustering has no effect).
					// A clustered storage manager is NOT provided yet.
					// Also you will have to take care of converting you data to and from binary format, since only
					// array of bytes can be inserted in the index (see RTree::Node::load and RTree::Node::store for
					// an example of how to do that).

				//tree->insertData((uint32_t)(data.size() + 1), reinterpret_cast<const uint8_t*>(data.c_str()), r, id);

				tree->insertData(0, 0, r, id);
//				std::cerr << " yoru are here: "<< id << " " <<std::endl;
					// example of passing zero size and a null pointer as the associated data.
				//utku: If you do not write some data, capacity=92 holds only 1 4K-disk page. Otherwise, if you write some data 1 node would need >1 disk page.
			}
			else if (op == DELETE)
			{
				plow[0] = x1; plow[1] = y1;
				phigh[0] = x2; phigh[1] = y2;
				Region r = Region(plow, phigh, 2);

				if (tree->deleteData(r, id) == false)
				{
					std::cerr << "******ERROR******" << std::endl;
					std::cerr << "Cannot delete id: " << id << " , count: " << count << std::endl;
					return -1;
				}
			}

			count++;
		}
		t2 = std::chrono::high_resolution_clock::now();
		time_span = duration_cast<duration<double>>(t2 - t1);
		std::cerr << "Time elapsed (sec) during loading  is : "
				<< time_span.count() << std::endl;
		t1 = std::chrono::high_resolution_clock::now();

		// here: this part is NOT time consuming..
		std::cerr << "Operations: " << count << std::endl;
		std::cerr << *tree;
		std::cerr << "Buffer hits: " << mybuffer->getHits() << std::endl;
		std::cerr << "Index ID: " << indexIdentifier << std::endl;

		// Do not forget to check validity..
		// here again: this part is NOT time consuming..
		bool ret = tree->isIndexValid();
		if (ret == false) std::cerr << "ERROR: Structure is invalid!" << std::endl;
		else std::cerr << "The stucture seems O.K." << std::endl;

		// New strategy for traversing Leaves Only. I want to plot them w/ gnuplot.
		// Bunu acarsan RTreload Disk IO numTotalNodes kadar artıyor. Cünkü burda bütün ağacı dolaşıyoruz.!!!
		// here this part IS time-cunsuming...
		if (isPlot) {
			MyQueryStrategy3 qs;
			tree->queryStrategy(qs);
		}

		delete tree;
		delete mybuffer;   // flush buffers to .dat file. But not sure to write to disk ?? OS actually writes..
		delete diskfile;  // flush .dat file to DISK. Request from OS.
			// delete the buffer first, then the storage manager
			// (otherwise the the buffer will fail trying to write the dirty entries).

		// utku
//		t2 = std::chrono::high_resolution_clock::now();
//		time_span = duration_cast<duration<double>>(t2 - t1);
//		std::cerr << "Time elapsed (sec) for until completion is : "
//				<< time_span.count() << std::endl;
//		t1 = std::chrono::high_resolution_clock::now();

		tend = std::chrono::high_resolution_clock::now(); //utku
		time_span = duration_cast<duration<double>>(tend - tstart);
		std::cerr << "TOTAL Time elapsed to completion is : "
						<< time_span.count() << std::endl;


	}
	catch (Tools::Exception& e)
	{
		std::cerr << "******ERROR******" << std::endl;
		std::string s = e.what();
		std::cerr << s << std::endl;
		return -1;
	}

	return 0;
//	return indexIdentifier;  no need for now..ANyway return of main is special case..it is supposed to be int and 0 or 1.
}
