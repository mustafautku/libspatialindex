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
// include library header file.
#include <spatialindex/SpatialIndex.h>
#include <chrono>

using namespace SpatialIndex;

#define INSERT 1
#define DELETE 0
#define QUERY 2

class MyDataStream: public IDataStream {
public:
	MyDataStream(std::string inputFile) :
			m_pNext(nullptr) {
		m_fin.open(inputFile.c_str());

		if (!m_fin)
			throw Tools::IllegalArgumentException("Input file not found.");

		readNextEntry();
	}

	~MyDataStream() override
	{
		if (m_pNext != nullptr)
			delete m_pNext;
	}

	IData* getNext() override
	{
		if (m_pNext == nullptr)
			return nullptr;

		RTree::Data* ret = m_pNext;
		m_pNext = nullptr;
		readNextEntry();
		return ret;
	}

	bool hasNext() override
	{
		return (m_pNext != nullptr);
	}

	uint32_t size() override
	{
		throw Tools::NotSupportedException("Operation not supported.");
	}

	void rewind() override
	{
		if (m_pNext != nullptr) {
			delete m_pNext;
			m_pNext = nullptr;
		}

		m_fin.seekg(0, std::ios::beg);
		readNextEntry();
	}

	void readNextEntry() {
		id_type id;
		uint32_t op;
		double low[2], high[2];

		m_fin >> op >> id >> low[0] >> low[1] >> high[0] >> high[1];

		if (m_fin.good()) {
			if (op != INSERT)
				throw Tools::IllegalArgumentException(
						"The data input should contain insertions only.");

			Region r(low, high, 2);
			m_pNext = new RTree::Data(0, 0, r, id);
//			m_pNext = new RTree::Data(sizeof(double),
//					reinterpret_cast<uint8_t*>(low), r, id);
			// Associate a bogus data array with every entry for testing purposes.
			// Once the data array is given to RTRee:Data a local copy will be created.
			// Hence, the input data array can be deleted after this operation if not
			// needed anymore.
		}
	}

	std::ifstream m_fin;
	RTree::Data* m_pNext;
};

// Strategy for traversing LEAVES ONLY. write Leaves' MBR to std output, cout. Bash script redirect cout to a "pltDynLevel0" file
class MyQueryStrategy3: public SpatialIndex::IQueryStrategy {

private:
	std::queue<id_type> ids;

public:
	void getNextEntry(const IEntry& entry, id_type& nextEntry, bool& hasNext) {
		IShape* ps;
		entry.getShape(&ps);
		Region* pr = dynamic_cast<Region*>(ps);

		const INode* n = dynamic_cast<const INode*>(&entry);
		// traverse only index nodes at levels 1 and higher.
		if (n != 0 && n->getLevel() > 0) {
			for (uint32_t cChild = 0; cChild < n->getChildrenCount();
					cChild++) {
				ids.push(n->getChildIdentifier(cChild));
			}
		}
		else{
			std::cout << pr->m_pLow[0] << " " << pr->m_pLow[1] << std::endl;
			std::cout << pr->m_pHigh[0] << " " << pr->m_pLow[1] << std::endl;
			std::cout << pr->m_pHigh[0] << " " << pr->m_pHigh[1] << std::endl;
			std::cout << pr->m_pLow[0] << " " << pr->m_pHigh[1] << std::endl;
			std::cout << pr->m_pLow[0] << " " << pr->m_pLow[1] << std::endl << std::endl;
		}
		if (!ids.empty()) {
					nextEntry = ids.front();
					ids.pop();
					hasNext = true;
		}
		else hasNext = false;
		delete ps;
	}
};

int main(int argc, char** argv) {
	try {
		if (! (argc == 5  || argc == 8) ) { // either 5 or 8 argument. Others are incorrect.
			std::cerr << "Usage       : " << argv[0]
					<< " input_file tree_file capacity utilization r PS BP "  // pS: PageSize(#ofRecords/Block)   bP: Total # of Buffers
					<< std::endl;
			return -1;
		}

		MyDataStream stream(argv[1]);
		std::string baseName = argv[2];
		uint32_t capacity = atoi(argv[3]);
		double fillFactor = atof(argv[4]);  // utilization


		IStorageManager* diskfile = StorageManager::createNewDiskStorageManager(
				baseName, 4096);
		// Create a new storage manager with the provided base name and a 4K page size.

		StorageManager::IBuffer* file =
				StorageManager::createNewRandomEvictionsBuffer(*diskfile, 10,
						false);
		// applies a main memory random buffer on top of the persistent storage manager
		// (LRU buffer, etc can be created the same way).



		//utku:
		auto t1 = std::chrono::high_resolution_clock::now();

		// Create and bulk load a new RTree with dimensionality 2, using "file" as
		// the StorageManager and the RSTAR splitting policy.
		id_type indexIdentifier;

		// Below use default buffer area (pS=10000 and bP=100)
//		ISpatialIndex* tree = RTree::createAndBulkLoadNewRTree(RTree::BLM_STR,
//				stream, *file, utilization, atoi(argv[3]), atoi(argv[3]), 2,
//				SpatialIndex::RTree::RV_RSTAR, indexIdentifier);

		// Below, we may set up our buffer pool org.
		Tools::Variant var;
		Tools::PropertySet ps;

		var.m_varType = Tools::VT_DOUBLE;
		var.m_val.dblVal = fillFactor;
		ps.setProperty("FillFactor", var);

		var.m_varType = Tools::VT_ULONG;
		var.m_val.ulVal = capacity;
		ps.setProperty("IndexCapacity", var);

		var.m_varType = Tools::VT_ULONG;
		var.m_val.ulVal = capacity;
		ps.setProperty("LeafCapacity", var);

		var.m_varType = Tools::VT_ULONG;
		var.m_val.ulVal = 2;
		ps.setProperty("Dimension", var);

		var.m_varType = Tools::VT_LONG;
		var.m_val.lVal = SpatialIndex::RTree::RV_RSTAR ;
		ps.setProperty("TreeVariant", var);


		if (argc == 8) {
			var.m_varType = Tools::VT_DOUBLE;
			var.m_val.dblVal = atof(argv[5]);   // r=qx/qy;
			ps.setProperty("QueryAspectRatio", var);

			var.m_varType = Tools::VT_ULONG;
			var.m_val.ulVal = atol(argv[6]);  //pS
			ps.setProperty("ExternalSortBufferPageSize", var);

			var.m_varType = Tools::VT_ULONG;
			var.m_val.ulVal = atol(argv[7]);  // bP
			ps.setProperty("ExternalSortBufferTotalPages", var);
		} else { // use default values
			var.m_varType = Tools::VT_DOUBLE;
			var.m_val.dblVal = 1.0;   // r=qx/qy;
			ps.setProperty("QueryAspectRatio", var);

			var.m_varType = Tools::VT_ULONG;
			var.m_val.ulVal = 10000;  //pS
			ps.setProperty("ExternalSortBufferPageSize", var);

			var.m_varType = Tools::VT_ULONG;
			var.m_val.ulVal = 100;  // bP
			ps.setProperty("ExternalSortBufferTotalPages", var);
		}



		ISpatialIndex* tree = RTree::createAndBulkLoadNewRTree(RTree::BLM_STR, stream, *file, ps, indexIdentifier);




		std::cerr << *tree;
		std::cerr << "Buffer hits: " << file->getHits() << std::endl;
		std::cerr << "Index ID: " << indexIdentifier << std::endl;

		auto t2 = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
				t2 - t1).count();
		std::cerr << "Time elapsed (msec) for STR loading is : " << duration
				<< std::endl;

//		bool ret = tree->isIndexValid();
//		if (ret == false)
//			std::cerr << "ERROR: Structure is invalid!" << std::endl;
//		else
//			std::cerr << "The stucture seems O.K." << std::endl;

		// New strategy for traversing Leaves Only. I want to plot them w/ gnuplot.
		// Bunu acarsan RTreBulkload Disk IO 2 katına çıkıyor. Cünkü burda bütün ağacı dolaşıyoruz.!!!
		MyQueryStrategy3 qs;
		tree->queryStrategy(qs);

		delete tree;
		delete file;
		delete diskfile;
		// delete the buffer first, then the storage manager
		// (otherwise the the buffer will fail trying to write the dirty entries).
	} catch (Tools::Exception& e) {
		std::cerr << "******ERROR******" << std::endl;
		std::string s = e.what();
		std::cerr << s << std::endl;
		return -1;
	}

	return 0;
}
