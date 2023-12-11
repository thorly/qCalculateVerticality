#include "ccCalculateVerticalityDlg.h"

ccCalculateVerticalityDlg::ccCalculateVerticalityDlg(QWidget* parent) : QDialog(parent), Ui::CalculateVerticalityDialog()
{
	setupUi(this);

	//��LineEdit����Ϊ���ɱ༭״̬
	lineEditReportPath->setEnabled(false);

	//�����ź���ۺ���
	QObject::connect(pushButtonSetReportPath, SIGNAL(clicked()), this, SLOT(setReportPath()));
}


void ccCalculateVerticalityDlg::setReportPath()
{
	QString reportPath = QFileDialog::getSaveFileName(
		this,
		"���÷����Ͳ��ֱ�ȱ���·��",
		"",
		"Excel(.xlsx);; *.xls;; *.xlsx"
	);
	if (reportPath.isEmpty())
		return;

	lineEditReportPath->setText(reportPath);
}

