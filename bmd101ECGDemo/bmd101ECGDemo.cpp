// bmd101ECGDemo.cpp : 定义应用程序的类行为。
//

#include "stdafx.h"
#include "bmd101ECGDemo.h"
#include "bmd101ECGDemoDlg.h"
#include "myport.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// Cbmd101ECGDemoApp

BEGIN_MESSAGE_MAP(Cbmd101ECGDemoApp, CWinApp)
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()


// Cbmd101ECGDemoApp 构造

Cbmd101ECGDemoApp::Cbmd101ECGDemoApp()
{
	// TODO: 在此处添加构造代码，
	// 将所有重要的初始化放置在 InitInstance 中
}


// 唯一的一个 Cbmd101ECGDemoApp 对象

Cbmd101ECGDemoApp theApp;


// Cbmd101ECGDemoApp 初始化

BOOL Cbmd101ECGDemoApp::InitInstance()
{
	// 如果一个运行在 Windows XP 上的应用程序清单指定要
	// 使用 ComCtl32.dll 版本 6 或更高版本来启用可视化方式，
	//则需要 InitCommonControlsEx()。否则，将无法创建窗口。
	InitCommonControls(); 

	CWinApp::InitInstance();

	// 初始化 OLE 库
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}
	AfxEnableControlContainer();
	// 标准初始化
	// 如果未使用这些功能并希望减小
	// 最终可执行文件的大小，则应移除下列
	// 不需要的特定初始化例程
	// 更改用于存储设置的注册表项
	// TODO: 应适当修改该字符串，
	// 例如修改为公司或组织名
	SetRegistryKey(_T("应用程序向导生成的本地应用程序"));
	LoadStdProfileSettings(4);  // 加载标准 INI 文件选项(包括 MRU)

	Cbmd101ECGDemoDlg dlg;
	m_pMainWnd = &dlg;

	////////////////////////////////////////////////////////////////////
	 
	CSerialPort mySerialPort;

	if (!mySerialPort.InitPort(4))
	{
		std::cout << "initPort fail !" << std::endl;
	}
	else
	{
		std::cout << "initPort success !" << std::endl;
	}
	if (!mySerialPort.OpenListenThread())
	{
		std::cout << "OpenListenThread fail !" << std::endl;
	}
	else
	{
		std::cout << "OpenListenThread success !" << std::endl;
	}
	
/////////////////////////////////////////////////////////////////////////
	 
	INT_PTR nResponse = dlg.DoModal();
	
	int a=1;
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}



 