#ifndef CCSliceExtrationDLG_H
#define CCSliceExtrationDLG_H

#include "ui_CCSliceExtrationDlg.h"
#include <QDialog>

//�������ı���
#pragma execution_character_set("utf-8")

namespace Ui
{
	class ccSliceExtrationDlg;
}


//ccSliceExtrationDlgClassΪui��name
class ccSliceExtrationDlg : public QDialog, public Ui::SliceExtrationDialog
{
	Q_OBJECT

public:
	explicit ccSliceExtrationDlg(QWidget* parent = 0);

private slots:
	void modelChanged(int state);
};

#endif // CCSliceExtrationDLG_H
