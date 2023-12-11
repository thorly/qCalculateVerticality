#include "ccSliceExtrationDlg.h"


ccSliceExtrationDlg::ccSliceExtrationDlg(QWidget* parent) : QDialog(parent), Ui::SliceExtrationDialog()
{
	setupUi(this);
	connect(checkBoxModel, &QCheckBox::stateChanged, this, &ccSliceExtrationDlg::modelChanged);
}


void ccSliceExtrationDlg::modelChanged(int state)
{
	if (state == Qt::CheckState::Checked)
	{
		labelModel->setText("��Ƭ������");
	}
	else
	{
		labelModel->setText("��Ƭ�����");
	}
}
