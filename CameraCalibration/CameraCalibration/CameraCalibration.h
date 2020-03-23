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
	void textInsert(uchar num);
private:
	Ui::CameraCalibrationClass ui;
	std::vector<cv::Mat> checkerboardPatterns;
	int cornerOfRow = 6;
	int cornerOfCol = 4;
	bool needXFlip, needYFlip;
	void calculateCameraParameters(std::vector<cv::Mat> srcImgs);
	
private slots:
	void on_loadImgBtn_clicked();
	void on_calparamsBtn_clicked();
	void on_cornersOfColEdit_textChanged(const QString & num);
	void on_cornersOfRowEdit_textChanged(const QString & num);
	void on_xFlipCheck_stateChanged(int state);
	void on_yFlipCheck_stateChanged(int state);

signals:
	void loadAllImages(std::vector<cv::Mat> imgVectors);
};
