#ifndef CCCalculateVerticalityDLG_H
#define CCCalculateVerticalityDLG_H

#include "ui_CCCalculateVerticalityDlg.h"
#include <QDialog>
#include <QFileDialog>

//…Ë÷√÷–Œƒ±‡¬Î
#pragma execution_character_set("utf-8")

namespace Ui
{
	class ccCalculateVerticalityDlg;
}


class ccCalculateVerticalityDlg : public QDialog, public Ui::CalculateVerticalityDialog
{
	Q_OBJECT

public:
	explicit ccCalculateVerticalityDlg(QWidget* parent = 0);


private slots:
	void setReportPath();
};

#endif // CCCalculateVerticalityDLG_H
