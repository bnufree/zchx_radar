#include <QCoreApplication>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui_c.h>
#include <iostream>
#include <math.h>
#include <QDebug>
#include <QImage>


using namespace cv;
using namespace std;
Mat src, dst, gray_src;
char input_image[] = "input image";
char output_image[] = "output image";

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    qDebug("start load image ..\n");
#if 0
    QImage img;
    QString destImgName("result.png");
    if(img.load("test.PNG"))
    {
        QImage dest = img.copy();
        for(int y = 0; y<dest.height(); y++)
        {
            for(int x=0; x<dest.width(); x++)
            {
                QColor color = dest.pixelColor(x, y);
                if(color.red() || color.green() || color.blue())
                {
                    dest.setPixelColor(x, y, QColor(255, 255, 255));
                } else
                {
                    dest.setPixelColor(x, y, QColor(0, 0, 0));
                }
            }
        }
        dest.save(destImgName, "PNG");
    }
#endif
    src = imread("test1111.PNG");
    if (src.empty()){
        printf("colud not load image ..\n");
        return -1;
    }

    namedWindow(input_image, CV_WINDOW_AUTOSIZE);
    namedWindow(output_image, CV_WINDOW_AUTOSIZE);
    imshow(input_image, src);
    // 均值降噪
    Mat blurImg;
    GaussianBlur(src, blurImg, Size(15, 15), 0, 0);
    imshow("input image2", src);
    // 二值化
    Mat binary;
    cvtColor(blurImg, gray_src, COLOR_BGR2GRAY);
    threshold(gray_src, binary, 0, 255, THRESH_BINARY /*| THRESH_TRIANGLE*/);
    imshow("binary", binary);
#if 0

//    // 闭操作进行联通物体内部
//    Mat morphImage;
//    Mat kernel = getStructuringElement(MORPH_RECT, Size(3, 3), Point(-1, -1));
//    morphologyEx(binary, morphImage, MORPH_CLOSE, kernel, Point(-1, -1), 2);
//    imshow("morphology", morphImage);
#endif

    // 获取最大轮廓
    vector<vector<Point>> contours;
    vector<Vec4i> hireachy;
    findContours(binary, contours, hireachy, CV_RETR_LIST, CV_CHAIN_APPROX_NONE, Point());

    Mat connImage = Mat ::zeros(src.size(),CV_8U);
     drawContours(connImage, contours, -1, Scalar(255, 0, 255));
 #if 0
    Mat connImage = Mat::zeros(src.size(), CV_8UC3);
    for (size_t t = 0; t < contours.size(); t++){
        Rect rect = boundingRect(contours[t]);
        if (rect.width < src.cols / 2) continue;
        if (rect.width > src.cols - 20) continue;

        double area = contourArea(contours[t]);
        double len = arcLength(contours[t], true);


        drawContours(connImage, contours, t, Scalar(0, 0, 255), 1, 8, hireachy);
        printf("area of star could : %f \n", area);
        printf("lenght of star could : %f \n", len);
    }
#endif
    imshow(output_image, connImage);


    waitKey(0);

    return 0;
}
