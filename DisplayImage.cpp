#include <stdio.h>
#include <opencv2/opencv.hpp>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <math.h>

#include <iostream>
#include <fstream>

using namespace cv;

RNG rng(12345);


int findmax(std::vector<int> status) {
    int max;
    if (status.size() > 0) {
        max = status[0];
    }
    for (int i = 1; i < status.size(); i++) {
        if (status[i] > max) {
            max = status[i];
        }
    }
    return max;
}

float contDist(Point p1, Point p2) {
    return sqrt(pow(p2.x - p1.x, 2) + pow(p2.y - p1.y, 2));
}

Point midpoint(Point p1, Point p2) {
    return Point(((p1.x + p2.x) / 2.0), ((p1.y + p2.y) / 2.0));
}


void exportBoundingBox(RotatedRect r, float ratio) {
    std::ofstream outfile;
    outfile.open("BoundingBoxes.txt");
    outfile << "[[";
    Point2f rect_points[4]; 
    r.points(rect_points);
    for (int i = 0; i < 4; i++) {
        outfile << "[" << (int) (rect_points[i].x * ratio)  << "," << (int) (rect_points[i].y * ratio) << "]";
        outfile << ",";
    }
    outfile << "[" << (int) (rect_points[0].x * ratio)  << "," << (int) (rect_points[0].y * ratio) << "]";
    outfile << ",";    
    //check if spacing of the bounding box requires middle lanes 
    if (contDist(rect_points[0], rect_points[1]) > 200.0f) {
        //find midpoint between the 2
        Point p = midpoint(rect_points[1], rect_points[2]);
        outfile << "[" << (int) (p.x * ratio)  << "," << (int) (p.y * ratio) << "]";
        Point p2 = midpoint(rect_points[3], rect_points[0]);
        outfile << ",";
        outfile << "[" << (int) (p2.x * ratio)  << "," << (int) (p2.y * ratio) << "]";
    }
    if (contDist(rect_points[1], rect_points[2]) > 200.0f) {
        //find midpoint between the 2
        Point p = midpoint(rect_points[0], rect_points[1]);
        outfile << "[" << (int) (p.x * ratio)  << "," << (int) (p.y * ratio) << "]";
        Point p2 = midpoint(rect_points[2], rect_points[3]);
        outfile << ",";
        outfile << "[" << (int) (p2.x * ratio)  << "," << (int) (p2.y * ratio) << "]";
    }


    outfile << "]]";

    outfile.close();
}


/*
Checks the proximity of the 2 contours. 
If the distance between them is less than 50, then return 1 
else return 0
*/
int checkProximity(std::vector<Point> c1, std::vector<Point> c2) { 
    for(int i = 0; i < c1.size(); i++) { //Iterate through all of the points in the first contour
        for(int j = 0; j < c2.size(); j++) { //Iterate through all of the points in the second contour
            float distance = contDist(c1[i], c2[j]);
            if (distance < 10.0f) {
                return 1;
            }
        }
    }
    return 0;
}



void render(Mat image, int highThreshold, int lowThreshold, int blurKernal, float ratio) {
    int kernel_size = 3;

    Mat blurimg;
    blur(image, blurimg, Size(blurKernal,blurKernal));

    Mat edges;
    Canny(blurimg, edges, lowThreshold, highThreshold, kernel_size);

    std::vector<std::vector<Point> > contours;

    //Generate the countours 
    findContours(edges, contours, CV_RETR_TREE, CHAIN_APPROX_SIMPLE);

    Mat drawing = Mat::zeros( edges.size(), CV_8UC3 );

    std::vector<int> status;

    for (int i = 0; i < contours.size(); i++) {
        status.push_back(0);
    } 

    int dist;
    int val;
    for (int i = 0; i < contours.size(); i++) {
        int x = i;
        if (i != contours.size() - 1) {
            for (int j = i + 1; j < contours.size(); j++) {
                x++;
                dist = checkProximity(contours[i], contours[j]);
                if (dist == 1) {  //They are close so needs to be merged
                    val = min(status[i], status[x]);
                    status[x] = status[i] = val;
                }
                else {
                    if (status[x] == status[i]) {
                        status[x] = i + 1;
                    }
                }
            }
        }
    } 
    std::vector<std::vector<Point> > unified;
    int maximum = findmax(status) + 1;
    for (int i = 0; i < maximum; i++) {
        std::vector<int> pos;
        for (int j = 0; j < status.size(); j++) {
            if (i == status[i]) {
                pos.push_back(j);
            }
        }
        if (pos.size() != 0) {
            std::vector<Point> cont;
            for (int k = 0; k < pos.size(); k++) {
                cont.insert(cont.end(), contours[pos[k]].begin(), contours[pos[k]].end());
            }
            std::vector<Point> hull;
            convexHull(cont, hull);
            unified.push_back(hull);
        }
    }   

    drawContours(drawing,unified,-1,Scalar(0,255,0),1);

    //drawContours(image,contours,-1,Scalar(0,255,0),1);

    //////////////////////////////////////////////////////////////////

    std::vector<std::vector<Point> > contours_poly(unified.size());
    std::vector<RotatedRect> boundRect(unified.size());

    for(int i = 0; i < unified.size(); i++) { 
        approxPolyDP(Mat(unified[i]), contours_poly[i], 3, true);
        boundRect[i] = minAreaRect(Mat(contours_poly[i]));
    }

    exportBoundingBox(boundRect[0], ratio);

    for(int i = 0; i < unified.size(); i++) { 
        Scalar color = Scalar(rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255));
        //drawContours(drawing, contours, 0, color, 1);
        Point2f rect_points[4]; 
        boundRect[0].points(rect_points);
        for(int j = 0; j < 4; j++) {
          line(image, rect_points[j], rect_points[(j+1)%4], color, 1, 8);
        }
    }
    namedWindow("Shipwreck", WINDOW_AUTOSIZE);
    imshow("Shipwreck", image);
    imwrite( "outfile.png", image);
}

int main(int argc, char** argv )
{
    if (argc < 6)
    {
        printf("usage: DisplayImage.out <Image_Path> <lowThreshold> <highThreshold> <blurKernalSize> <imageWidth> <imageHeight>\n");
        return -1;
    }

    int lowThreshold = atoi(argv[2]);
    int highThreshold = atoi(argv[3]); 
    int blurKernal = 15;

    Mat image;
    image = imread(argv[1], 1);

    if (!image.data)
    {
        printf("No image data \n");
        return -1;
    }

    blurKernal = atoi(argv[4]);
    int outputCols = atoi(argv[6]);
    int outputRows = atoi(argv[5]);

    printf("Image.rows: %d\n", image.rows);
    printf("Image.rows: %d\n", image.cols);
    float ratio = (float) outputRows / (float) image.rows;
    printf("Ratio is: %f\n", ratio);


    render(image, highThreshold, lowThreshold, blurKernal, ratio);

    int key;
    while ((key = waitKey(30)) != 27) {
        if (key == 119) { //w is pressed [INCREASE HIGHTHRESHOLD]
            if (highThreshold < 255) {
                highThreshold++;
                render(image, highThreshold, lowThreshold, blurKernal, ratio);
            }
            printf("highThreshold: %d\n", highThreshold);
        }
        else if (key == 115) { //s is pressed [DECREASE HIGHTHRESHOLD]
            if (highThreshold > 0) {
                highThreshold--;
                render(image, highThreshold, lowThreshold, blurKernal, ratio);
            }
            printf("highThreshold: %d\n", highThreshold);
        }
        else if (key == 113) { //q is pressed [INCREASE LOWTHRESHOLD]
            if (lowThreshold < 255) {
                lowThreshold++;
                render(image, highThreshold, lowThreshold, blurKernal, ratio);
            }
            printf("lowThreshold: %d\n", lowThreshold);
        }
        else if (key == 97) { //a is pressed [DECREASE LOWTHRESHOLD]
            if (lowThreshold > 0) {
                lowThreshold--;
                render(image, highThreshold, lowThreshold, blurKernal, ratio);
            }
            printf("lowThreshold: %d\n", lowThreshold);
        }
        else if(key == 43) { //'+' is pressed [INCREASE BLUR]
            if (blurKernal < 30) {
                blurKernal++;
                render(image, highThreshold, lowThreshold, blurKernal, ratio);             
            }
            printf("BlurKernalSize: %d\n", blurKernal);
        }
        else if(key == 45) { //'-' is pressed [DECREASE BLUR]
            if (blurKernal > 1) {
                blurKernal--;
                render(image, highThreshold, lowThreshold, blurKernal, ratio);
            }
            printf("BlurKernalSize: %d\n", blurKernal);
        }
    }



    return 0;
}
