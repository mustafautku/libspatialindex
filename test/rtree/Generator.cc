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

#define INSERT 1
#define DELETE 0
#define QUERY 2

class Region
{
public:
	double m_xmin, m_ymin, m_xmax, m_ymax;

	Region(double x1, double y1, double x2, double y2)
	{
		m_xmin = (x1 < x2) ? x1 : x2;
		m_ymin = (y1 < y2) ? y1 : y2;
		m_xmax = (x1 > x2) ? x1 : x2;
		m_ymax = (y1 > y2) ? y1 : y2;
	}
};

int main(int argc, char** argv)
{
	if (argc != 11)
	{
		std::cerr << "Usage: " << argv[0] << " ds loc_distr qs loc_distr d_dx d_dy d_distr. q_dx q_dy q_distr." << std::endl;
		return -1;
	}

	size_t numberOfObjects = atol(argv[1]);
	char data_loc_dist=*argv[2];

	size_t numberOfQueries = atol(argv[3]);
	char query_loc_dist=*argv[4];

	double d_dx = atof(argv[5]);
	double d_dy = atof(argv[6]);
	char d_dist = *argv[7];

	double q_dx = atof(argv[8]);
	double q_dy = atof(argv[9]);
	char q_dist=*argv[10];

	Tools::Random rnd;



	switch (data_loc_dist) {
	case 'u': // uniformly distributed locations.
		for (size_t i = 0; i < numberOfObjects; i++) {
			double x, y;
			x = rnd.nextUniformDouble();
			y = rnd.nextUniformDouble();

			double dx;
			double dy;
			switch (d_dist) {
			case 'f':  // fix size
				dx = d_dx;
				dy = d_dy;
				break;
			case 'u':   // uniform dist.
				dx = rnd.nextUniformDouble(0.001 * d_dx, d_dx);
				dy = rnd.nextUniformDouble(0.001 * d_dy, d_dy);
				break;
			}
			Region r = Region(x, y, x + dx, y + dy);
			std::cout << INSERT << " " << i << " " << r.m_xmin << " "
					<< r.m_ymin << " " << r.m_xmax << " " << r.m_ymax
					<< std::endl;
		}
		break;
	case 'g': //// gaussian distributed data locations.
		std::random_device rdx { };
		std::random_device rdy { };
		std::mt19937 genx { rdx() };
		std::mt19937 geny { rdy() };

		// values near the mean are the most likely
		// standard deviation affects the dispersion of generated values from the mean
		std::normal_distribution<> d1 { 0.25, 0.1 };
		for (size_t i = 0; i < (numberOfObjects * 4) / 10; i++) {
			double x, y;
			x = d1(genx);
			y = d1(geny);

			double dx;
			double dy;
			switch (d_dist) {
			case 'f':  // fix size
				dx = d_dx;
				dy = d_dy;
				break;
			case 'u':   // uniform dist.
				dx = rnd.nextUniformDouble(0.001 * d_dx, d_dx);
				dy = rnd.nextUniformDouble(0.001 * d_dy, d_dy);
				break;
			}
			Region r = Region(x, y, x + dx, y + dy);

			std::cout << INSERT << " " << i << " " << r.m_xmin << " "
					<< r.m_ymin << " " << r.m_xmax << " " << r.m_ymax
					<< std::endl;
		}
		std::normal_distribution<> d2 { 0.75, 0.1 };
		for (size_t i = (numberOfObjects * 4) / 10;
				i < (numberOfObjects * 8) / 10; i++) {
			double x, y;
			x = d2(genx);
			y = d2(geny);

			double dx;
			double dy;
			switch (d_dist) {
			case 'f':  // fix size
				dx = d_dx;
				dy = d_dy;
				break;
			case 'u':   // uniform dist.
				dx = rnd.nextUniformDouble(0.001 * d_dx, d_dx);
				dy = rnd.nextUniformDouble(0.001 * d_dy, d_dy);
				break;
			}

			Region r = Region(x, y, x + dx, y + dy);

			std::cout << INSERT << " " << i << " " << r.m_xmin << " "
					<< r.m_ymin << " " << r.m_xmax << " " << r.m_ymax
					<< std::endl;
		}
		for (size_t i = (numberOfObjects * 8) / 10; i < numberOfObjects; i++) {
					double x, y;
					x = rnd.nextUniformDouble();
					y = rnd.nextUniformDouble();

					double dx;
					double dy;
					switch (d_dist) {
					case 'f':  // fix size
						dx = d_dx;
						dy = d_dy;
						break;
					case 'u':   // uniform dist.
						dx = rnd.nextUniformDouble(0.001 * d_dx, d_dx);
						dy = rnd.nextUniformDouble(0.001 * d_dy, d_dy);
						break;
					}
					Region r = Region(x, y, x + dx, y + dy);

					std::cout << INSERT << " " << i << " " << r.m_xmin << " "
							<< r.m_ymin << " " << r.m_xmax << " " << r.m_ymax
							<< std::endl;
				}
		break;
	}

	switch (query_loc_dist) {
		case 'u': // uniformly distributed locations.
		for (size_t i = 0; i < numberOfQueries; i++) {
			double x = rnd.nextUniformDouble();
			double y = rnd.nextUniformDouble();
			double qx;
			double qy;
			switch (q_dist) {
			case 'f':  // fix size
				qx = q_dx;
				qy = q_dy;
				break;
			case 'u':   // uniform dist.
				qx = rnd.nextUniformDouble(0.001 * q_dx, q_dx);
				qy = rnd.nextUniformDouble(0.001 * q_dy, q_dy);
				break;
			}
			//			std::cout << QUERY << " 9999999 " << stx << " " << sty << " "
			//					<< (stx + 0.01) << " " << (sty + 0.01) << std::endl;
			std::cout << QUERY << " 9999999 " << x << " " << y << " "
					<< (x + qx) << " " << (y + qy) << std::endl;
		}
		break;
		case 'g'://// gaussian distributed Query locations.
			std::random_device rdx { };
			std::random_device rdy { };
			std::mt19937 genx { rdx() };
			std::mt19937 geny { rdy() };

			// values near the mean are the most likely
			// standard deviation affects the dispersion of generated values from the mean
			std::normal_distribution<> d1x { 0.25, 0.1 };
			std::normal_distribution<> d1y { 0.75, 0.1 };
			for (size_t i = 0; i < (numberOfQueries * 4) / 10; i++) {
				double x, y;
				x = d1x(genx);
				y = d1y(geny);

				double qx;
				double qy;
				switch (q_dist) {
				case 'f':  // fix size
					qx = q_dx;
					qy = q_dy;
					break;
				case 'u':   // uniform dist.
					qx = rnd.nextUniformDouble(0.001 * q_dx, q_dx);
					qy = rnd.nextUniformDouble(0.001 * q_dy, q_dy);
					break;
				}

				std::cout << QUERY << " 9999999 " << x << " " << y << " "
												<< (x + qx) << " " << (y + qy) << std::endl;
			}
			std::normal_distribution<> d2x { 0.75, 0.1 };
			std::normal_distribution<> d2y { 0.25, 0.1 };
			for (size_t i = (numberOfQueries * 4) / 10;
					i < (numberOfQueries * 8) / 10; i++) {
				double x, y;
				x = d2x(genx);
				y = d2y(geny);

				double qx;
				double qy;
				switch (q_dist) {
				case 'f':  // fix size
					qx = q_dx;
					qy = q_dy;
					break;
				case 'u':   // uniform dist.
					qx = rnd.nextUniformDouble(0.001 * q_dx, q_dx);
					qy = rnd.nextUniformDouble(0.001 * q_dy, q_dy);
					break;
				}

				std::cout << QUERY << " 9999999 " << x << " " << y << " "
												<< (x + qx) << " " << (y + qy) << std::endl;
			}
			for (size_t i = (numberOfQueries * 8) / 10; i < numberOfQueries; i++) {
						double x, y;
						x = rnd.nextUniformDouble();
						y = rnd.nextUniformDouble();

						double qx;
						double qy;
						switch (q_dist) {
						case 'f':  // fix size
							qx = q_dx;
							qy = q_dy;
							break;
						case 'u':   // uniform dist.
							qx = rnd.nextUniformDouble(0.001 * q_dx, q_dx);
							qy = rnd.nextUniformDouble(0.001 * q_dy, q_dy);
							break;
						}
						std::cout << QUERY << " 9999999 " << x << " " << y << " "
																		<< (x + qx) << " " << (y + qy) << std::endl;
					}
			break;
	}


//	size_t A = static_cast<size_t>(std::floor(static_cast<double>(numberOfObjects) * 0.05));
//
//	for (size_t T = 1; T <= simulationLength; T++)
//	{
//		std::cerr << (simulationLength + 1 - T) << std::endl;
//		std::set<size_t> examined;
//
//		for (size_t a = 0; a < A; a++)
//		{
//			// find an id that is not yet examined.
//			size_t id = static_cast<size_t>(rnd.nextUniformLong(0, (int32_t)numberOfObjects));
//			std::set<size_t>::iterator itSet = examined.find(id);
//
//			while (itSet != examined.end())
//			{
//				id = static_cast<size_t>(rnd.nextUniformLong(0, (int32_t)numberOfObjects));
//				itSet = examined.find(id);
//			}
//			examined.insert(id);
//
//			std::map<size_t, Region>::iterator itMap = data.find(id);
//			assert(itMap != data.end());
//
//			std::cout << DELETE << " " << id << " " << (*itMap).second.m_xmin << " " << (*itMap).second.m_ymin << " "
//				<< (*itMap).second.m_xmax << " " << (*itMap).second.m_ymax << std::endl;
//
//			double x = rnd.nextUniformDouble();
//			double dx = rnd.nextUniformDouble(0.0001, 0.1);
//			(*itMap).second.m_xmin = x;
//			(*itMap).second.m_xmax = x + dx;
//			double y = rnd.nextUniformDouble();
//			double dy = rnd.nextUniformDouble(0.0001, 0.1);
//			(*itMap).second.m_ymin = y;
//			(*itMap).second.m_ymax = y + dy;
//
//			std::cout << INSERT << " " << id << " " << (*itMap).second.m_xmin << " " << (*itMap).second.m_ymin << " "
//				<< (*itMap).second.m_xmax << " " << (*itMap).second.m_ymax << std::endl;
//		}
//
//		double stx = rnd.nextUniformDouble();
//		double sty = rnd.nextUniformDouble();
//		std::cout << QUERY << " 9999999 " << stx << " " << sty << " " << (stx + 0.01) << " " << (sty + 0.01) << std::endl;
//	}

	return 0;
}
