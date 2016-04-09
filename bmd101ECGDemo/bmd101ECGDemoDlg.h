// bmd101ECGDemoDlg.h : header file
//

#pragma once
#include "afxwin.h"
#include "Plot.h"

using namespace RealtimeCurve;

// bmd101ECGDemoDlg dialog
class Cbmd101ECGDemoDlg : public CDialog
{
// Construction
public:
	Cbmd101ECGDemoDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_bmd101ECGDemo_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnBnClickedStart();
	afx_msg void OnBnClickedStop();
	afx_msg void OnBnClickedSetting();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSizing(UINT fwSide, LPRECT pRect);
	afx_msg void OnBnClickedDispLine1();
	afx_msg void OnBnClickedDispLine2();
	DECLARE_MESSAGE_MAP()
	CPlot m_plot;

	CRect m_rectBtnExit;
	CRect m_rectOldWindow;
	CRect m_rectOldPlotWindow;
private:
	float m_time;
	
protected:
	CButton m_btnExit;
	int m_iBtnExitFromRight;
	int m_iBtnExitFromBottom;
	
public:
	CString edit;
	afx_msg void OnEnChangeEdit1();
};
