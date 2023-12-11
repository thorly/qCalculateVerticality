//##########################################################################
//#                                                                        #
//#                CLOUDCOMPARE PLUGIN: CalculateVerticality               #
//#                                                                        #
//#  This program is free software; you can redistribute it and/or modify  #
//#  it under the terms of the GNU General Public License as published by  #
//#  the Free Software Foundation; version 2 of the License.               #
//#                                                                        #
//#  This program is distributed in the hope that it will be useful,       #
//#  but WITHOUT ANY WARRANTY; without even the implied warranty of        #
//#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
//#  GNU General Public License for more details.                          #
//#                                                                        #
//#                             COPYRIGHT: XXX                             #
//#                                                                        #
//##########################################################################

// First:
//	Replace all occurrences of 'qCalculateVerticality' by your own plugin class name in this file.
//	This includes the resource path to info.json in the constructor.

// Second:
//	Open qCalculateVerticality.qrc, change the "prefix" and the icon filename for your plugin.
//	Change the name of the file to <yourPluginName>.qrc

// Third:
//	Open the info.json file and fill in the information about the plugin.
//	 "type" should be one of: "Standard", "GL", or "I/O" (required)
//	 "name" is the name of the plugin (required)
//	 "icon" is the Qt resource path to the plugin's icon (from the .qrc file)
//	 "description" is used as a tootip if the plugin has actions and is displayed in the plugin dialog
//	 "authors", "maintainers", and "references" show up in the plugin dialog as well

#include "qCalculateVerticality.h"
#include "qcustomplot.h"

//Qt
#include <QtGui>
#include <QMainWindow>

//CC
#include "ccHObject.h"
#include "ccPolyline.h"
#include "ccProgressDialog.h"
#include "GeometricalAnalysisTools.h"
#include "ccPointCloud.h"

//PCL
#include <pcl/filters/passthrough.h>// 直通滤波
#include <pcl/common/common.h>

//Dialog
#include "ccCalculateVerticalityDlg.h"
#include "ccSliceExtrationDlg.h"

//system
#include <vector>
#include <algorithm>

//QXlsx
#include "xlsxcellrange.h"
#include "xlsxchart.h"
#include "xlsxchartsheet.h"
#include "xlsxdocument.h"
#include "xlsxrichstring.h"
#include "xlsxworkbook.h"

using namespace QXlsx;
using namespace std;

typedef pcl::PointCloud<pcl::PointXYZ> PCLPointCloudT;


// Default constructor:
//	- pass the Qt resource path to the info.json file (from <yourPluginName>.qrc file) 
//  - constructor should mainly be used to initialize actions and other members
qCalculateVerticality::qCalculateVerticality(QObject* parent)
	: QObject(parent)
	, ccStdPluginInterface(":/CC/plugin/qCalculateVerticality/info.json")
	, actionCalculateVerticality(nullptr)
	, actionSliceExtration(nullptr)
	, actionFitCircle(nullptr)
{
}

// This method should enable or disable your plugin actions
// depending on the currently selected entities ('selectedEntities').
void qCalculateVerticality::onNewSelection(const ccHObject::Container& selectedEntities)
{
	if (actionCalculateVerticality == nullptr)
	{
		return;
	}

	if (actionSliceExtration == nullptr)
	{
		return;
	}

	if (actionFitCircle == nullptr)
	{
		return;
	}

	// If you need to check for a specific type of object, you can use the methods
	// in ccHObjectCaster.h or loop and check the objects' classIDs like this:
	//
	//	for ( ccHObject *object : selectedEntities )
	//	{
	//		if ( object->getClassID() == CC_TYPES::VIEWPORT_2D_OBJECT )
	//		{
	//			// ... do something with the viewports
	//		}
	//	}

	// For example - only enable our action if something is selected.
	actionCalculateVerticality->setEnabled(!selectedEntities.empty());
	actionSliceExtration->setEnabled(!selectedEntities.empty());
	actionFitCircle->setEnabled(!selectedEntities.empty());
}

// This method returns all the 'actions' your plugin can perform.
// getActions() will be called only once, when plugin is loaded.
QList<QAction*> qCalculateVerticality::getActions()
{
	// default action (if it has not been already created, this is the moment to do it)
	if (!actionCalculateVerticality)
	{
		// Here we use the default plugin name, description, and icon,
		// but each action should have its own.
		actionCalculateVerticality = new QAction("计算风机塔筒垂直度", this);
		actionCalculateVerticality->setToolTip("计算风机塔筒垂直度");
		actionCalculateVerticality->setIcon(QIcon(":/CC/plugin/qCalculateVerticality/images/icon_verticality.png"));

		// Connect appropriate signal
		connect(actionCalculateVerticality, SIGNAL(triggered()), this, SLOT(doActionCalculateVerticality()));
	}

	if (!actionSliceExtration)
	{
		// Here we use the default plugin name, description, and icon,
		// but each action should have its own.
		actionSliceExtration = new QAction("提取切片", this);
		actionSliceExtration->setToolTip("提取切片");
		actionSliceExtration->setIcon(QIcon(":/CC/plugin/qCalculateVerticality/images/icon_slice.png"));

		// Connect appropriate signal
		connect(actionSliceExtration, SIGNAL(triggered()), this, SLOT(doActionSliceExtration()));
	}

	if (!actionFitCircle)
	{
		// Here we use the default plugin name, description, and icon,
		// but each action should have its own.
		actionFitCircle = new QAction("拟合圆", this);
		actionFitCircle->setToolTip("拟合圆");
		actionFitCircle->setIcon(QIcon(":/CC/plugin/qCalculateVerticality/images/icon_circle.png"));

		// Connect appropriate signal
		connect(actionFitCircle, SIGNAL(triggered()), this, SLOT(doActionFitCircle()));
	}

	return { actionFitCircle, actionSliceExtration, actionCalculateVerticality };
}

//排序
bool cmp(CCVector3 c1, CCVector3 c2)
{
	if (c1.z != c2.z)
	{
		return c1.z < c2.z;
	}
	else
	{
		return true;
	}
}


void qCalculateVerticality::doActionCalculateVerticality()
{
	//m_app should have already been initialized by CC when plugin is loaded!
	//(--> pure internal check)
	assert(m_app);
	if (!m_app)
		return;

	//显示窗口
	QString reportPath = "";
	{
		ccCalculateVerticalityDlg calculateVerticalityDlg(m_app->getMainWindow());

		if (!calculateVerticalityDlg.exec())
		{
			return;
		}

		//获取report路径
		reportPath = calculateVerticalityDlg.lineEditReportPath->text();
		//reportPath = "D:\\Coding\\CloudCompare\\test.xlsx";
		if (reportPath == "")
		{
			m_app->dispToConsole("[计算风机塔筒垂直度] 请设置垂直度报告路径!", ccMainAppInterface::ERR_CONSOLE_MESSAGE);
			return;
		}
	}


	m_app->dispToConsole("[计算风机塔筒垂直度]", ccMainAppInterface::STD_CONSOLE_MESSAGE);

	//获取选择的实体
	const ccHObject::Container& selectedEntities = m_app->getSelectedEntities();

	//切片个数必须大于等于2
	if (selectedEntities.size() < 2)
	{
		m_app->dispToConsole("[计算风机塔筒垂直度] 请至少选择两个点云切片!", ccMainAppInterface::ERR_CONSOLE_MESSAGE);
		return;
	}

	//添加一组新的DB实体
	ccHObject* cloudContainer = new ccHObject("拟合圆");

	//拟合圆
	std::vector<CCVector3> circleXYZ;
	CCVector3d globalShift;

	for (ccHObject* entity : selectedEntities)
	{
		ccPointCloud* cloud = ccHObjectCaster::ToPointCloud(entity);
		if (!cloud)
			continue;

		CCVector3 center;
		CCVector3 normal;
		PointCoordinateType radius = 0;
		double rms = std::numeric_limits<double>::quiet_NaN();
		if (CCCoreLib::GeometricalAnalysisTools::DetectCircle(cloud,
			center,
			normal,
			radius,
			rms,
			nullptr) != CCCoreLib::GeometricalAnalysisTools::NoError)
		{
			ccLog::Warning(QObject::tr("[计算风机塔筒垂直度] 点云'%1'拟合失败").arg(cloud->getName()));
			continue;
		}

		//BoundingBox
		CCVector3 bbMin, bbMax;
		cloud->getBoundingBox(bbMin, bbMax);
		center.z = bbMin.z;

		// global shift
		globalShift = cloud->getGlobalShift();

		double x = center.x - globalShift.x;
		double y = center.y - globalShift.y;
		double z = center.z - globalShift.z;


		ccLog::Print(QObject::tr("[拟合圆] 点云 '%1': 圆心 (%2, %3, %4) - 半径 = %5 [RMS = %6]")
			.arg(cloud->getName())
			.arg(x, 0, 'r')
			.arg(y, 0, 'r')
			.arg(z, 0, 'r')
			.arg(radius)
			.arg(rms));

		// create the circle representation as a polyline
		ccPolyline* circle = ccPolyline::Circle(CCVector3(0, 0, 0), radius, 128);
		if (circle)
		{
			circle->setName(QObject::tr("%1 Circle r=%2").arg(cloud->getName()).arg(radius));
			cloudContainer->addChild(circle);
			circle->prepareDisplayForRefresh();
			circle->copyGlobalShiftAndScale(*cloud);
			circle->setMetaData("RMS", rms);

			ccGLMatrix trans = ccGLMatrix::FromToRotation(CCVector3(0, 0, 1), normal);
			trans.setTranslation(center);
			circle->applyGLTransformation_recursive(&trans);
		}

		circleXYZ.push_back(center);
	}

	m_app->addToDB(cloudContainer);

	//排序
	sort(circleXYZ.begin(), circleXYZ.end(), cmp);

	//计算垂直度
	/*
	vector<vector<double>> verticalityInfo;

	for (int i = 0; i < circleXYZ.size(); i++)
	{
		double dx = circleXYZ[i].x - circleXYZ[0].x;
		double dy = circleXYZ[i].y - circleXYZ[0].y;
		double dxy = sqrt(dx * dx + dy * dy);
		double height = circleXYZ[i].z - circleXYZ[0].z;
		double verticality;

		if (i == 0)
		{
			verticality = 0;
		}
		else
		{
			verticality = dxy / height;
		}

		vector<double> lineInfo;
		lineInfo.push_back(i + 1); //切片号
		lineInfo.push_back(circleXYZ[i].x); //东坐标
		lineInfo.push_back(circleXYZ[i].y); //北坐标
		lineInfo.push_back(circleXYZ[i].z); //高程
		lineInfo.push_back(height); //测量位置高度
		lineInfo.push_back(dx); //东偏移量
		lineInfo.push_back(dy); //北偏移量
		lineInfo.push_back(dxy); //累计偏移量
		lineInfo.push_back(verticality * 1000); //倾斜率

		verticalityInfo.push_back(lineInfo);
	}
	*/


	//输出
	Document xlsx;

	xlsx.setRowHeight(1, 60);    //设置第一行行高为60
	xlsx.setRowHeight(2, 20);
	xlsx.setRowHeight(3, 20);
	xlsx.setRowHeight(4, 20);

	Format defaultFormat;
	defaultFormat.setHorizontalAlignment(Format::AlignHCenter);  // 水平居中
	defaultFormat.setVerticalAlignment(Format::AlignVCenter);    // 垂直居中
	defaultFormat.setBorderStyle(Format::BorderThin); //边框

	Format titleFormat;
	titleFormat.setHorizontalAlignment(Format::AlignHCenter);
	titleFormat.setVerticalAlignment(Format::AlignVCenter);
	titleFormat.setBorderStyle(Format::BorderThin);
	titleFormat.setFontSize(16);

	xlsx.write("A1", "垂直度计算表");
	xlsx.mergeCells("A1:J1", titleFormat);

	xlsx.write("A2", "测量日期：");
	xlsx.mergeCells("A2:D2", defaultFormat);

	xlsx.write("E2", "温度：℃");
	xlsx.mergeCells("E2:F2", defaultFormat);

	xlsx.write("G2", "风力：级");
	xlsx.mergeCells("G2:H2", defaultFormat);

	xlsx.write("I2", "天气：");
	xlsx.mergeCells("I2:J2", defaultFormat);

	xlsx.write("A3", "风机号");
	xlsx.mergeCells("A3:A4", defaultFormat);

	xlsx.write("B3", "切片号");
	xlsx.mergeCells("B3:B4", defaultFormat);

	xlsx.write("C3", "中心轴线坐标（米）");
	xlsx.mergeCells("C3:E3", defaultFormat);

	xlsx.write("C4", "东", defaultFormat);
	xlsx.write("D4", "北", defaultFormat);
	xlsx.write("E4", "高程", defaultFormat);

	xlsx.write("F3", "测量位置高度（米）");
	xlsx.mergeCells("F3:F4", defaultFormat);

	xlsx.write("G3", "偏移量（米）");
	xlsx.mergeCells("G3:I3", defaultFormat);

	xlsx.write("G4", "偏东", defaultFormat);
	xlsx.write("H4", "偏北", defaultFormat);
	xlsx.write("I4", "累计", defaultFormat);

	xlsx.write("J3", "倾斜率");
	xlsx.mergeCells("J3:J4", defaultFormat);

	//输出计算结果
	int num = circleXYZ.size();

	xlsx.write("A5", "号");
	xlsx.mergeCells(CellRange(5, 1, 4 + num, 1), defaultFormat);

	QVector<double> h(num), v(num);

	for (int i = 1; i <= num; i++)
	{
		double dx = circleXYZ[i - 1].x - circleXYZ[0].x;
		double dy = circleXYZ[i - 1].y - circleXYZ[0].y;
		double dxy = sqrt(dx * dx + dy * dy);
		double height = circleXYZ[i - 1].z - circleXYZ[0].z;

		h[i - 1] = height;
		v[i - 1] = dxy;

		xlsx.setRowHeight(i + 4, 20);

		xlsx.write(i + 4, 2, i, defaultFormat); //切片号
		xlsx.write(i + 4, 3, circleXYZ[i - 1].x - globalShift.x, defaultFormat); //东坐标
		xlsx.write(i + 4, 4, circleXYZ[i - 1].y - globalShift.y, defaultFormat); //北坐标
		xlsx.write(i + 4, 5, circleXYZ[i - 1].z - globalShift.z, defaultFormat); //高程

		//QVariant value1 = QString("=E%1-E5").arg(i + 4);
		QVariant value1 = height;
		xlsx.write(i + 4, 6, value1, defaultFormat);     //测量位置高度

		//QVariant value2 = QString("=C%1-C5").arg(i + 4);
		QVariant value2 = dx;
		xlsx.write(i + 4, 7, value2, defaultFormat);     //偏东

		//QVariant value3 = QString("=D%1-D5").arg(i + 4);
		QVariant value3 = dy;
		xlsx.write(i + 4, 8, value3, defaultFormat);     //偏北

		//QVariant value4 = QString("=SQRT(G%1^2+H%2^2)").arg(i + 4).arg(i + 4);
		QVariant value4 = dxy;
		xlsx.write(i + 4, 9, value4, defaultFormat);     //累计

		//倾斜率
		if (i == 1)
		{
			xlsx.write(i + 4, 10, 0, defaultFormat);
		}
		else
		{
			//QVariant value2 = QString("=I%1/F%2").arg(i + 4).arg(i + 4);
			QVariant value2 = dxy / height;
			xlsx.write(i + 4, 10, value2, defaultFormat);
		}
	}

	xlsx.setRowHeight(num + 5, 20);

	xlsx.write(num + 5, 1, "风机扫描点云示意图");
	xlsx.mergeCells(CellRange(num + 5, 1, num + 5, 5), defaultFormat);

	xlsx.write(num + 5, 6, "垂直度变化曲线");
	xlsx.mergeCells(CellRange(num + 5, 6, num + 5, 10), defaultFormat);


	xlsx.setRowHeight(num + 6, 340);

	xlsx.mergeCells(CellRange(num + 6, 1, num + 6, 5), defaultFormat);
	xlsx.mergeCells(CellRange(num + 6, 6, num + 6, 10), defaultFormat);


	xlsx.setRowHeight(num + 7, 20);

	xlsx.write(num + 7, 1, "计算：");
	xlsx.mergeCells(CellRange(num + 7, 1, num + 7, 5), defaultFormat);

	xlsx.write(num + 7, 6, "校核：");
	xlsx.mergeCells(CellRange(num + 7, 6, num + 7, 10), defaultFormat);

	//绘图
	QCustomPlot* pCustomPlot = new QCustomPlot(nullptr);
	pCustomPlot->resize(100, 300);

	// 向绘图区域QCustomPlot添加三条曲线
	for (int i = 0; i < 3; i++)
	{
		pCustomPlot->addGraph();
	}

	// 添加数据
	QPen penBlue;
	penBlue.setWidth(3);//设置线宽
	penBlue.setColor(Qt::blue);//设置线条蓝色
	pCustomPlot->graph(2)->setPen(penBlue);
	pCustomPlot->graph(2)->setData(v, h, true);
	pCustomPlot->graph(2)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, 5));

	
	QVector<double> xMin(num), xMax(num);
	for (int i = 0; i < num; i++)
	{
		xMin[i] = h[num - 1] * 0.002;
		xMax[i] = h[num - 1] * 0.004;
	}

	// 绘制绿线
	QPen penGreen;
	penGreen.setWidth(3);
	penGreen.setColor(Qt::green);
	pCustomPlot->graph(1)->setPen(penGreen);
	pCustomPlot->graph(1)->setData(xMin, h);

	// 绘制红线
	QPen penRed;
	penRed.setWidth(3);
	penRed.setColor(Qt::red);
	pCustomPlot->graph(0)->setPen(penRed);
	pCustomPlot->graph(0)->setData(xMax, h);


	// 设置坐标轴名称
	pCustomPlot->xAxis->setLabel("偏移量/米");
	pCustomPlot->yAxis->setLabel("高度/米");

	//设置刻度
	QSharedPointer<QCPAxisTickerFixed>xTicker(new QCPAxisTickerFixed);
	xTicker->setTickStep(0.1);
	pCustomPlot->xAxis->setTicker(xTicker);

	QSharedPointer<QCPAxisTickerFixed>yTicker(new QCPAxisTickerFixed);
	yTicker->setTickStep(10);
	pCustomPlot->yAxis->setTicker(yTicker);


	// 设置坐标轴显示范围，否则只能看到默认范围
	pCustomPlot->xAxis->setRange(0, 0.4);
	pCustomPlot->yAxis->setRange(0, h[num - 1]);

	//设置字体
	QFont font;
	font.setPixelSize(15);

	pCustomPlot->xAxis->setLabelFont(font);
	pCustomPlot->yAxis->setLabelFont(font);
	pCustomPlot->xAxis->setTickLabelFont(font);
	pCustomPlot->yAxis->setTickLabelFont(font);


	//保存图片
	QFileInfo fileInfo;
	fileInfo = QFileInfo(reportPath);
	QString fileDir = fileInfo.absolutePath();
	QString imageFile = fileDir + "\\verticality.png";
	pCustomPlot->savePng(imageFile, 230, 400, 1, 300);

	//插入图片
	/*QImage image;
	image.load(imageFile);
	xlsx.insertImage(num + 5, 6, image.scaled(125, 300, Qt::KeepAspectRatio, Qt::SmoothTransformation));*/

	xlsx.insertImage(num + 5, 6, QImage(imageFile));

	//删除图片
	QFileInfo fi(imageFile);
	if (fi.isFile())
	{
		QFile::remove(imageFile);
	}


	if (xlsx.saveAs(reportPath))    // 如果文件已经存在则覆盖
	{
		m_app->dispToConsole(QString("[计算风机塔筒垂直度] 成功保存文件至 %1!").arg(reportPath));
	}
	else
	{
		ccLog::Warning("[计算风机塔筒垂直度] 保存文件失败!", ccMainAppInterface::ERR_CONSOLE_MESSAGE);
	}
}


PCLPointCloudT sliceByPassThrough(PCLPointCloudT::Ptr inCloud, std::string field, double limitMin, double limitMax)
{
	PCLPointCloudT::Ptr cloudFiltered(new PCLPointCloudT);

	pcl::PassThrough<pcl::PointXYZ> ptfilter;    //新建一个直通滤波器
	ptfilter.setInputCloud(inCloud);                //设置输入点云
	ptfilter.setFilterFieldName(field);               //设置滤波方向
	ptfilter.setFilterLimits(limitMin, limitMax);     //设置最大最小值
	ptfilter.setNegative(false);                    //false表示保留内部点
	ptfilter.filter(*cloudFiltered);               //滤波并输出

	return *cloudFiltered;
}

void qCalculateVerticality::doActionSliceExtration()
{
	//m_app should have already been initialized by CC when plugin is loaded!
	//(--> pure internal check)
	assert(m_app);
	if (!m_app)
		return;

	//获取选择的实体
	const ccHObject::Container& selectedEntities = m_app->getSelectedEntities();
	if (selectedEntities.size() != 1)
	{
		m_app->dispToConsole("[提取切片] 请至少选择一个点云!", ccMainAppInterface::ERR_CONSOLE_MESSAGE);
		return;
	}
	ccHObject* entity = selectedEntities[0];

	assert(entity);
	if (!entity || !entity->isA(CC_TYPES::POINT_CLOUD))
	{
		m_app->dispToConsole("[提取切片] 该文件不是点云文件，请选择点云文件!", ccMainAppInterface::ERR_CONSOLE_MESSAGE);
		return;
	}

	//将选择的实体转换成ccPointCloud*类型
	ccPointCloud* ccCloud = ccHObjectCaster::ToPointCloud(entity);

	//CC点云转PCL点云
	PCLPointCloudT::Ptr pclCloud(new PCLPointCloudT);
	pclCloud->resize(ccCloud->size());
	for (int i = 0; i < pclCloud->size(); ++i)
	{
		const CCVector3* point = ccCloud->getPoint(i);
		pclCloud->points[i].x = point->x;
		pclCloud->points[i].y = point->y;
		pclCloud->points[i].z = point->z;
	}

	//点云极值
	pcl::PointXYZ pMin, pMax;
	pcl::getMinMax3D(*pclCloud, pMin, pMax);

	//显示窗口
	ccSliceExtrationDlg SliceExtrationDlg(m_app->getMainWindow());

	SliceExtrationDlg.lineEditMinHeight->setText(QString::fromStdString(std::to_string(pMin.z)));
	SliceExtrationDlg.lineEditMaxHeight->setText(QString::fromStdString(std::to_string(pMax.z)));
	SliceExtrationDlg.lineEditGapOrQuantity->setText(QString::fromStdString(std::to_string(10)));

	if (!SliceExtrationDlg.exec())
	{
		return;
	}

	CCVector3d globalShift = ccCloud->getGlobalShift();

	if (!SliceExtrationDlg.checkBoxModel->isChecked())
	{
		m_app->dispToConsole("[提取切片] 使用切片间隔提取切片");
		//方法一：根据厚度和间隔进行切片
		double thickness = SliceExtrationDlg.doubleSpinBoxThickness->value();
		double minHeight = SliceExtrationDlg.lineEditMinHeight->text().toDouble();
		double maxHeight = SliceExtrationDlg.lineEditMaxHeight->text().toDouble();
		double gap = SliceExtrationDlg.lineEditGapOrQuantity->text().toDouble();
		int index = 1;

		//添加一组新的DB实体
		ccHObject* cloudContainer = new ccHObject(ccCloud->getName() + QString("_slice_gap"));
		//隐藏原始点云
		ccCloud->setEnabled(false);

		for (double sliceStart = minHeight; sliceStart < maxHeight - thickness;)
		{
			double limitMin = sliceStart;
			double limitMax = sliceStart + thickness;

			//切片
			PCLPointCloudT::Ptr cloudFiltered(new PCLPointCloudT);
			*cloudFiltered = sliceByPassThrough(pclCloud, "z", limitMin, limitMax);

			if (!cloudFiltered->empty())
			{
				ccPointCloud* newCloud = new ccPointCloud();

				//PCL点云转CC点云
				for (int i = 0; i < cloudFiltered->size(); ++i)
				{
					double x = cloudFiltered->points[i].x;
					double y = cloudFiltered->points[i].y;
					double z = cloudFiltered->points[i].z;
					newCloud->addPoint(CCVector3(x, y, z));
				}
				
				//设置新点云并添加到实体
				newCloud->setGlobalShift(globalShift.x, globalShift.y, globalShift.z);
				newCloud->setVisible(true);
				newCloud->setName(QObject::tr("slice%1").arg(index));
				cloudContainer->addChild(newCloud);

				//添加到DB树
				m_app->addToDB(cloudContainer);

				//刷新
				m_app->refreshAll();

				index++;
			}
			sliceStart += gap;
		}
	}
	else
	{
		m_app->dispToConsole("[提取切片] 使用切片数量提取切片");
		//方法二：根据数量和厚度进行切片
		double thickness = SliceExtrationDlg.doubleSpinBoxThickness->value();
		double minHeight = SliceExtrationDlg.lineEditMinHeight->text().toDouble();
		double maxHeight = SliceExtrationDlg.lineEditMaxHeight->text().toDouble();
		double quantity = SliceExtrationDlg.lineEditGapOrQuantity->text().toDouble();

		int index = 1;

		//计算gap
		double gap = ((maxHeight - minHeight) - quantity * thickness) / (quantity - 1);

		//添加新的一组DB实体
		ccHObject* cloudContainer = new ccHObject(ccCloud->getName() + QString("_slice_quantity"));
		//隐藏原始点云
		ccCloud->setEnabled(false);

		for (double sliceStart = minHeight; sliceStart <= maxHeight - thickness;)
		{
			double limitMin = sliceStart;
			double limitMax = sliceStart + thickness;

			//切片
			PCLPointCloudT::Ptr cloudFiltered(new PCLPointCloudT);
			*cloudFiltered = sliceByPassThrough(pclCloud, "z", limitMin, limitMax);

			if (!cloudFiltered->empty())
			{
				ccPointCloud* newCloud = new ccPointCloud();

				//PCL点云转CC点云
				for (int i = 0; i < cloudFiltered->size(); ++i)
				{
					double x = cloudFiltered->points[i].x;
					double y = cloudFiltered->points[i].y;
					double z = cloudFiltered->points[i].z;
					newCloud->addPoint(CCVector3(x, y, z));
				}

				//设置新点云并添加到实体
				newCloud->setGlobalShift(globalShift.x, globalShift.y, globalShift.z);
				newCloud->setVisible(true);
				newCloud->setName(QObject::tr("slice%1").arg(index));
				cloudContainer->addChild(newCloud);

				//添加实体到DB树
				m_app->addToDB(cloudContainer);

				//刷新
				m_app->refreshAll();

				index++;
			}
			sliceStart += gap;
		}
	}
}


void qCalculateVerticality::doActionFitCircle()
{
	//m_app should have already been initialized by CC when plugin is loaded!
	//(--> pure internal check)

	const ccHObject::Container& selectedEntities = m_app->getSelectedEntities();
	//size_t num = selectedEntities.size();
	//m_app->dispToConsole(QString::fromStdString(std::to_string(num)), ccMainAppInterface::STD_CONSOLE_MESSAGE);

	m_app->dispToConsole("[拟合圆]", ccMainAppInterface::STD_CONSOLE_MESSAGE);

	for (ccHObject* entity : selectedEntities)
	{
		ccPointCloud* cloud = ccHObjectCaster::ToPointCloud(entity);
		if (!cloud)
			continue;

		CCVector3 center;
		CCVector3 normal;
		PointCoordinateType radius = 0;
		double rms = std::numeric_limits<double>::quiet_NaN();
		if (CCCoreLib::GeometricalAnalysisTools::DetectCircle(cloud,
			center,
			normal,
			radius,
			rms,
			nullptr) != CCCoreLib::GeometricalAnalysisTools::NoError)
		{
			ccLog::Warning(QObject::tr("[拟合圆] 点云'%1'拟合失败").arg(cloud->getName()));
			continue;
		}

		// global shift
		CCVector3d globalShift;
		globalShift = cloud->getGlobalShift();

		double x = center.x - globalShift.x;
		double y = center.y - globalShift.y;
		double z = center.z - globalShift.z;

		ccLog::Print(QObject::tr("[拟合圆] 点云 '%1': 圆心 (%2, %3, %4) - 半径 = %5 [RMS = %6]")
			.arg(cloud->getName())
			.arg(x, 0, 'r')
			.arg(y, 0, 'r')
			.arg(z, 0, 'r')
			.arg(radius)
			.arg(rms));

		// create the circle representation as a polyline
		ccPolyline* circle = ccPolyline::Circle(CCVector3(0, 0, 0), radius, 128);
		if (circle)
		{
			circle->setName(QObject::tr("Circle r=%1").arg(radius));
			cloud->addChild(circle);
			circle->prepareDisplayForRefresh();
			circle->copyGlobalShiftAndScale(*cloud);
			circle->setMetaData("RMS", rms);

			ccGLMatrix trans = ccGLMatrix::FromToRotation(CCVector3(0, 0, 1), normal);
			trans.setTranslation(center);
			circle->applyGLTransformation_recursive(&trans);

			m_app->addToDB(circle, false, false, false);
		}
	}

	m_app->refreshAll();
}
