// bmd101ECGDemo.h : bmd101ECGDemo Ӧ�ó������ͷ�ļ�
//
#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"       // ������
#include "myport.h"

// Cbmd101ECGDemoApp:
// �йش����ʵ�֣������ bmd101ECGDemo.cpp
//

class Cbmd101ECGDemoApp : public CWinApp
{
public:
	Cbmd101ECGDemoApp();


// ��д
public:
	virtual BOOL InitInstance();

// ʵ��
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
};

extern Cbmd101ECGDemoApp theApp;