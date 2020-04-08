#pragma once

#include <QtWidgets/QWidget>
#include "ui_CameraCalibration.h"
#include <qdebug.h>
#include <iostream>
#include <vector>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/calib3d.hpp>
#include <qfiledialog.h>  
#include <Windows.h>



class CameraCalibration : public QWidget
{
	Q_OBJECT

public:
	CameraCalibration(QWidget *parent = Q_NULLPTR);
	void showImage(cv::Mat imshowsrc, QLabel * label);//显示图像
	double C_CX = 165.6507875355497,
		C_CY = 120.9800873314841,
		C_FX = 186.0929297122383,
		C_FY = 186.5344394192148,
		C_K1 = -0.2636080098584763,
		C_K2 = 0.02495287341003726;

private:
	Ui::CameraCalibrationClass ui;
	std::vector<cv::Mat> checkerboardPatterns;
	cv::Mat camereaMatrixTemp = cv::Mat(3, 3, CV_32FC1, cv::Scalar::all(0));//内参矩阵
	cv::Mat distCoffeffsTemp = cv::Mat(1, 5, CV_32FC1, cv::Scalar::all(0)); //畸变系数
	int cornerOfRow;
	int cornerOfCol;
	int CurrentImgNum = 0; //畸变预览图片位置
	bool needXFlip = false, needYFlip = false, needCut = true;
	void textInsert(uchar num);
	void calculateCameraParameters(std::vector<cv::Mat> srcImgs);
	cv::Mat imageUndist(cv::Mat src);
	cv::Mat regular_img(cv::Mat image);
	
private slots:
	void on_loadImgBtn_clicked();
	void on_calparamsBtn_clicked();
	void on_cornersOfColEdit_textChanged(const QString & num);
	void on_cornersOfRowEdit_textChanged(const QString & num);
	void on_xFlipCheck_stateChanged(int state);
	void on_yFlipCheck_stateChanged(int state);
	void on_cutImgCheck_stateChanged(int state);				//裁剪图片选择
	void on_nextCalibImgBtn_clicked();							//畸变矫正预览
	void on_lastCalibImgBtn_clicked();
	void on_calibParamSetBtn_clicked();							//应用畸变系数
signals:
	void loadAllImages(std::vector<cv::Mat> imgVectors);
};
