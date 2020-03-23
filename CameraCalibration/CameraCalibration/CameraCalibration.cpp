#include "CameraCalibration.h"
using namespace std;
#pragma execution_character_set("utf-8")
CameraCalibration::CameraCalibration(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	cornerOfRow = ui.cornersOfRowEdit->text().toInt();
	cornerOfCol = ui.cornersOfColEdit->text().toInt();;

	//部分槽连接
	

}

void CameraCalibration::on_loadImgBtn_clicked()
{
	qDebug() << "load images..." << endl;
	checkerboardPatterns.clear();
	const QStringList imgPaths = QFileDialog::getOpenFileNames(this, tr("选取图片"), "", tr("图片格式 (*.png *.jpg);;All(*.*);;"));
	if (!imgPaths.isEmpty())
	{
		cv::Mat img;
		foreach(const QFileInfo &imgPath, imgPaths) {
			QString imageFile = imgPath.absoluteFilePath();
			img = cv::imread(imageFile.toStdString());
			showImage(img, ui.imgViewer);
			ui.imgViewer->repaint();  //强制更新
			checkerboardPatterns.push_back(img);//保存图片
		}
		emit loadAllImages(checkerboardPatterns);//信号：载入所有图片
	}
}
void CameraCalibration::on_calparamsBtn_clicked()
{
	ui.calculateDisplay->clear();
	calculateCameraParameters(checkerboardPatterns);
}

void CameraCalibration::on_cornersOfColEdit_textChanged(const QString & num)
{
	this->cornerOfCol = num.toInt();
}
void CameraCalibration::on_cornersOfRowEdit_textChanged(const QString & num)
{
	this->cornerOfRow = num.toInt();
}
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

//QT显示图像
//输入：imshowsrc 伪彩色图像
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

void CameraCalibration::textInsert(uchar num)
{
	ui.calculateDisplay->insertPlainText(QString::number(num));
}

//计算相机内外参
void CameraCalibration::calculateCameraParameters(vector<cv::Mat> srcImgs)
{
	int imgCount = 0; //图像计数
	cv::Size img_size;//图像尺寸
	cv::Size board_size = cv::Size(cornerOfCol, cornerOfRow);//行列上的内角点数量
	vector<cv::Point2f> per_img_point;//每张图检测的角点数量
	vector<vector<cv::Point2f> > img_point;
	ui.calculateDisplay->insertPlainText("寻找角点...");
	for (int i = 0; i < srcImgs.size(); i++)
	{
		imgCount++;
		if (needXFlip)flip(srcImgs[i], srcImgs[i], 0); //是否需要翻转图像
		if (needYFlip)flip(srcImgs[i], srcImgs[i], 1);
		cv::Mat imgShow = srcImgs[i].clone();
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
			cv::Mat img_gray;
			cv::cvtColor(srcImgs[i], img_gray, CV_RGB2GRAY);//转换为灰度图来通过亚像素精确化寻找角点坐标
			// Size(5, 5)角点窗口的尺寸
			cv::find4QuadCornerSubpix(img_gray, per_img_point, cv::Size(5, 5));
			img_point.push_back(per_img_point);
			//在图像显示角点位置
			drawChessboardCorners(imgShow, board_size, per_img_point, true);
			showImage(imgShow, ui.imgViewer);
			ui.imgViewer->repaint();//强制更新

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
	ui.calculateDisplay->insertPlainText("\n计算完成。");
	ui.calculateDisplay->insertPlainText("\n相机内参矩阵：\n");
	//camereaMatrix.forEach<double>
	//(
	//	[this](double &pixel, const int * position) -> void
	//	{
	//		camInternalMatrix.append(QString::number(pixel));
	//		camInternalMatrix.append(", ");

	//		ui.calculateDisplay->insertPlainText(QString::number(pixel));
	//	}
	//);

	//std::cout << camereaMatrix<<endl; 
	for (int i = 0; i < camereaMatrix.rows; i++)
	{
		
		for (int j = 0; j < camereaMatrix.cols; j++)
		{
			ui.calculateDisplay->insertPlainText(QString::number(camereaMatrix.at<double>(i, j), 'f', 16));
			ui.calculateDisplay->insertPlainText(", ");
		}
		ui.calculateDisplay->insertPlainText("\n");
	}
	ui.calculateDisplay->insertPlainText("\n畸变系数：\n");

	for (int i = 0; i < distCoffeffs.rows; i++)
	{
		for (int j = 0; j < distCoffeffs.cols; j++)
		{
			ui.calculateDisplay->insertPlainText(QString::number(distCoffeffs.at<double>(i, j), 'f', 16));
			ui.calculateDisplay->insertPlainText("\n");
		}
	}



	


}