// bmd101ECGDemo.cpp : ����Ӧ�ó��������Ϊ��
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


// Cbmd101ECGDemoApp ����

Cbmd101ECGDemoApp::Cbmd101ECGDemoApp()
{
	// TODO: �ڴ˴���ӹ�����룬
	// ��������Ҫ�ĳ�ʼ�������� InitInstance ��
}


// Ψһ��һ�� Cbmd101ECGDemoApp ����

Cbmd101ECGDemoApp theApp;


// Cbmd101ECGDemoApp ��ʼ��

BOOL Cbmd101ECGDemoApp::InitInstance()
{
	// ���һ�������� Windows XP �ϵ�Ӧ�ó����嵥ָ��Ҫ
	// ʹ�� ComCtl32.dll �汾 6 ����߰汾�����ÿ��ӻ���ʽ��
	//����Ҫ InitCommonControlsEx()�����򣬽��޷��������ڡ�
	InitCommonControls(); 

	CWinApp::InitInstance();

	// ��ʼ�� OLE ��
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}
	AfxEnableControlContainer();
	// ��׼��ʼ��
	// ���δʹ����Щ���ܲ�ϣ����С
	// ���տ�ִ���ļ��Ĵ�С����Ӧ�Ƴ�����
	// ����Ҫ���ض���ʼ������
	// �������ڴ洢���õ�ע�����
	// TODO: Ӧ�ʵ��޸ĸ��ַ�����
	// �����޸�Ϊ��˾����֯��
	SetRegistryKey(_T("Ӧ�ó��������ɵı���Ӧ�ó���"));
	LoadStdProfileSettings(4);  // ���ر�׼ INI �ļ�ѡ��(���� MRU)

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



 