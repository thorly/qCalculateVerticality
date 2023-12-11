#include "ccCalculateVerticalityDlg.h"

ccCalculateVerticalityDlg::ccCalculateVerticalityDlg(QWidget* parent) : QDialog(parent), Ui::CalculateVerticalityDialog()
{
	setupUi(this);

	//将LineEdit设置为不可编辑状态
	lineEditReportPath->setEnabled(false);

	//连接信号与槽函数
	QObject::connect(pushButtonSetReportPath, SIGNAL(clicked()), this, SLOT(setReportPath()));
}


void ccCalculateVerticalityDlg::setReportPath()
{
	QString reportPath = QFileDialog::getSaveFileName(
		this,
		"设置风机塔筒垂直度报告路径",
		"",
		"Excel(.xlsx);; *.xls;; *.xlsx"
	);
	if (reportPath.isEmpty())
		return;

	lineEditReportPath->setText(reportPath);
}

