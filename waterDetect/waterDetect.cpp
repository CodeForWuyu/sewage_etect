//程序说明
//1、需要修改原始视频路径和结果输出视频路径
//2、运行检测需要修改第41行，选择要检测的视频


#include <opencv2\opencv.hpp>

using namespace cv;
using namespace std;

void drawPolygon(Mat img, int a[], int b[]);
Mat detect_black(Mat img);
Mat get_color_result(Mat img, Mat detect, double red = 0.5);
Mat fillHole(Mat img);
Mat foam_detect(Mat img);
Mat dirty_detect(Mat img);
Mat mergeVideo(Mat ori, Mat detect);

//原视频路径
String black_path = "H://data//异常视频//cut//黑cut.avi";
String foam_path = "H://data//异常视频//cut//泡cut.avi";
String dirty_path = "H://data//异常视频//cut//浊cut.avi";

//视频结果输出路径
String black_out_path = "H://data//异常视频//cut//黑color.avi";
String foam_out_path = "H://data//异常视频//cut//泡color.avi";
String dirty_out_path = "H://data//异常视频//cut//浊color.avi";

//extract region 提取水域
int black_x[7] = { 90,202,311,333,480,45,25 }; //for black
int black_y[7] = { 356,350,434,433,720,720,431 };
int dirty_x[7] = { 87,112,218,234,375,31,15 }; //for foam and dirty
int dirty_y[7] = { 333,321,413,418,720,720,404 };

Mat foam_normal;
Mat dirty_normal;

int black = 0;	//黑
int foam = 1;	//泡沫
int dirty = 2;	//脏水
int flag = black;		//选择检测视频方案

int main()
{
	String ori_path;
	String out_path;
	if (flag == black) {
		ori_path = black_path;
		out_path = black_out_path;
	}
	else if (flag == foam) {
		ori_path = foam_path;
		out_path = foam_out_path;
	}
	else {
		ori_path = dirty_path;
		out_path = dirty_out_path;
	}
	

	VideoCapture cap;
	cap.open(ori_path);
	if (!cap.isOpened()) {
		cout << "cannot open file" << endl;
	}
	Mat frame;
	cap.read(frame);

	VideoWriter writer(out_path,
		CV_FOURCC('M', 'P', '4', '2'), 50.0, frame.size());

	Mat mask = Mat::zeros(frame.size(), CV_8UC3);

	if (flag == black) {
		drawPolygon(mask, black_x, black_y);	//黑水水域  
	}
	else {
		drawPolygon(mask, dirty_x, dirty_y);	//泡沫和脏水水域
	}

	Mat masked;

	int count = 0;
	while (true) {
		if (!frame.data) {
			break;
		}
		bitwise_and(frame, mask, masked);
		//imshow("masked", masked);
		if (count == 0) {		//记录第一帧
			masked.copyTo(foam_normal);
			masked.copyTo(dirty_normal);
		}
		count++;
		//imshow("ori", frame);

		//-----------------检测过程-----------------------------
		Mat detect;
		if (flag == black) {
			detect = detect_black(masked);	//黑水检测
		}
		else {
			detect = dirty_detect(masked); //泡沫或污水检测 
		}
		
		Mat colored_result = get_color_result(frame, detect, 1.0);//颜色标注
		Mat final = mergeVideo(frame, colored_result);  //视频合并

		//writer << final;  //写出视频
		//-----------------检测过程-----------------------------
		waitKey(20);

		cap.read(frame); //读取下一帧	
	}
	cap.release();
	writer.release();

	waitKey(0);
	return 0;
}

//绘制多边形mask
void drawPolygon(Mat img, int a[], int b[])
{
	Point points[1][7];

	for (int i = 0; i < 7; i++) {
		points[0][i].x = a[i];
		points[0][i].y = b[i];
	}

	const Point* ppt[1] = { points[0] };
	int npt[] = { 7 };

	fillPoly(img, ppt, npt, 1, Scalar(255, 255, 255), 8);

}

//泡沫检测  （弃用） 使用dirty_detect
Mat foam_detect(Mat img)
{
	Mat gray, normal_gray;
	cvtColor(img, gray, COLOR_BGR2GRAY);
	cvtColor(foam_normal, normal_gray, COLOR_BGR2GRAY);


	Mat diff;
	absdiff(gray, normal_gray, diff);
	Mat thresh;
	threshold(diff, thresh, 40, 255, THRESH_BINARY);

	Mat element = getStructuringElement(MORPH_RECT, Size(5, 5));
	Mat erode_thresh, dilate_thresh;
	erode(thresh, erode_thresh, element);
	dilate(erode_thresh, dilate_thresh, element);

	imshow("dilate", dilate_thresh);
	//imshow("thresh", thresh);
	//imshow("diff", diff);
	//imshow("gray", gray);
	imshow("thresh", thresh);

	return dilate_thresh;
}

//脏水检测
Mat dirty_detect(Mat img) {
	Mat hsv_img, hsv_normal;
	cvtColor(img, hsv_img, COLOR_BGR2HSV);
	cvtColor(dirty_normal, hsv_normal, COLOR_BGR2HSV);

	vector<Mat> channels1, channels2;
	split(hsv_img, channels1);
	split(hsv_normal, channels2);
	//imshow("h", channels1[0]); imshow("s", channels1[1]); imshow("v", channels1[2]);
	Mat s = channels1[1];
	Mat s_normal = channels2[1];

	Mat diff;
	absdiff(s, s_normal, diff);

	Mat thresh_diff, thresh_s;

	threshold(diff, thresh_diff, 65, 255, THRESH_BINARY);

	Mat element = getStructuringElement(MORPH_RECT, Size(7, 7));
	Mat erode_thresh, dilate_thresh;
	erode(thresh_diff, erode_thresh, element);
	dilate(erode_thresh, dilate_thresh, element);

	//imshow("s_thresh", thresh_s);
	//imshow("s", s);
	//imshow("dilate", dilate_thresh);
	//imshow("diff thresh", thresh_diff);
	//imshow("h", channels1[0]);
	//imshow("diff", diff);
	Mat result = fillHole(dilate_thresh);
	return result;
}

//黑水检测
Mat detect_black(Mat img)
{
	Mat hsv_img;
	cvtColor(img, hsv_img, COLOR_BGR2HSV); //hue通道检测

	Mat gray;
	cvtColor(img, gray, COLOR_BGR2GRAY);//灰度图检测

	vector<Mat> channels;
	split(hsv_img, channels);
	Mat h = channels[0];
	GaussianBlur(h, h, Size(5, 5), 10, 20);

	Mat thresh;
	threshold(h, thresh, 99, 255, THRESH_BINARY);
	Mat element = getStructuringElement(MORPH_RECT, Size(5, 5));
	Mat dilate_thresh, erode_thresh;
	erode(thresh, erode_thresh, element);
	Mat element1 = getStructuringElement(MORPH_ELLIPSE, Size(5, 5));
	dilate(erode_thresh, dilate_thresh, element1);

	//imshow("dilate", dilate_thresh);
	Mat without_hole = fillHole(dilate_thresh);
	//imshow("gray", gray);
	//imshow("h", h);
	//imshow("thresh", thresh);


	return without_hole;
}

//上色
Mat get_color_result(Mat img, Mat detect, double red)
{
	Mat result;

	vector<Mat> channels;
	split(img, channels);
	Mat r = channels[2];

	addWeighted(r, 1 - red, detect, red, 0, r);
	merge(channels, result);

	//imshow("color result", result);
	return result;
}

//空洞填充
Mat fillHole(Mat img)
{
	Mat filled;

	Mat tempColor;
	cvtColor(img, tempColor, COLOR_GRAY2BGR);
	vector<vector<Point> > contours;
	vector<Vec4i>hierarchy;
	findContours(img, contours, cv::noArray(), cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);
	for (int i = 0; i < (int)contours.size(); i++) {
		if (contourArea(contours[i], true) < 200) {
			drawContours(tempColor, contours, i, Scalar(255, 255, 255), CV_FILLED);
		}
	}
	cvtColor(tempColor, tempColor, COLOR_BGR2GRAY);
	threshold(tempColor, filled, 50, 255, THRESH_BINARY);
	//imshow("filled", tempColor);
	return filled;
}

//合并视频并显示
Mat mergeVideo(Mat ori, Mat detect) {

	Size size = ori.size();
	Mat final_img = Mat::zeros(Size(size.width * 2, size.height), CV_8UC3);
	int fontface = cv::FONT_HERSHEY_PLAIN;
	double fontScale = 2;

	ori.copyTo(final_img(Rect(0, 0, size.width, size.height)));
	detect.copyTo(final_img(Rect(size.width, 0, size.width, size.height)));
	putText(final_img, "DETECT", Point(size.width + 10, 40), fontface, fontScale, Scalar(0, 0, 255), 2);

	imshow("final", final_img);
	return final_img;

}
