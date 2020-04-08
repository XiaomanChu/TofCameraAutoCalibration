#include "CameraCalibration.h"
using namespace std;
using namespace cv;
#pragma execution_character_set("utf-8")

CameraCalibration::CameraCalibration(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	cornerOfRow = ui.cornersOfRowEdit->text().toInt();
	cornerOfCol = ui.cornersOfColEdit->text().toInt();
	ui.currentParamShow->setText("当前畸变参数:\n CX =" + QString::number(C_CX) + "\n CY = " + QString::number(C_CY) + "\n FX = " + QString::number(C_FX) + "\n FY = " + QString::number(C_FY) + "\n K1 = " + QString::number(C_K1) + "\n K2 = " + QString::number(C_K2));
	
	//部分槽连接
	//TODO

}

/*!
* @brief 选择多张图片, 载入内存
*        存入变量：vector<cv::Mat> checkerboardPatterns
* @param NULL
* @return NULL
*/
void CameraCalibration::on_loadImgBtn_clicked()
{
	//qDebug() << "load images..." << endl;
	int imgNums = 0;
	checkerboardPatterns.clear();
	const QStringList imgPaths = QFileDialog::getOpenFileNames(this, tr("选取图片"), "", tr("图片格式 (*.png *.jpg);;All(*.*);;"));
	if (!imgPaths.isEmpty())
	{
		cv::Mat img;
		foreach(const QFileInfo &imgPath, imgPaths) {
			QString imageFile = imgPath.absoluteFilePath();
			if (needCut) img = regular_img(cv::imread(imageFile.toStdString())); //是否裁剪
			else img = cv::imread(imageFile.toStdString());
			showImage(img, ui.CalibImgViewer);
			ui.CalibImgViewer->repaint();  //强制更新
			checkerboardPatterns.push_back(img);//保存图片
		}

		imgNums = checkerboardPatterns.size();
		if (!imgNums) ui.imgLoadStatus->setText("尚未载入图片");
		else
		{	
			ui.imgLoadStatus->setStyleSheet("color:black");
			ui.imgLoadStatus->setText("载入图片数: " + QString::number(imgNums));
		}
		//emit loadAllImages(checkerboardPatterns);//信号：载入所有图片
	}
}

/*!
* @brief 计算畸变参数按钮槽
* @param NULL
* @return NULL
*/
void CameraCalibration::on_calparamsBtn_clicked()
{
	ui.calculateDisplay->clear();
	calculateCameraParameters(checkerboardPatterns);
}

/*!
* @brief 内角点数量输入槽
* @param LineEdit输入
* @return NULL
*/
void CameraCalibration::on_cornersOfColEdit_textChanged(const QString & num)
{
	this->cornerOfCol = num.toInt();
}

/*!
* @brief 内角点数量输入槽
* @param LineEdit输入
* @return NULL
*/
void CameraCalibration::on_cornersOfRowEdit_textChanged(const QString & num)
{
	this->cornerOfRow = num.toInt();
}

/*!
* @brief 是否沿x轴翻转
* @param CheckBox状态
* @return NULL
*/
void  CameraCalibration::on_xFlipCheck_stateChanged(int state)
{
	switch (state)
	{
	case Qt::Checked:
		this->needXFlip = true; break;
	case Qt::Unchecked:
		this->needXFlip = false; break;
	default:
		break;
	}
}

/*!
* @brief 是否沿y轴翻转
* @param CheckBox状态
* @return NULL
*/
void  CameraCalibration::on_yFlipCheck_stateChanged(int state)
{
	switch (state)
	{
	case Qt::Checked:
		this->needYFlip = true; break;
	case Qt::Unchecked:
		this->needYFlip = false; break;
	default:
		break;
	}
}

/*!
* @brief 是否四周裁剪图片选择槽
* @param CheckBox状态
* @return NULL
*/
void CameraCalibration::on_cutImgCheck_stateChanged(int state)
{
	switch (state)
	{
	case Qt::Checked:
		this->needCut = true; break;
	case Qt::Unchecked:
		this->needCut = false; break;
	default:
		break;
	}
}

/*!
* @brief 矫正缓冲区下一张图片并显示
* @param NULL
* @return NULL
*/
void CameraCalibration::on_nextCalibImgBtn_clicked()
{
	ui.CalibImgResult->clear();
	ui.CalibImgViewer->clear();
	ui.calculateDisplay->clear();

	if (checkerboardPatterns.size() < 1)
		ui.calculateDisplay->setText("缓冲区无图片，请载入");
	else
	{
		if (CurrentImgNum == checkerboardPatterns.size())CurrentImgNum = 0;
		Mat imgShow = imageUndist(checkerboardPatterns[CurrentImgNum]);
		showImage(checkerboardPatterns[CurrentImgNum++], ui.CalibImgViewer);
		showImage(imgShow, ui.CalibImgResult);
	}

}

/*!
* @brief 矫正缓冲区上一张图片并显示
* @param NULL
* @return NULL
*/
void CameraCalibration::on_lastCalibImgBtn_clicked()
{
	ui.CalibImgResult->clear();
	ui.CalibImgViewer->clear();
	ui.calculateDisplay->clear();

	if (checkerboardPatterns.size() < 1)
		ui.calculateDisplay->setText("缓冲区无图片，请载入");
	else
	{
		if (CurrentImgNum < 0)CurrentImgNum = checkerboardPatterns.size() - 1;
		Mat imgShow = imageUndist(checkerboardPatterns[CurrentImgNum]);
		showImage(checkerboardPatterns[CurrentImgNum--], ui.CalibImgViewer);
		showImage(imgShow, ui.CalibImgResult);
	}
}

/*!
* @brief 应用矫正参数按钮槽(只做预览，不会保存）
* @param NULL
* @return NULL
*/
void CameraCalibration::on_calibParamSetBtn_clicked()
{
	if (!camereaMatrixTemp.at<double>(0, 0))  //缓存中没有记录
	{
		ui.currentParamShow->setText("缓冲区无数据\n请先计算矫正参数!\n");
	}
	else
	{
		this->C_FX = camereaMatrixTemp.at<double>(0, 0);
		this->C_FY = camereaMatrixTemp.at<double>(1, 1);
		this->C_CX = camereaMatrixTemp.at<double>(0, 2);
		this->C_CY = camereaMatrixTemp.at<double>(1, 2);
		this->C_K1 = distCoffeffsTemp.at<double>(0, 0);
		this->C_K2 = distCoffeffsTemp.at<double>(0, 1);
		ui.currentParamShow->setText("当前畸变参数:\n CX = " + QString::number(C_CX) + "\n CY = " + QString::number(C_CY) + "\n FX = " + QString::number(C_FX) + "\n FY = " + QString::number(C_FY) + "\n K1 = " + QString::number(C_K1) + "\n K2 = " + QString::number(C_K2));

	}
}

/*!
* @brief Qt显示图像
* @param 伪彩色图像
* @param 显示载体
*/
void CameraCalibration::showImage(cv::Mat imshowsrc, QLabel* label)
{
	cv::Mat imgShow = imshowsrc.clone();
	cv::cvtColor(imgShow, imgShow, CV_BGR2RGB); //Opencv默认BGR存储，Qt需要RGB
	QImage img = QImage((uchar*)(imgShow.data), imgShow.cols, imgShow.rows, QImage::Format_RGB888);

	int width = label->size().width();
	int height = label->size().height();

	
	img = img.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation); //图像缩放
	label->setAlignment(Qt::AlignCenter); //居中显示
	label->setPixmap(QPixmap::fromImage(img)); //更新图像
}

//这个函数莫名其妙的多余...
void CameraCalibration::textInsert(uchar num)
{
	ui.calculateDisplay->insertPlainText(QString::number(num));
}

/*!
* @brief 计算相机内参
* @param 原始图(经过翻转or裁剪）
*/
void CameraCalibration::calculateCameraParameters(vector<cv::Mat> srcImgs)
{
	int imgCount = 0; //图像计数
	cv::Size img_size;//图像尺寸
	cv::Size board_size = cv::Size(cornerOfCol, cornerOfRow);//行列上的内角点数量
	vector<cv::Point2f> per_img_point;//每张图检测的角点数量
	vector<vector<cv::Point2f> > img_point;
	cv::Mat img_gray;

	ui.calculateDisplay->insertPlainText("寻找角点...");
	for (int i = 0; i < srcImgs.size(); i++)
	{
		imgCount++;
		if (needXFlip) flip(srcImgs[i], srcImgs[i], 0); //是否需要翻转图像
		if (needYFlip) flip(srcImgs[i], srcImgs[i], 1);
		cv::Mat imgShow = srcImgs[i].clone();
		//cv::cvtColor(srcImgs[i], img_gray, CV_RGB2GRAY);//转换为灰度图来通过亚像素精确化寻找角点坐标
		//获取图像尺寸
		if (imgCount == 1)
		{
			img_size.width = srcImgs[i].cols;
			img_size.height = srcImgs[i].rows;
		}
		if (cv::findChessboardCorners(srcImgs[i], board_size, per_img_point) == 0)
		{
			//TODO:: display notice
			continue;
		}
		else
		{
			// Size(5, 5)角点窗口的尺寸
			cv::cvtColor(srcImgs[i], img_gray, CV_RGB2GRAY);//转换为灰度图来通过亚像素精确化寻找角点坐标
			cv::find4QuadCornerSubpix(img_gray, per_img_point, cv::Size(5, 5));
			img_point.push_back(per_img_point);
			//在图像显示角点位置
			drawChessboardCorners(imgShow, board_size, per_img_point, true);
			showImage(imgShow, ui.CalibImgViewer);
			ui.CalibImgViewer->repaint();//强制更新

		}
	}
	QApplication::processEvents();//主线程中存在耗时计算，界面会卡死，调用此功能，维持界面响应，但最好将耗时的东西移出ui线程

	ui.calculateDisplay->insertPlainText("\n图片数量：");
	ui.calculateDisplay->insertPlainText(QString::number(imgCount));
	QApplication::processEvents();

	int corner_number = img_point.size();//所有的角点数
	int per_corner_number = board_size.width * board_size.height;//每张图的角点数

	ui.calculateDisplay->insertPlainText("\n开始计算...");
	QApplication::processEvents();


	cv::Size square_size = cv::Size(10, 10);//每个棋盘格大小
	vector<vector<cv::Point3f> > object_points; //世界坐标系中的三维坐标

	cv::Mat camereaMatrix = cv::Mat(3, 3, CV_32FC1, cv::Scalar::all(0));//摄像机的内参矩阵
	vector<int> point_count; //每幅图中角点数量
	cv::Mat distCoffeffs = cv::Mat(1, 5, CV_32FC1, cv::Scalar::all(0)); //摄像机的畸变系数

	vector<cv::Mat> rotaMatrix; //旋转向量，最后需要转换为旋转矩阵
	vector<cv::Mat> transMatrix; //平移向量

		// 初始化标定板上角点的世界坐标
	for (int num = 0; num < imgCount; num++)
	{
		vector<cv::Point3f> temp;
		for (int i = 0; i < board_size.height; i++)
		{
			for (int j = 0; j < board_size.width; j++)
			{
				cv::Point3f realPoints;
				// 假设世界坐标系中z=0坐标很奇怪
				realPoints.x = i * square_size.width;
				realPoints.y = j * square_size.height;
				realPoints.z = 0;
				temp.push_back(realPoints);
			}
		}
		object_points.push_back(temp);
	}

	//标定图像中角点数量理论是这么多
	for (int i = 0; i < imgCount; i++)
	{
		point_count.push_back(board_size.width*board_size.height);
	}

	//开始标定
	calibrateCamera(object_points, img_point, img_size, camereaMatrix, distCoffeffs, rotaMatrix, transMatrix, 0);
	camereaMatrixTemp = camereaMatrix.clone();//更新临时变量
	distCoffeffsTemp = distCoffeffs.clone();
	ui.calculateDisplay->insertPlainText("\n计算完成。");
	ui.calculateDisplay->insertPlainText("\n相机内参矩阵：");

	ui.calculateDisplay->insertPlainText("\nFx= ");
	ui.calculateDisplay->insertPlainText(QString::number(camereaMatrix.at<double>(0, 0), 'f', 16));
	ui.calculateDisplay->insertPlainText("\nFy= ");
	ui.calculateDisplay->insertPlainText(QString::number(camereaMatrix.at<double>(1, 1), 'f', 16));
	ui.calculateDisplay->insertPlainText("\nCx= ");
	ui.calculateDisplay->insertPlainText(QString::number(camereaMatrix.at<double>(0, 2), 'f', 16));
	ui.calculateDisplay->insertPlainText("\nCy= ");
	ui.calculateDisplay->insertPlainText(QString::number(camereaMatrix.at<double>(1, 2), 'f', 16));

	ui.calculateDisplay->insertPlainText("\n畸变系数：");
	ui.calculateDisplay->insertPlainText("\nk1= ");
	ui.calculateDisplay->insertPlainText(QString::number(distCoffeffs.at<double>(0, 0), 'f', 16));
	ui.calculateDisplay->insertPlainText("\nk2= ");
	ui.calculateDisplay->insertPlainText(QString::number(distCoffeffs.at<double>(0, 1), 'f', 16));


}

/*!
* @brief 畸变矫正
* @param 未经矫正的原始图
* @return 校正后的图片
*/
cv::Mat CameraCalibration::imageUndist(cv::Mat src)
{
	//畸变矫正
	Mat img;

	//内参矩阵
	Mat cameraMatrix = Mat::eye(3, 3, CV_64F); 		//3*3单位矩阵
	cameraMatrix.at<double>(0, 0) = C_FX;
	cameraMatrix.at<double>(0, 1) = 0;
	cameraMatrix.at<double>(0, 2) = C_CX;
	cameraMatrix.at<double>(1, 1) = C_FY;
	cameraMatrix.at<double>(1, 2) = C_CY;
	cameraMatrix.at<double>(2, 2) = 1;

	//畸变参数
	Mat distCoeffs = Mat::zeros(5, 1, CV_64F); 		//5*1全0矩阵
	distCoeffs.at<double>(0, 0) = C_K1;
	distCoeffs.at<double>(1, 0) = C_K2;
	distCoeffs.at<double>(2, 0) = 0;
	distCoeffs.at<double>(3, 0) = 0;
	distCoeffs.at<double>(4, 0) = 0;

	Size imageSize = src.size();
	Mat map1, map2;
	//参数1：相机内参矩阵
	//参数2：畸变矩阵
	//参数3：可选输入，第一和第二相机坐标之间的旋转矩阵
	//参数4：校正后的3X3相机矩阵
	//参数5：无失真图像尺寸
	//参数6：map1数据类型，CV_32FC1或CV_16SC2
	//参数7、8：输出X/Y坐标重映射参数
	initUndistortRectifyMap(cameraMatrix, distCoeffs, Mat(), cameraMatrix, imageSize, CV_32FC1, map1, map2); 	//计算畸变映射
	//参数1：畸变原始图像
	//参数2：输出图像
	//参数3、4：X\Y坐标重映射
	//参数5：图像的插值方式
	//参数6：边界填充方式
	remap(src, img, map1, map2, cv::INTER_NEAREST); 																	//畸变矫正
	return img;
}

/*!
* @brief 裁剪图片
* @param 原始图
* @return 裁剪后的图片
*/
cv::Mat CameraCalibration::regular_img(cv::Mat image)
{
	Mat source;
	cvtColor(image, source, CV_RGBA2RGB);
	Mat dst = Mat::zeros(240, 320, source.type());

	for (int i = 0; i < 320; i++)
	{
		for (int j = 0; j < 240; j++)
		{
			dst.at<Vec3b>(j, i) = source.at<Vec3b>(j + 6, i + 4);
		}
	}
	return dst;
}
