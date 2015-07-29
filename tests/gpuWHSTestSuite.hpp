#ifndef _gpu_WINDOW_HISTOGRAM_SUITE_H_
#define _gpu_WINDOW_HISTOGRAM_SUITE_H_

#include <cxxtest/TestSuite.h>
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <iostream>
#include <chrono>


#ifdef HAVE_CGI
	#include "../src/base/cvTileConversion.hpp"
	#include <ext/numeric>
#endif

#include "../src/base/cvTile.hpp"
#include "../src/base/Tiler.hpp"
#include <boost/filesystem.hpp>


#include "../src/gpu/drivers/GpuWHS.hpp"
#include "../src/gpu/drivers/GpuWindowFilterAlgorithm.hpp"


#define TIMING_ON 0

struct Stats {
	float mean;
	float entropy;
	float variance;
	float skewness;
	float kurtosis;
};


class gpuWHSTestSuite : public CxxTest::TestSuite
{
	public:

		void setUp()
		{;}

		void tearDown()
		{;}


		void calcStatisitics(Stats *stats, double* data,unsigned int histSize, unsigned int dataSize) {
			//std::cout << "TEST VALUES ARE: " << std::endl;
			/*for (int i = 0; i < dataSize; ++i) {
				std::cout << data[i] << " ";
			}
			std::cout << std::endl;*/
			
			double total = 0;
			short min = data[0];
			short max = data[0];

			/*Find total sum of data*/
			for (size_t i = 0; i < dataSize; ++i) {
				total += data[i];

				if (data[i]  > max){
					max = data[i];
				}

				if (data[i] < min) {
					min = data[i];
				}
			}
			

			short num_bins = 128;
			//std::cout << "num bins = " << num_bins << std::endl;
			//std::cout << "min = " << min << " max = " << max << std::endl;
			short bin_width = (max - min) / num_bins;
			if (bin_width <= 0) {
				bin_width = 1;
			}

			//std::cout << "bin width = " << bin_width << std::endl;
			short hist[num_bins];
			memset(hist,0,sizeof(short) * num_bins);

			float pdf[num_bins];
			
			short bin_idx = 0;
			/* Histogram Calculation */
			for (unsigned int j = 0; j < dataSize; ++j) {
				bin_idx = (short) ((data[j] - min) / bin_width);
				if (bin_idx >= 0 && bin_idx < num_bins) {
					hist[bin_idx]++;
				}
				else
					hist[127]++;
			}
			/*
			 * PDF calculation */
			for (short i = 0; i < num_bins;++i) {
				pdf[i] = (float) hist[i] / dataSize;
			}


			
			/*calculate mean*/
			double mean  = (double) total/dataSize;

			/*Varaince calculation*/
			double var = 0;
			for (unsigned int i = 0; i < dataSize; ++i) {
				const double res = data[i] - mean;
				var = var + (res * res); 
			}
			var = (double) var/(dataSize);
			/*calculate std */
			
			double std = sqrtf(var);
			double skewness = 0;

			double kurtosis = 0;

			/* Calculate Entropy */
			double entropy = 0;
			for (short i = 0; i < num_bins; ++i) {
				if (pdf[i] != 0) {
					entropy += (pdf[i] * log2(pdf[i]));
				}
			}
			if (std == 0 || var == 0) {
				stats->mean = (float) mean;
				stats->entropy = (float) (entropy * -1);
				stats->variance = (float) var;
				stats->skewness = (float) skewness;
				stats->kurtosis = (float) kurtosis;
				return;
			}
			//std::cout << "std = " << std << std ::endl;

			/*Calculate Skewness*/
			for (unsigned int i = 0; i < dataSize; ++i) {
				const double tmp = (data[i] - mean);
				skewness = skewness + (tmp * tmp * tmp);
			}

			skewness = (double) skewness/(dataSize * var * std);


			/*Calculate kurtosis*/

			for (unsigned int i = 0; i < dataSize; ++i) {
				const double tmp = (data[i] - mean);
				kurtosis = kurtosis + ( tmp * tmp * tmp * tmp );
			}

			kurtosis = (double) kurtosis/(dataSize * var * var);



			//kurtosis -= 3;


			stats->mean = (float) mean;
			stats->entropy = (float) (entropy * -1);
			stats->variance = (float) var;
			stats->skewness = (float) skewness;
			stats->kurtosis = (float) kurtosis;
		
		}

		void testWindowHistogramSingleBandImage () {
			std::cout << std::endl << "VERIFICATION TEST" << std::endl;
			int cuda_device_id = 0;
			//unsigned int window_size = 1;



			cvt::Tiler read_tiler;

			cv::Size2i tSize(256,256);
			read_tiler.setCvTileSize(tSize);

			read_tiler.open("test1-1.tif");

			cvt::cvTile<short> inputTile;
			//cvt::cvTile<float> *outputTile;
			inputTile = read_tiler.getCvTile<short>(4);

			/* Loop through all the tiles in the image */
			for (int window = 1; window <= 11; window++) {	

					cvt::gpu::GpuWHS<short,1,float,5> whs(cuda_device_id,
					256,256,window);
					whs.initializeDevice(cvt::gpu::SQUARE);

					cvt::cvTile<float> *outputTile;
					whs(inputTile,(const cvt::cvTile<float> **)&outputTile);
					if (!outputTile) {
						std::cout << "FAILURE TO GET DATA FROM DEVICE" << std::endl;
						return;
					}
					TS_ASSERT_EQUALS(outputTile->getBandCount(),5);	
					/*Calculate Window Histogram Statistics for each pixel*/
					cv::Size2i dims = inputTile.getSize();
					const int area = dims.width * dims.height;
					std::vector<Stats> stats;
					stats.resize(area);
			
					for (int i = 0; i < area; ++i) {
						std::vector<double> data;
						//const int pixelX = (i / 256);
						//const int pixelY = (i % 256);

						//std::cout << "Center Pixel: ( " << pixelX << "," << pixelY << " )" << std::endl;
						for (int x = 0 - window; x <= window; ++x) {
							for (int y = 0 - window; y <= window; ++y) {
								const int X = (i / 256) + x;
								const int Y = (i % 256) + y;
								if (X >= 0 && X < dims.width && Y >= 0 && Y < dims.height) {
									//std::cout << "( " << X << "," << Y << " ) ";
									data.push_back(inputTile[0].at<short>(X,Y));
								}
								else {
									data.push_back(0);
								}
							}
						}
						//std::cout << "window area = " << data.size() << std::endl;

						calcStatisitics(&stats[i], data.data(),32, data.size());	

					}
					//std::cout << "size = " << data.size() << std::endl;

					
					for (size_t s = 0; s < stats.size(); ++s) {
						const size_t row = s / 256;
						const size_t col = s % 256;
					

						TS_ASSERT_DELTA(stats[s].entropy,(*outputTile)[0].at<float>(row,col),1e-5);
						TS_ASSERT_DELTA(stats[s].mean,(*outputTile)[1].at<float>(row,col),1e-5);
						TS_ASSERT_DELTA(stats[s].variance,(*outputTile)[2].at<float>(row,col),1e-5);
						TS_ASSERT_DELTA(stats[s].skewness,(*outputTile)[3].at<float>(row,col),1e-5);
						TS_ASSERT_DELTA(stats[s].kurtosis,(*outputTile)[4].at<float>(row,col),1e-5);
					}
					delete outputTile;
					

			}
					read_tiler.close();

		
		}

		void testTimingVariousBlockSizes () {
			std::cout << std::endl << "TESTING BLOCKSIZES" << std::endl;
			int cuda_device_id = 0;
			unsigned int unbuffered_data_width = 0;
			unsigned int unbuffered_data_height = 0;
			unsigned int window_size = 7;
			cvt::cvTile<short> inputTile;
			

			cvt::Tiler read_tiler;
			cvt::Tiler write_tiler;

			cv::Size2i tSize(256,256);
			read_tiler.setCvTileSize(tSize);
			write_tiler.setCvTileSize(tSize);
			unsigned long numPixels = tSize.area();

			read_tiler.open("test1-1.tif");
			std::string outFile = "windowHistogram.tif";
			/*cv::Size2i rasterSize = read_tiler.getRasterSize();
			read_tiler.setCvTileSize(rasterSize);*/

			
			cvt::gpu::GpuWHS<short,1,float,5> whs(cuda_device_id,
					256,256,window_size);

			whs.initializeDevice(cvt::gpu::SQUARE);

		
			
			if(boost::filesystem::exists(outFile)){
				boost::filesystem::remove(outFile);
			}
			/* Ensure tiler opens outfile correctly */
			TS_ASSERT_EQUALS(cvt::NoError, 
			write_tiler.create(outFile, "GTiff", 
					tSize, 
					5, cvt::Depth32F))
	
			

			std::chrono::high_resolution_clock::time_point startI = std::chrono::high_resolution_clock::now();


			/* Loop through all the tiles in the image */
			//for(int i = 0; i < read_tiler.getCvTileCount(); ++i){
					/* Retrieve a tile, with 126 pixel edge buffer */
				
			const cvt::cvTile<short> tile = read_tiler.getCvTile<short>( 4 ); //no buffer
			for (int i = 0; i < 35; ++i) {
				std::chrono::high_resolution_clock::time_point startT = std::chrono::high_resolution_clock::now();

				

					/* Do the Window Histogram Stats */
				cvt::cvTile<float> *outputTile;
				whs(tile,(const cvt::cvTile<float> **)&outputTile);

				delete outputTile;	
				//write_tiler.putCvTile( *outputTile, 0);

				std::chrono::high_resolution_clock::time_point stopT = std::chrono::high_resolution_clock::now();
				std::chrono::duration<double> time_spanT = std::chrono::duration_cast<std::chrono::duration<double> >(stopT - startT);
#if TIMIN_ON
				std::cout << "Time : " << time_spanT.count() <<  std::endl;
#endif
			}

			/*std::chrono::high_resolution_clock::time_point stopI = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double> time_spanI = std::chrono::duration_cast<std::chrono::duration<double> >(stopI - startI);

			double pixels_per_second = numPixels / time_spanI.count();
			double seconds_per_pixel = time_spanI.count() / numPixels;*/

			//std::cout << "It took me    :" << time_spanI.count() <<  std::endl;
			//std::cout << "Pixels/Second :" << pixels_per_second <<  std::endl;
			//std::cout << "Seconds/Pixel :" << seconds_per_pixel <<  std::endl;

			write_tiler.close();
			read_tiler.close();
			/*inputTile = read_tiler.getCvTile<short>(4);
	
			whs(inputTile);
			whs.copyTileFromDevice(&outputTile);
			TS_ASSERT_EQUALS(outputTile->getBandCount(),5);*/
			/*Calculate Window Histogram Statistics for each pixel*/
			
	
		}


		void testTimingVariousTileSizesAcrossWholeImage () {

			std::cout << std::endl << "TESTING WHOLE IMAGE TIMING" << std::endl;
			std::vector<cv::Size2i> tileSizes;

			tileSizes.push_back(cv::Size2i(256,256));
			tileSizes.push_back(cv::Size2i(512,512));
			tileSizes.push_back(cv::Size2i(1024,1024));
			tileSizes.push_back(cv::Size2i(2048,2048));

			int cuda_device_id = 0;
			unsigned int unbuffered_data_width = 0;
			unsigned int unbuffered_data_height = 0;
			unsigned int window_size = 2;
			cvt::cvTile<short> inputTile;
			cvt::cvTile<float> *outputTile;


			cvt::Tiler read_tiler;
			cvt::Tiler write_tiler;

			cv::Size2i tSize(256,256);
			read_tiler.setCvTileSize(tSize);
			write_tiler.setCvTileSize(tSize);
			unsigned long numPixels = tSize.area();

			//read_tiler.open("test2.tif");
			std::string outFile = "windowHistogram.tif";


			if(boost::filesystem::exists(outFile)){
				boost::filesystem::remove(outFile);
			}
			/* Ensure tiler opens outfile correctly */
			TS_ASSERT_EQUALS(cvt::NoError, 
				write_tiler.create(outFile, "GTiff", 
				tSize, 
				5, cvt::Depth16U))
			

	
			std::string sourceFile("/raiddata/test_data/po_37704_pan_0000000.tif");
			read_tiler.open(sourceFile); // need big image

				for (size_t i = 0; i < tileSizes.size(); ++i) {
				/* Loop through all the tiles in the image */

				cv::Size2i tSize = tileSizes[i];
				read_tiler.setCvTileSize(tSize);
				write_tiler.setCvTileSize(tSize);
				unsigned long numPixels = tSize.area();

				std::chrono::high_resolution_clock::time_point startI = std::chrono::high_resolution_clock::now();


			
				cvt::gpu::GpuWHS<short,1,float,5> whs(cuda_device_id,
					tSize.width,tSize.height,window_size);

				whs.initializeDevice(cvt::gpu::SQUARE);



				for(int j = 0; j < read_tiler.getCvTileCount(); ++j){

					std::chrono::high_resolution_clock::time_point startT = std::chrono::high_resolution_clock::now();

					/* Retrieve a tile, with 126 pixel edge buffer */
					const cvt::cvTile<short> tile = read_tiler.getCvTile<short>(j); //126 pixel buffer

					/* Do the Window Histogram Stats */

					whs(tile,(const cvt::cvTile<float> **)&outputTile);
					write_tiler.putCvTile( *outputTile  , j);
					delete outputTile; 

					std::chrono::high_resolution_clock::time_point stopT = std::chrono::high_resolution_clock::now();
					std::chrono::duration<double> time_spanT = std::chrono::duration_cast<std::chrono::duration<double> >(stopT - startT);
#if TIMING_ON	
					std::cout << "Tile : " << time_spanT.count() <<  std::endl;
#endif

				}

				std::chrono::high_resolution_clock::time_point stopI = std::chrono::high_resolution_clock::now();
				std::chrono::duration<double> time_spanI = std::chrono::duration_cast<std::chrono::duration<double> >(stopI - startI);

				double pixels_per_second = numPixels / time_spanI.count();
				double seconds_per_pixel = time_spanI.count() / numPixels;
#if TIMING_ON	
				std::cout <<  tileSizes[i].width << " it took me :" << time_spanI.count() <<  std::endl;
#endif
				//std::cout << << " Pixels/Second :" << pixels_per_second <<  std::endl;
				//std::cout << << " Seconds/Pixel :" << seconds_per_pixel <<  std::endl;
			}
			write_tiler.close();
			read_tiler.close();	
		
		}
};

#endif
