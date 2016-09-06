/* robogame_kinectfeatures_extractor
 * utils.cpp -- This file is part of the robogame_kinectfeatures_extractor ROS node created for
 * the purpose of extracting relevant motion features from images.
 *
 * Copyright (C) 2016 Ewerton Lopes
 *
 * LICENSE: This is a free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License (LGPL v3) as
 * published by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version. This code is distributed in the hope
 * that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License (LGPL v3): http://www.gnu.org/licenses/.
 */

#include <ros/ros.h>    // ROS
#include <math.h>       /* ceil */
#include <string>
#include "utils.h"
#include "common.h"

/* printMatImage -- a function to print a cv::Mat to console. It
Basically serve as for debugging purposes*/
void printMatImage(cv::Mat _m){
    printf("\n --> Matrix type is CV_32F \n");

    for( size_t i = 0; i < _m.rows; i++ ) {
        for( size_t j = 0; j < _m.cols; j++ ) {
   	    printf( " %.3f  ", _m.at<float>(i,j) );
 	}
	printf("\n");
    }
}

/* writeMatToFile -- a function to save a cv::Mat to file. Mainly
used for frame saving purposes.*/
void writeMatToFile(cv::Mat& m, const char* filename)
{
    std::ofstream fout(filename);           // Define file.
    if(!fout){
        std::cout << "File Not Opened" << std::endl;  return;
    }
    /*Loop through pixels in the cv::Mat file*/
    for(unsigned int i=0; i<m.rows; i++){
        for(unsigned int j=0; j<m.cols; j++){
            fout << m.at<float>(i,j) << " ";
        }
        fout << std::endl;
    }
    fout.close();                           // close file
}

/* resize -- A helper function to resize image. Here, the width is
default to 512 as it is the width of a kinect frame. That is the only
reason for doing it.*/
void resize(cv::Mat& image, cv::Mat& dst, int width=512, int height=-1){
    // initialize the dimensions of the image to be resized and
    // grab the image size

	cv::Size dim;
    int h = image.size().height;
	int w = image.size().width;

    // if both the width and height are None, then return the
    // original image
    if ((width == -1) && (height==-1)){
        std::cerr << "You have to specify width or height!" << std::endl;
		exit(-1);
	}else{
		// check to see if the width is None
		if (width == -1){
		    // calculate the ratio of the height and construct the
		    // dimensions
		    float r = height / float(h);
		    dim = cv::Size(int(w * r), height);
		}
		// otherwise, the height is None
		else{
		    // calculate the ratio of the width and construct the
		    // dimensions
		    float r = width / float(w);
		    dim = cv::Size(width, int(h * r));
		}
		// resize the image
		cv::Mat resized(dim,CV_8UC3);
		cv::resize(image, resized, resized.size(),0,0, CV_INTER_AREA);

		// return the resized image
		dst = resized.clone();
	}
}

/* trackUser -- Function used to track color blobs on a RGB image. */
void trackUser(cv::Mat& src){
	/* resize the frame and convert it to the HSV
	color space... */
	cv::Mat frame(src.size(), src.type());                         // make copy
	resize(src,frame);                                             // resize
    cv::Mat hsv = cv::Mat::zeros(frame.size(), frame.type());      // define container for the converted Mat.
	cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);                   // convert
    /* ... */

    /* construct a mask for the color (default to "green"), then perform
	   a series of dilations and erosions to remove any small
       blobs left in the mask... */
	cv::Mat mask;
	cv::inRange(hsv,cv::Scalar(hMin,sMin,vMin), cv::Scalar(hMax,sMax,vMax), mask);
	cv::Mat erodeElement = cv::getStructuringElement(cv::MORPH_RECT,cv::Size(3,3));
	cv::Mat dilateElement = cv::getStructuringElement(cv::MORPH_RECT,cv::Size(8,8));
    // perform erode and dilate operations.
    cv::erode(mask, mask, erodeElement);
	cv::erode(mask, mask, erodeElement);
	cv::dilate(mask, mask, dilateElement);
	cv::dilate(mask, mask, dilateElement);
    /* ... */
	cv::imshow("mask",mask);           // exihbit mask.


	// find contours in the mask and initialize the current
	// (x, y) center of the ball
	std::vector<std::vector<cv::Point> > contours;  // container for the contours
	std::vector<cv::Vec4i> hierarchy;
    cv::findContours(mask.clone(), contours,
                     hierarchy,                     /* find the image contours */
                     CV_RETR_EXTERNAL,
                     CV_CHAIN_APPROX_SIMPLE);

    cv::Point2f center(-1000,-1000);                /* define center. Set to
                                                        arbitrary init value */
    missedPlayer = true;                            /* flag to track the Player
                                                        presence.*/

	/* Only proceed if at least one contour was found, i.e., if at least one
        object has been tagged as a target (NOTE: that the contour correspond
        to the color blog dettected) */
	if (contours.size() > 0){

		int largest_area=0;               // container for the max area
		int largest_contour_index=0;      /* container for the index of the
                                              max area found in countours */

		/* find the largest contour in the mask, then use
    	   it to compute the minimum enclosing circle and
           centroid.*/
		for(int i=0; i < contours.size();i++){
			// iterate through each contour.
			double a = cv::contourArea(cv::Mat(contours[i]),false);  //  Find the area of contour
            /* if the area is bigger than the lready found one, update it.*/
            if(a > largest_area){
				largest_area=a;
				largest_contour_index=i;
			}
		}

		cv::Point2f tempcenter;
  		float radius;
		cv::minEnclosingCircle((cv::Mat)contours[largest_contour_index], tempcenter, radius);
		cv::Moments M = cv::moments((cv::Mat)contours[largest_contour_index]);
		center = cv::Point2f(int(M.m10 / M.m00), int(M.m01 / M.m00));

		/* Only proceed if the radius meets a minimum size. This is used to restrict
            the size of the object detected.*/
		if (radius > 15){
			// draw the circle and centroid on the frame,
			// then update the list of tracked points
			cv::circle(frame, cv::Point(int(tempcenter.x), int(tempcenter.y)), int(radius), cv::Scalar(0, 255, 255), 2);
			cv::circle(frame, center, 5, cv::Scalar(0, 0, 255), -1);
            mainCenter = center;            // save the center of the detected circle.
            missedPlayer = false;           // update the flag for the player presence.
		}
	}

	// update the points queue
	pts.push_front(center);

	/* Loop over the set of tracked points (i.e, the history of points)
        in order to print a line in the frame representing the history of
        tracked points. This line color is set to RED*/
	for (int i=1; i < (pts.size()-1); i++){
		/* if either of the tracked points are NONE, ignore them.
            NOTE: here at this code, a point set to NONE is the one
                  set to -1000; */
		cv::Point2f ptback = pts[i - 1];
		cv::Point2f pt = pts[i];
		if ((ptback.x == -1000) or (pt.x == -1000)){
			continue;
		}

		/* otherwise, compute the thickness of the line and
		    draw the connecting lines */
		int thickness = int(sqrt(BUFFER / float(i + 1)) * 2.5);
		line(frame, pts[i - 1], pts[i], cv::Scalar(0, 0, 255), thickness);
	}
	src = frame.clone();   // update the input frame.
}

/* distanceFunction -- used to compute the similarity betweent the pixels.*/
bool distanceFunction(float a, float b, int threshold){
    if(abs(a-b) <= threshold) return true;
	else return false;
}

/* segmentDepth -- a function that implements a "Region Growing algorithm", which
 is defined here in a "Breadth-first search" manner.
	sX --> Seed Pixel x value (columns == width)
	sY --> Seed Pixel y value (rows == height)
	threshold --> the value to be used in the call to "distanceFunction" method. If distance
    is less than threshold then recursion proceeds, else stops.
*/
void segmentDepth(cv::Mat& input, cv::Mat& dst, int sX, int sY, float& ci, int threshold)
{

	long int pixels = 0;                           // segmented pixels counter variable.
	std::vector< std::vector<int> > reach;	       // This is the binary mask for the segmentation.
	for (int i = 0; i < input.rows; i++){	//They are set to 0 at first. Since no pixel is assigned to the segmentation yet.
		reach.push_back(std::vector<int>(input.cols));
	}

	// Define the queue. NOTE: it is a BFS based algorithm.
	std::queue< std::pair<int,int> > seg_queue;

	// verify the depth value of the seed position.
	float &in_pxl_pos = input.at<float>(sY,sX);

    if(in_pxl_pos == 0){
        ROS_WARN_STREAM("THE SEED DEPTH VALUE IS ZERO!!!!!");
    }else if (missedPlayer){
        ROS_INFO_STREAM("PLAYER IS MISSING!");          //Send a message to rosout
        ci = -1 ;                                       //Set the value indicating player missing.
    }else{
        dst.at<float>(sY,sX) = 255;                     // add seed to output image.

        // Mark the seed as 1, for the segmentation mask.
    	reach[sY][sX] = 1;
        pixels++;                                        // increase the pixel counter.

    	// init the queue witht he seed.
        seg_queue.push(std::make_pair(sY,sX));

        /* Loop over the frame, based on the seed adding a
            new pixel to the dst frame accordingly*/
    	while(!seg_queue.empty())
    	{
            /* pop values */
    		std::pair<int,int> s = seg_queue.front();
    		int x = s.second;
    		int y = s.first;
            seg_queue.pop();
            /* ... */

            /* The following "if" blocks analise the pixels incorporating them to the
                dst frame if they meet the threshold condition. */

    		// Right pixel
    		if((x + 1 < input.cols) && (!reach[y][x + 1]) &&
                distanceFunction(input.at<float>(sY,sX), input.at<float>(y, x + 1),
                threshold)){
                reach[y][x+1] = true;
                seg_queue.push(std::make_pair(y, x+1));
    			float &pixel = dst.at<float>(y,x+1);
                pixel = 255;
    			pixels++;;

    		}

    		//Below Pixel
    		if((y+1 < input.rows) && (!reach[y+1][x]) &&
                distanceFunction(input.at<float>(sY,sX), input.at<float>(y+1,x),
                threshold)){
    			reach[y + 1][x] = true;
    			seg_queue.push(std::make_pair(y+1,x));
    			dst.at<float>(y+1,x) = 255;
    			pixels++;;
    		}

    		//Left Pixel
    		if((x-1 >= 0) && (!reach[y][x-1]) &&
                distanceFunction(input.at<float>(sY,sX), input.at<float>(y,x-1),
                threshold)){
    			reach[y][x-1] = true;
    			seg_queue.push(std::make_pair(y,x-1));
    			dst.at<float>(y,x-1) = 255;
    			pixels++;;
    		}

    		//Above Pixel
    		if((y-1 >= 0) && (!reach[y - 1][x]) &&
                distanceFunction(input.at<float>(sY,sX), input.at<float>(y-1,x),
                threshold)){
    			reach[y-1][x] = true;
    			seg_queue.push(std::make_pair(y-1,x));
    			dst.at<float>(y-1,x) = 255;
    			pixels++;;
    		}

    		//Bottom Right Pixel
    		if((x+1 < input.cols) && (y+1 < input.rows) && (!reach[y+1][x+1]) &&
                distanceFunction(input.at<float>(sY,sX), input.at<float>(y+1,x+1),
                threshold)){
    			reach[y+1][x+1] = true;
    			seg_queue.push(std::make_pair(y+1,x+1));
    			dst.at<float>(y+1,x+1) = 255;
    			pixels++;
    		}

    		//Upper Right Pixel
    		if((x+1 < input.cols) && (y-1 >= 0) && (!reach[y-1][x+1]) &&
                distanceFunction(input.at<float>(sY,sX), input.at<float>(y-1,x+1),
                threshold)){
    			reach[y-1][x+1] = true;
    			seg_queue.push(std::make_pair(y-1,x+1));
    			dst.at<float>(y-1,x+1) = 255;
    			pixels++;
    		}

    		//Bottom Left Pixel
    		if((x-1 >= 0) && (y + 1 < input.rows) && (!reach[y+1][x-1]) &&
                distanceFunction(input.at<float>(sY,sX), input.at<float>(y+1,x-1),
                threshold)){
    			reach[y+1][x-1] = true;
    			seg_queue.push(std::make_pair(y+1,x-1));
    			dst.at<float>(y+1,x-1) = 255;
    			pixels++;
    		}

    		//Upper left Pixel
    		if((x-1 >= 0) && (y-1 >= 0) && (!reach[y-1][x-1]) &&
                distanceFunction(input.at<float>(sY,sX), input.at<float>(y-1,x-1),
                threshold)){
    			reach[y-1][x-1] = true;
    			seg_queue.push(std::make_pair(y-1,x-1));
    			dst.at<float>(y-1,x-1) = 255;
                pixels++;
    		}
    	}

        /* FROM THIS POINT ON: Initialization of supporting code for detecting the blob in the
            segmented frame. This is needed for drawing the minimum bounding rectangle
            for the detected blob, thus, enabling the "contraction index" feature
            calculation.*/
        std::vector<std::vector<cv::Point> > contours;
    	std::vector<cv::Vec4i> hierarchy;
        cv::Mat bwImage(input.size(),CV_8UC1);
        dst.convertTo(bwImage,CV_8U,255.0/(255-0));
    	cv::findContours(bwImage, contours, hierarchy,
                     CV_RETR_EXTERNAL,
                     CV_CHAIN_APPROX_SIMPLE);

        /* Only proceed if at least one contour was found. NOTE: This is need for avoiding
            "seg fault". */
    	if (contours.size() > 0){

            cv::Mat erodeElement = cv::getStructuringElement(cv::MORPH_RECT,cv::Size(3,3));
            cv::Mat dilateElement = cv::getStructuringElement(cv::MORPH_RECT,cv::Size(8,8));
            cv::erode(dst, dst, erodeElement);
            cv::dilate(dst, dst, dilateElement);

            int largest_area=0;
    		int largest_contour_index=0;

    		// find the largest contour in the mask, then use
    		// it to compute the minimum enclosing circle and
    		// centroid
    		for(int i=0; i < contours.size();i++){
    			// iterate through each contour.
                double a = cv::contourArea(cv::Mat(contours[i]),false);  //  Find the area of contour
    			if( a > largest_area){
    				largest_area=a;
    				largest_contour_index=i;   //Store the index of largest contour
    			}
    		}

            /*Calculate the minimum bounding box for the detected blob.*/
            cv::Rect rect = cv::boundingRect(contours[largest_contour_index]);
            cv::Point pt1, pt2;
            pt1.x = rect.x;
            pt1.y = rect.y;
            pt2.x = rect.x + rect.width;
            pt2.y = rect.y + rect.height;

            /* compute the Contraction index feature. It is the difference
            between the bounding rectangle area and the segmented object
            in the segmentation frame, normalized by
            the area of the rectangle. */
            cv::Mat roiSeg = cv::Mat(dst,rect);
            int roiSegarea = roiSeg.total();
            ci = float(roiSegarea-pixels)/float(roiSegarea);
            /* ... */

            /* COMPUTING THE MIDDLE POINT ON THE TOP OF THE SEGMENTED IMAGE */
            cv::Mat test = cv::Mat(dst,rect);
            int begin = 0;
            int loop_counter = 0;
            bool sign = false;
            int row = 10;
            for (int j=0;j < test.cols;j++){                 //Basically cost O(n)
                if (test.at<float>(row,j) == 255){
                    if(!sign){
                        begin = j;
                        sign = !sign;
                    }
                    loop_counter++;
                }else if (sign && test.at<float>(row,j) == 0){
                    break;
                }
            }
            /* --- */
            //writeMatToFile(test,"test.txt");

            int mid = std::ceil(loop_counter/2);
            int topPoint = begin + (mid-1);

            /* APPLY COLOR TO THE FRAME*/
            std::vector<cv::Mat> tempcolorFrame(3);                         // Used to print a colored segmented frame.
            cv::Mat black = cv::Mat::zeros(dst.rows, dst.cols, dst.type());
            tempcolorFrame.at(0) = black; //for blue channel
            tempcolorFrame.at(1) = dst;   //for green channel
            tempcolorFrame.at(2) = black;  //for red channel

            cv::merge(tempcolorFrame, segmentedColorFrame);
            // Draws the rect in the segmentedColorFrame image
            cv::circle(segmentedColorFrame, cv::Point(sX,sY),5, cv::Scalar(0,0,255),CV_FILLED, 8,0);
            segmentedTarget = cv::Mat(segmentedColorFrame,rect);
            cv::circle(segmentedTarget, cv::Point(topPoint,10),5, cv::Scalar(0,0,255),CV_FILLED, 8,0); // TAGGING THE POINT
            /* ---- */

            cv::rectangle(segmentedColorFrame, pt1, pt2, cv::Scalar(255,255,255), 2,8,0);// the selection white rectangle

            /* ... */


            /* put the CI value to frame */
            cv::putText(segmentedColorFrame,
            std::to_string(ci),
            cv::Point(rect.x,rect.y-5), // Coordinates
            cv::FONT_HERSHEY_COMPLEX_SMALL, // Font
            0.9, // Scale. 2.0 = 2x bigger
            cv::Scalar(255,255,255), // Color
            1 // Thickness
            ); // Anti-alias
            /*... */

        }
    }
}
