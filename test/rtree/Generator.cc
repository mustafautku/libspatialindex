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

#include <spatialindex/tools/Tools.h>
#include <cmath>
#include <limits>
#include <string>

#include <set>

#include <iostream>
#include <iomanip>
#include <string>
#include <map>
#include <random>
#include <cmath>

#include <morton.h>

#define INSERT 1
#define DELETE 0
#define QUERY 2


void gen_uniform_mc(size_t ,size_t , double  , char* );

void gen_uniform(size_t, size_t, double , double ,char ,char*, double*, double*, size_t,size_t );
void gen_uniform(size_t, size_t, double , double ,double, char*, double*, double*, size_t, size_t );
void gen_normal(size_t, size_t, double , double, char, char*, double*, double*, size_t,size_t , int);
void gen_normal(size_t, size_t, double , double, double,char*, double*, double*, size_t,size_t, int);



bool is_digits(const std::string &str)
{
//    return std::all_of(str.begin(), str.end(), ::isdigit); // C++11
	return str.find_first_not_of(".0123456789") == std::string::npos;
}


int main(int argc, char** argv)
{
	size_t start_id=0;
	size_t grid_depth=0;
	size_t numOfcells=1;
	double* centerX = nullptr;
	double* centerY = nullptr;

	if (argc < 7 )
	{

		std::cerr << "Usage: " << argv[0] <<
		//                                 								   set-size                               u|g[skewedness]                      max-dx-sized-region        "				                             dx_dy-distr | AQAR value
		" 1-)data?query[data|query]   2-)ds(qs)[numeric]   3-)loc_distr[u|g[skewedness]]    4-)d(q)_dx[double]     5-)d(q)_dy[double]     6-)d(q)_dxdy_distr.[f|u|double] "
		"[7-) starting id ] [8-) grid_depth] [9-) grid_mc1] [10-) grid_mc2] ..." << std::endl;

		// [skewedness] : [1, >1]:  10 is very skew. Default is 5.

		// d(q)_dxdy_distr.  : for data we mention "fixed-sized" vs. "uniformly-dist-sized regions"
		//                     for query we mention AQAR

		//  1-)query    2-)100    3-)u   4-)10000   5-)-1   6-)-1    7-)0

	}

//	for previous tests..

	if (argc == 7 )
	{
		centerX = new double[numOfcells];
		centerY = new double[numOfcells];
		centerX[0] = 0.5;
		centerY[0] = 0.5;
	}
	if (argc == 8) {
		start_id = atol(argv[7]);

		double dy = atof(argv[5]);
		if(dy == -1){  // then we are generating 32 bit unsigned random values..
			char *dataOrQuery=argv[1];
			size_t numberOfObjects = atol(argv[2]);
			char* loc_dist=argv[3];

			double dx = atof(argv[4]);   //  this is selectivity. F.e.  10  --> %10 ==> 0.1 of max
			switch (*loc_dist) {
				case 'u':
					gen_uniform_mc(start_id,start_id+numberOfObjects,dx, dataOrQuery);
					break;
				case 'g':
					break;
			}
			return 0;
		}

		centerX = new double[numOfcells];
		centerY = new double[numOfcells];
		centerX[0] = 0.5;
		centerY[0] = 0.5;

	}
	if (argc >8 ) {

		start_id = atol(argv[7]);
		grid_depth = atol(argv[8]);
		numOfcells = argc - 9 ;
		uint32_t grid_mc[numOfcells];
		centerX = new double[numOfcells];
		centerY = new double[numOfcells];
		for (auto i=0;i<numOfcells;i++){
			grid_mc[i] = atoi(argv[9 + i]);

			uint_fast16_t xs1, ys1;
			libmorton::morton2D_32_decode(grid_mc[i], xs1, ys1);
			//	std::cout << "decoding:" << xs1 << '\t' << ys1 << std::endl;
			//	std::cout << "decoding:" << (double) xs1 / pow(2, grid_depth) << '\t'
			//		<< (double) ys1 / pow(2, grid_depth) << std::endl;
			// "center" of "data generation area" in unit area
			centerX[i] = (double) (xs1 / pow(2, grid_depth)
					+ 1 / (2 * pow(2, grid_depth)));
			centerY[i] = (double) (ys1 / pow(2, grid_depth)
					+ 1 / (2 * pow(2, grid_depth)));
		}
	}

	char *dataOrQuery=argv[1];

	size_t numberOfObjects = atol(argv[2]);
	char* loc_dist=argv[3];

	// extent sizes
	double dx = atof(argv[4]);
	double dy = atof(argv[5]);
//	char dxdy_dist = *argv[6];


//	std::string dq_string=std::string(dataOrQuery);
	char *pchr=argv[6];
	std::string aqarOrExtent=std::string(pchr);



	switch (*loc_dist) {
	case 'u': // uniformly distributed locations.
		if (is_digits(aqarOrExtent)) {
			gen_uniform(start_id,start_id+numberOfObjects, dx, dy, atof(argv[6]),dataOrQuery,centerX,centerY,grid_depth, numOfcells); // send aqar for query
		} else {
			gen_uniform(start_id,start_id+numberOfObjects, dx, dy, *argv[6],dataOrQuery,centerX,centerY,grid_depth, numOfcells); // send extent for data or query
		}
		break;
	case 'g': //// gaussian distributed data locations. you may generate many different forms of clustered data here..
		std::string locdist=std::string(loc_dist);
		size_t len = locdist.length()-1;
		int skw=5;
		if (len > 0) {
			std::string skws = locdist.substr(1, len);
			skw = atoi(skws.c_str());
		}
		if (is_digits(aqarOrExtent)) {
			gen_normal(start_id,start_id+numberOfObjects, dx, dy,atof(argv[6]),dataOrQuery,centerX,centerY,grid_depth, numOfcells, skw);
		} else {
			gen_normal(start_id,start_id+numberOfObjects, dx, dy,*argv[6],dataOrQuery,centerX,centerY,grid_depth, numOfcells,skw);
		}
		break;
	}
	delete centerX;
	delete centerY;
	return 0;
}

// generate scalar values or scalar ranges in uint32_t domain.
void gen_uniform_mc(size_t start,size_t end, double dx, char* dataOrQuery){
	Tools::Random rnd;
		size_t i = start;
//		double sel=dx;
		uint32_t interval= (std::numeric_limits<uint32_t>::max() / 100)*dx;  // dx is selectivity. f.e. 10   means %10 intervals..  0.1 * max
		for (; i < end;)
		{
			uint32_t x = rnd.nextUniformUnsignedLong();
			if (std::string(dataOrQuery) == "data"){
				std::cout << INSERT << " " << i << " " << x  <<std::endl;
				i++;
				continue;
			}
	//		uint32_t interval= rnd.nextUniformUnsignedLong(100000000,1000000000);

			if( x >= std::numeric_limits<uint32_t>::max() - interval)
				continue;
	//		uint32_t y = ( x < std::numeric_limits<uint32_t>::max() - interval) ? (x + interval) : std::numeric_limits<uint32_t>::max();
	//		uint32_t y = rnd.nextUniformUnsignedLong(x,std::numeric_limits<uint32_t>::max());
			uint32_t y = (x + interval);
			// Below is MBR-style and needed for libspatialindex input data format.
			std::cout << QUERY << " " << i << " " << x << " "<< y <<" "<< dx <<std::endl;
			i++;
		}


}


void gen_uniform(size_t start,size_t end, double dx, double dy, double AQAR,char* dataOrQuery, double* centerX, double* centerY, size_t depth, size_t numOfCells) {
	Tools::Random rnd;

	int subsetsize = (end-start) / numOfCells;
	std::cerr  <<" generating uniform data with " << AQAR <<" extent dist" <<std::endl;
	int s = start;
	for(auto b=0;b<numOfCells;b++){
		double xl = centerX[b] - 1 / (2 * pow(2, depth));
		double xh = centerX[b] + 1 / (2 * pow(2, depth));
		double yl = centerY[b] - 1 / (2 * pow(2, depth));
		double yh = centerY[b] + 1 / (2 * pow(2, depth));

		for (size_t i = s; i < s+subsetsize; i++) {
			double x, y;
			double dx2;
			double dy2;

			do {
				x = xl + (xh - xl) * rnd.nextUniformDouble();
				y = yl + (yh - yl) * rnd.nextUniformDouble();
				double p = sqrt(1 / AQAR);
				dx2 = dx / p;
				dy2 = dy * p;
			} while (x < 0 || y < 0 ||x >= 1 || y >= 1 ||  (x + dx2) >= 1 || (y + dy2) >= 1);

			if (x == 1 ||y==1){
				std::cout << "error at " << i << std::endl;
				exit(1);
			}
			if (std::string(dataOrQuery) == "data")
				std::cout << INSERT << " " << i << " " << x << " " << y << " "
						<< (x + dx2) << " " << (y + dy2) << std::endl;
			else
				std::cout << QUERY << " 9999999 " << x << " " << y << " "
						<< (x + dx2) << " " << (y + dy2) << std::endl;
		}
		s = s + subsetsize;
	}
}

void gen_uniform(size_t start,size_t end, double dx, double dy,char dxdy_dist,char* dataOrQuery, double* centerX, double* centerY, size_t depth, size_t numOfCells) {
	Tools::Random rnd;
	int subsetsize = (end-start) / numOfCells;
	std::cerr  <<" generating uniform data with " << dxdy_dist <<" extent dist" <<std::endl;
	int s = start;

	for (auto b = 0; b < numOfCells; b++) {
		double xl = centerX[b] - 1 / (2 * pow(2, depth));
		double xh = centerX[b] + 1 / (2 * pow(2, depth));
		double yl = centerY[b] - 1 / (2 * pow(2, depth));
		double yh = centerY[b] + 1 / (2 * pow(2, depth));

		for (size_t i = s; i < s+subsetsize; i++) {

			double x, y;
			double dx2;
			double dy2;
			do {
				x = xl + (xh - xl) * rnd.nextUniformDouble();
				y = yl + (yh - yl) * rnd.nextUniformDouble();
				switch (dxdy_dist) {  //extent
				case 'f':  // fix size
					dx2 = dx;
					dy2 = dy;
					break;
				case 'u':   // uniform dist. sized regions
					dx2 = rnd.nextUniformDouble(0.001 * dx, dx);
					dy2 = rnd.nextUniformDouble(0.001 * dy, dy);
					break;
				}
			} while (x < 0 || y < 0 ||x >= 1 || y >= 1 || (x + dx2) >= 1 || (y + dy2) >= 1);

			if (x == 1 ||y==1){
				std::cout << "error at " << i << std::endl;
				exit(1);
			}
			if (std::string(dataOrQuery) == "data")
				std::cout << INSERT << " " << i << " " << x << " " << y << " "
						<< (x + dx2) << " " << (y + dy2) << std::endl;
			else
				std::cout << QUERY << " 9999999 " << x << " " << y << " "
						<< (x + dx2) << " " << (y + dy2) << std::endl;
		}
	  s = s+subsetsize;
	}
}


void gen_normal(size_t start,size_t end,double dx, double dy, double AQAR,char* dataOrQuery, double* centerX, double* centerY, size_t depth, size_t numOfCells, int skw) {
	Tools::Random rnd;
	std::random_device rdx { };
	std::random_device rdy { };
	std::mt19937 genx { rdx() };
	std::mt19937 geny { rdy() };
	std::cerr  <<" generating normal data with " << AQAR <<" extent dist" <<std::endl;
	double stdev = 1 / (skw * pow(2, depth));

	int subsetsize = (end - start) / numOfCells;
	int s = start;
	for (auto b = 0; b < numOfCells; b++) {
		// values near the mean are the most likely
		// standard deviation affects the dispersion of generated values from the mean
		std::normal_distribution<> d1x { centerX[b], stdev };
		std::normal_distribution<> d1y { centerY[b], stdev };

		for (size_t i = s; i < s+subsetsize; i++) {
			double x, y;
			double dx2;
			double dy2;

			do {
				x = d1x(genx);
				y = d1y(geny);
				double p = sqrt(1 / AQAR);
				dx2 = dx / p;
				dy2 = dy * p;
			} while (x < 0 || y < 0 ||x >= 1 || y >= 1 || (x + dx2) >= 1 || (y + dy2) >= 1);

			if (x == 1 ||y==1){
				std::cout << "error at " << i << std::endl;
				exit(1);
			}

			if (std::string(dataOrQuery) == "data")
				std::cout << INSERT << " " << i << " " << x << " " << y << " "
						<< (x + dx2) << " " << (y + dy2) << std::endl;
			else
				std::cout << QUERY << " 9999999 " << x << " " << y << " "
						<< (x + dx2) << " " << (y + dy2) << std::endl;
		}

		s = s+subsetsize;
	}

}


void gen_normal(size_t start,size_t end,double dx, double dy, char dxdy_dist,char* dataOrQuery, double* centerX, double* centerY, size_t depth, size_t numOfCells, int skw) {
	Tools::Random rnd;
	std::random_device rdx { };
	std::random_device rdy { };
	std::mt19937 genx { rdx() };
	std::mt19937 geny { rdy() };
	
	std::cerr  <<" generating normal data with " << dxdy_dist <<" extent dist" <<std::endl;
	double stdev = 1 / (skw * pow(2, depth));

	int subsetsize = (end - start) / numOfCells;
	int s = start;
	for (auto b = 0; b < numOfCells; b++) {

		// values near the mean are the most likely
		// standard deviation affects the dispersion of generated values from the mean
		std::normal_distribution<> d1x { centerX[b], stdev };  // std dev=0.1
		std::normal_distribution<> d1y { centerY[b], stdev };

		for (size_t i = s; i < s+subsetsize; i++) {
			double x, y;
			double dx2;
			double dy2;

			do {
				x = d1x(genx);
				y = d1y(geny);
				switch (dxdy_dist) {  //query extent
				case 'f':  // fix size
					dx2 = dx;
					dy2 = dy;
					break;
				case 'u':   // uniform dist. sized regions
					dx2 = rnd.nextUniformDouble(0.001 * dx, dx);
					dy2 = rnd.nextUniformDouble(0.001 * dy, dy);
					break;
				}
			} while (x < 0 || y < 0 ||x >= 1 || y >= 1 || (x + dx2) >= 1 || (y + dy2) >=1);

			if (x == 1 ||y==1){
				std::cerr << "error at " << i << std::endl;
				exit(1);
			}
			if (std::string(dataOrQuery) == "data")
				std::cout << INSERT << " " << i << " " << x << " " << y << " "
						<< (x + dx2) << " " << (y + dy2) << std::endl;
			else
				std::cout << QUERY << " 9999999 " << x << " " << y << " "
						<< (x + dx2) << " " << (y + dy2) << std::endl;

		}
		 s = s+subsetsize;

	}
}





























