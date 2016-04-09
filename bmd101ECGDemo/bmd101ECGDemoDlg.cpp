// bmd101ECGDemoDlg.cpp : implementation file
//filter 2015/7/1  
//����������źŵ������ 2015/7/2  
//���ʶ�𣬷�ֵΪ����

#include "stdafx.h"
#include "bmd101ECGDemo.h"
#include "bmd101ECGDemoDlg.h"
#include "math.h"
#include "myport.h"
 
using namespace RealtimeCurve;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#define mymax 300

////////////////////////////////////////////////////////
queue<double> q1,q2;  //ȫ�ֱ���raw y��ֵ
int mycount=0,row=0,concludei=0,selectq=1;
int signal=1,heartRate=0;
double temp[mymax];
int tempCount=0;
char heart[4],sl[4];
DaisyFilter *iir = DaisyFilter::SinglePoleIIRFilter(0.3f);         //�˲�����
DaisyFilter *avg = DaisyFilter::MovingAverageFilter(3);

double ytemp[801];
int ymark=1,id=-1;

double Theta1[8][14];
double   Theta2[4][9];

////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////
/** �߳��˳���־ */ 
bool CSerialPort::s_bExit = false;
/** ������������ʱ��sleep���´β�ѯ�����ʱ�䣬��λ��s */ 
const UINT SLEEP_TIME_INTERVAL = 5;

CSerialPort::CSerialPort(void)
: m_hListenThread(INVALID_HANDLE_VALUE)
{
	FILE *f1;										// ��ȡTheta1
	f1 = fopen("Theta1.txt","r");
	for (int i=0;i<8;i++)
		for(int j=0;j<14;j++)
			fscanf(f1,"%lf",&Theta1[i][j]);
	fclose(f1);

	FILE *f2;                                       //��ȡTheta2
	f2 = fopen("Theta2.txt","r");
	for (int i=0;i<5;i++)
		for(int j=0;j<9;j++)
			fscanf(f2,"%lf",&Theta2[i][j]);
	fclose(f2);

	m_hComm = INVALID_HANDLE_VALUE;
	m_hListenThread = INVALID_HANDLE_VALUE;

	InitializeCriticalSection(&m_csCommunicationSync);

}

CSerialPort::~CSerialPort(void)
{
	CloseListenTread();
	ClosePort();
	DeleteCriticalSection(&m_csCommunicationSync);
}

bool CSerialPort::InitPort( UINT portNo /*= 1*/,UINT baud /*= CBR_9600*/,char parity /*= 'N'*/,
						    UINT databits /*= 8*/, UINT stopsbits /*= 1*/,DWORD dwCommEvents /*= EV_RXCHAR*/ )
{

	/** ��ʱ���������ƶ�����ת��Ϊ�ַ�����ʽ���Թ���DCB�ṹ */ 
	char szDCBparam[50];
	sprintf_s(szDCBparam, "baud=%d parity=%c data=%d stop=%d", baud, parity, databits, stopsbits);

	/** ��ָ�����ڣ��ú����ڲ��Ѿ����ٽ��������������벻Ҫ�ӱ��� */ 
	if (!openPort(portNo))
	{
		return false;
	}

	/** �����ٽ�� */ 
	EnterCriticalSection(&m_csCommunicationSync);

	/** �Ƿ��д����� */ 
	BOOL bIsSuccess = TRUE;

    /** �ڴ˿����������������������С����������ã���ϵͳ������Ĭ��ֵ
	 *  �Լ����û�������Сʱ��Ҫע�������Դ�һЩ�����⻺�������
	 */
	/*if (bIsSuccess )
	{
		bIsSuccess = SetupComm(m_hComm,10,10);
	}*/

	/** ���ô��ڵĳ�ʱʱ�䣬����Ϊ0����ʾ��ʹ�ó�ʱ���� */
	COMMTIMEOUTS  CommTimeouts;
	CommTimeouts.ReadIntervalTimeout         = 0;
	CommTimeouts.ReadTotalTimeoutMultiplier  = 0;
	CommTimeouts.ReadTotalTimeoutConstant    = 0;
	CommTimeouts.WriteTotalTimeoutMultiplier = 0;
	CommTimeouts.WriteTotalTimeoutConstant   = 0; 
	if ( bIsSuccess)
	{
		bIsSuccess = SetCommTimeouts(m_hComm, &CommTimeouts);
	}

	DCB  dcb;
	if ( bIsSuccess )
	{
		// ��ANSI�ַ���ת��ΪUNICODE�ַ���
		DWORD dwNum = MultiByteToWideChar (CP_ACP, 0, szDCBparam, -1, NULL, 0);
		wchar_t *pwText = new wchar_t[dwNum] ;
		if (!MultiByteToWideChar (CP_ACP, 0, szDCBparam, -1, pwText, dwNum))
		{
			bIsSuccess = TRUE;
		}

		/** ��ȡ��ǰ�������ò��������ҹ��촮��DCB���� */ 
		bIsSuccess = GetCommState(m_hComm, &dcb) && BuildCommDCB((LPCWSTR)pwText, &dcb) ;
		/** ����RTS flow���� */ 
		dcb.fRtsControl = RTS_CONTROL_ENABLE; 

		/**�ͷ��ڴ�ռ� */ 
		delete [] pwText;
	}

	if ( bIsSuccess )
	{
		/** ʹ��DCB�������ô���״̬ */ 
		bIsSuccess = SetCommState(m_hComm, &dcb);
	}
		
	/** ��մ��ڻ�����  */
	PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

	/**�뿪�ٽ�� */ 
	LeaveCriticalSection(&m_csCommunicationSync);

	return bIsSuccess==TRUE;
}

bool CSerialPort::InitPort( UINT portNo ,const LPDCB& plDCB )
{
	/** ��ָ�����ڣ��ú����ڲ��Ѿ����ٽ��������������벻Ҫ�ӱ��� */ 
	if (!openPort(portNo))
	{
		return false;
	}
	
	/** �����ٽ�� */ 
	EnterCriticalSection(&m_csCommunicationSync);

	/** ���ô��ڲ��� */ 
	if (!SetCommState(m_hComm, plDCB))
	{
		return false;
	}

	/**  ��մ��ڻ����� */
	PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

	/** �뿪�ٽ�� */ 
	LeaveCriticalSection(&m_csCommunicationSync);

	return true;
}

void CSerialPort::ClosePort()
{
	/** ����д��ڱ��򿪣��ر��� */
	if( m_hComm != INVALID_HANDLE_VALUE )
	{
		CloseHandle( m_hComm );
		m_hComm = INVALID_HANDLE_VALUE;
	}
}

bool CSerialPort::openPort( UINT portNo )
{
	/** �����ٽ�� */ 
	EnterCriticalSection(&m_csCommunicationSync);

	/** �Ѵ��ڵı��ת��Ϊ�豸�� */ 
    char szPort[50];
	sprintf_s(szPort, "COM%d", portNo);

	/**��ָ���Ĵ��� */ 
	m_hComm = CreateFileA(szPort,		                /** �豸����COM1��COM2�� */ 
						 GENERIC_READ | GENERIC_WRITE,  /** ����ģʽ����ͬʱ��д */   
						 0,                             /** ����ģʽ��0��ʾ������ */ 
					     NULL,							/** ��ȫ�����ã�һ��ʹ��NULL */ 
					     OPEN_EXISTING,					/** �ò�����ʾ�豸������ڣ����򴴽�ʧ�� */ 
						 0,    
						 0);    

	/** �����ʧ�ܣ��ͷ���Դ������ */ 
	if (m_hComm == INVALID_HANDLE_VALUE)
	{
		LeaveCriticalSection(&m_csCommunicationSync);
		return false;
	}

	/** �˳��ٽ��� */ 
	LeaveCriticalSection(&m_csCommunicationSync);

	return true;
}

bool CSerialPort::OpenListenThread()
{
	/** ����߳��Ƿ��Ѿ������� */ 
	if (m_hListenThread != INVALID_HANDLE_VALUE)
	{
		/** �߳��Ѿ����� */ 
		return false;
	}

	s_bExit = false;
	/** �߳�ID */ 
	UINT threadId;
	/** �����������ݼ����߳� */ 
	m_hListenThread = (HANDLE)_beginthreadex(NULL, 0, ListenThread, this, 0, &threadId);
	if (!m_hListenThread)
	{
		return false;
	}
	/** �����̵߳����ȼ���������ͨ�߳� */ 
	if (!SetThreadPriority(m_hListenThread, THREAD_PRIORITY_ABOVE_NORMAL))
	{
		return false;
	}

	return true;
}

bool CSerialPort::CloseListenTread()
{	
	if (m_hListenThread != INVALID_HANDLE_VALUE)
	{
		/** ֪ͨ�߳��˳� */ 
		s_bExit = true;

		/** �ȴ��߳��˳� */ 
		Sleep(10);

		/** ���߳̾����Ч */ 
		CloseHandle( m_hListenThread );
		m_hListenThread = INVALID_HANDLE_VALUE;
	}
	return true;
}

UINT CSerialPort::GetBytesInCOM()
{
	DWORD dwError = 0;	/** ������*/ 
	COMSTAT  comstat;   /** COMSTAT�ṹ�壬��¼ͨ���豸��״̬��Ϣ */ 
	memset(&comstat, 0, sizeof(COMSTAT));

	UINT BytesInQue = 0;
	/** �ڵ���ReadFile��WriteFile֮ǰ��ͨ�������������ǰ�����Ĵ����־ */ 
	if ( ClearCommError(m_hComm, &dwError, &comstat) )
	{
		BytesInQue = comstat.cbInQue; /** ��ȡ�����뻺�����е��ֽ��� */ 
	}

	return BytesInQue;
}

UINT WINAPI CSerialPort::ListenThread( void* pParam )
{
	///////////////////////////////////////////
	ofstream outfile,mysignal;
	outfile.open("myfile.txt");
	mysignal.open("signalfile.txt");
	////////////////////////////////////////////
	/** �õ������ָ�� */ 
	CSerialPort *pSerialPort = reinterpret_cast<CSerialPort*>(pParam);

	///////////////////////////////////////////
	double raw=0;

	int check = 0,i=0,mark=0;
	char cRecved = 0x00;
	int rawdata=0;
	unsigned char cR[2]={0x00};
	unsigned char payload[256],paylength,cs;

	 
	q1.push(0);	
 
	// �߳�ѭ������ѯ��ʽ��ȡ��������
	while (!pSerialPort->s_bExit) 
	{
		UINT BytesInQue = pSerialPort->GetBytesInCOM();
		/** ����������뻺�����������ݣ�����Ϣһ���ڲ�ѯ */ 
		if ( BytesInQue == 0 )
		{
			Sleep(SLEEP_TIME_INTERVAL);
			continue;
		}

		/** ��ȡ���뻺�����е����ݲ������ʾ */
		 
		do
		{
			 
			if(pSerialPort->ReadChar(cRecved) == true)
			{
				unsigned char temp = cRecved&0x000000ff; 
				if(check == 0&&temp == SYNC)
				{
					check = 1;
				//	cR[0]=temp;
					continue;
				}
				if(check == 1&&temp == SYNC)
				{
					check = 2;
				//	cR[1]=temp;
					continue;
				}
				else if(check == 1&&temp != SYNC)
				{
					check = 0;
					continue;
				}
				if(check == 2)
				{
					 paylength = temp;
					 if(paylength > 169)
					 {
						check=0;
						continue;
					 }
					 else
					 {
						check = 3;
						continue;
					 }
				}
				if(check>=3)
				{
					if(i<paylength)
					{
						payload[i++] = temp;
					}
					else
					{
						cs = temp;
						i=0;check=0,mark=1;
					}
				}	
			}
			if(i==0&&check==0&&mark==1)
			{
				int bytecount=0;
				unsigned char checksum = 0x00;
				unsigned char code,length;
				 
				for(int j=0;j<paylength;j++)
					checksum+=payload[j];
				checksum &= 0xFF;
				checksum = ~checksum & 0xFF;
				if(checksum != cs)  
					continue;
				else 
				{
					if(paylength == 4)
					{
						code = payload[bytecount++];
						if( code & 0x80 ) length = payload[bytecount++];
						else length = 1;
						if(signal!=0)
						{
							rawdata=payload[bytecount]<<8|payload[bytecount+1];
							if( rawdata >= 32768 ) rawdata = rawdata - 65536;
							if(abs(rawdata)>6000) 
							{
								rawdata=0;						
							}
						}
						else rawdata = 0;
						raw=rawdata*18.3/128.0;
						 
						
						
						ytemp[ymark++]=raw/400;
						if(ymark>=801)	 
						{
							id=identify(ytemp);
							ymark=1;
						}
						 
					
	
						//////////////////////////////////////////////////////
						if(row==1&&concludei==1&&selectq%5==0)
						{
							if(tempCount<mymax)
								temp[tempCount++]=raw;
							else
							{
								if(mycount==1)
								{							 
									q1.push(raw);							 
								}
								if(mycount==0)
								{							 
									q2.push(raw);							 
								}
							}
							selectq=1;
							//outfile<<raw<<std::endl;
						}
						selectq++;
						///////////////////////////////////////////////////////
						outfile<<raw/400.0<<std::endl;
					}
					if(paylength == 18)
					{						 
						signal = payload[1];
						if(signal>0)
							heartRate = payload[3];
						else
							heartRate = 0;
						mysignal<<signal<<" "<<heartRate<<std::endl;					 
					}
				}
				mark=0;
			}
		}while(--BytesInQue);
	}	 
	mysignal.close();
	outfile.close();	 
	return 0;
}

bool CSerialPort::ReadChar( char &cRecved )
{
	BOOL  bResult     = TRUE;
	DWORD BytesRead   = 0;
	if(m_hComm == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	/** �ٽ������� */ 
	EnterCriticalSection(&m_csCommunicationSync);

	/** �ӻ�������ȡһ���ֽڵ����� */ 
	bResult = ReadFile(m_hComm, &cRecved, 1, &BytesRead, NULL);
	if ((!bResult))
	{ 
		/** ��ȡ�����룬���Ը��ݸô������������ԭ�� */ 
		DWORD dwError = GetLastError();

		/** ��մ��ڻ����� */ 
		PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_RXABORT);
		LeaveCriticalSection(&m_csCommunicationSync);

		return false;
	}

	/** �뿪������ */ 
	LeaveCriticalSection(&m_csCommunicationSync);

	return (BytesRead == 1);

}

bool CSerialPort::WriteData( unsigned char* pData, unsigned int length )
{
	BOOL   bResult     = TRUE;
	DWORD  BytesToSend = 0;
	if(m_hComm == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	/** �ٽ������� */ 
	EnterCriticalSection(&m_csCommunicationSync);

	/** �򻺳���д��ָ���������� */ 
	bResult = WriteFile(m_hComm, pData, length, &BytesToSend, NULL);
	if (!bResult)  
	{
		DWORD dwError = GetLastError();
		/** ��մ��ڻ����� */ 
		PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_RXABORT);
		LeaveCriticalSection(&m_csCommunicationSync);

		return false;
	}

	/** �뿪�ٽ��� */ 
	LeaveCriticalSection(&m_csCommunicationSync);

	return true;
}
///////////////////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{

}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// Cbmd101ECGDemoDlg dialog



Cbmd101ECGDemoDlg::Cbmd101ECGDemoDlg(CWnd* pParent /*=NULL*/)
	: CDialog(Cbmd101ECGDemoDlg::IDD, pParent)
	, edit(_T(""))
{
	m_time	= 0.0f;
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_iBtnExitFromRight = 10;
	m_iBtnExitFromBottom =10;
}

void Cbmd101ECGDemoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PLOT, m_plot);
	DDX_Control(pDX, IDOK, m_btnExit);
	DDX_Text(pDX, IDC_EDIT1, edit);
}

BEGIN_MESSAGE_MAP(Cbmd101ECGDemoDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_START, OnBnClickedStart)
	ON_BN_CLICKED(IDC_STOP, OnBnClickedStop)
	ON_WM_SIZE()
	ON_WM_SIZING()
	ON_BN_CLICKED(IDC_DISP_LINE1, OnBnClickedDispLine1)
	ON_BN_CLICKED(IDC_DISP_LINE2, OnBnClickedDispLine2)
	ON_WM_LBUTTONDOWN()
	ON_EN_CHANGE(IDC_EDIT1, &Cbmd101ECGDemoDlg::OnEnChangeEdit1)
END_MESSAGE_MAP()


// Cbmd101ECGDemoDlg message handlers

BOOL Cbmd101ECGDemoDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	((CButton*)GetDlgItem(IDC_DISP_LINE1))->SetCheck(TRUE);
	((CButton*)GetDlgItem(IDC_DISP_LINE2))->SetCheck(TRUE);
	
	//remember initial position
	GetWindowRect(&m_rectOldWindow);
	m_btnExit.GetClientRect(&m_rectBtnExit);
	m_plot.GetWindowRect(&m_rectOldPlotWindow);
	TRACE("m_rectOldPlotWindow.left=%d,m_rectOldPlotWindow.width=%d\n",m_rectOldPlotWindow.left,m_rectOldPlotWindow.Width());
	ScreenToClient(&m_rectOldPlotWindow);
	TRACE("m_rectOldPlotWindow.left=%d,m_rectOldPlotWindow.width=%d\n",m_rectOldPlotWindow.left,m_rectOldPlotWindow.Width());

	m_plot.AddNewLine(_T("PV"),PS_SOLID,RGB(255,0,0));
	m_plot.AddNewLine(_T("SV"),PS_SOLID,RGB(0,255,0));

	m_plot.GetAxisX().AxisColor=RGB(0,255,0);
	m_plot.GetAxisY().AxisColor=RGB(0,255,0);
	//sprintf(heart,"%s",123);
	//GetDlgItem(IDC_EDIT1)->SetWindowTextA(_T("123"));


	return TRUE;  // return TRUE  unless you set the focus to a control
}

void Cbmd101ECGDemoDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void Cbmd101ECGDemoDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR Cbmd101ECGDemoDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

 
void Cbmd101ECGDemoDlg::OnTimer(UINT nIDEvent)
{

	//In fact, x values are not used when draw lines
	//If you want to use x values in any way, 
	//you may calculate point x in the way that point y is calculated.
	//see also: DrawLines(CDC* pDC);
	 
		sprintf(heart,"%d",heartRate);
		GetDlgItem(IDC_EDIT1)->SetWindowText(LPCTSTR(heart));
		sprintf(sl,"%d",signal);
		GetDlgItem(IDC_EDIT2)->SetWindowText(LPCTSTR(sl));

	////////////////////////////////////////////////////////////
		if(signal==0)
		{
			id=-1;
		}
		switch(id)
		{
			case 1:GetDlgItem(IDC_EDIT3)->SetWindowTextW(_T("zmz"));break;
			case 2:GetDlgItem(IDC_EDIT3)->SetWindowTextW(_T("ln"));break;
			case 3:GetDlgItem(IDC_EDIT3)->SetWindowTextW(_T("crz"));break;
			case 4:GetDlgItem(IDC_EDIT3)->SetWindowTextW(_T("four"));break;
			default:GetDlgItem(IDC_EDIT3)->SetWindowTextW(_T("None"));break;
		}
	////////////////////////////////////////////////////////////
		row=1;
		double y0=0,y1=0;
		if(tempCount>=mymax&&signal>0)
		{
			if(!q1.empty()&&mycount==1)
			{
				if(!q2.empty())
				{
					y0=q2.front();			 
					q2.pop();
				}
				if(q2.empty())
				{
					mycount=0;		
				}
			}
			if(!q2.empty()&&mycount==0)
			{
				if(!q1.empty())
				{
					y0=q1.front();
					q1.pop();
				}
				if(q1.empty())
				{
					mycount=1;
				}
			}
		}
		else
		{
			concludei=0;			 
			while(!q1.empty())
				q1.pop();
			while(!q2.empty())
				q2.pop();
			concludei=1;

		}
		 y1=y0;
		y0=y0/3; 
		y0=iir->Calculate(y0);
		m_plot.AddNewPoint(m_time,y0,0);
		//m_plot.AddNewPoint(m_time,y1,1);
		m_time += 0.20f;
	 
	//m_plot.GetAxisX().GetRangeLowerLimit() += 0.10f; 
	//CDialog::OnTimer(nIDEvent);
}

void Cbmd101ECGDemoDlg::OnBnClickedStart()
{
	 
	 
	SetTimer(1,2,NULL);
	m_plot.SetRate(10);//
	m_plot.Start();

	GetDlgItem(IDC_START)->EnableWindow(FALSE);
}

void Cbmd101ECGDemoDlg::OnBnClickedStop()
{
	KillTimer(1);
	m_plot.Stop();
	//for(int i=0;i<m_plot.GetLineCount();i++)
	//{
	//	m_plot.GetLineByIndex(i).RemoveAllPoints();
	//}
	GetDlgItem(IDC_START)->EnableWindow();
	m_plot.Invalidate();
}

/*
 Set new control postition as needed.
 */
void Cbmd101ECGDemoDlg::OnSize(UINT nType, int cx, int cy)
{	
	CDialog::OnSize(nType, cx, cy);
	CRect rect;
	GetClientRect(&rect);

		
	if(m_btnExit.GetSafeHwnd())
	{
		m_btnExit.MoveWindow(rect.right-m_iBtnExitFromRight-m_rectBtnExit.Width(),
		rect.bottom-m_iBtnExitFromBottom-m_rectBtnExit.Height(),
		m_rectBtnExit.Width(),m_rectBtnExit.Height());
	}
	if(m_plot.GetSafeHwnd())
	{
		//TRACE("m_rectOldPlotWindow.left=%d,m_rectOldPlotWindow.width=%d\n",m_rectOldPlotWindow.left,m_rectOldPlotWindow.Width());

		m_plot.MoveWindow(m_rectOldPlotWindow.left,
			m_rectOldPlotWindow.top,
			rect.right-m_iBtnExitFromRight-5-m_rectOldPlotWindow.left,
			rect.bottom-m_iBtnExitFromBottom-m_rectBtnExit.Height()-7-m_rectOldPlotWindow.top);
	}

	
}
/*
 Check window size while resizeing, if small than origianl size, then restore to original size.
 */
void Cbmd101ECGDemoDlg::OnSizing(UINT fwSide, LPRECT pRect)
{
	/*if(pRect->right - pRect->left <=m_rectOldWindow.Width())
     	pRect->right = pRect->left + m_rectOldWindow.Width();
	
     if(pRect->bottom - pRect->top <=m_rectOldWindow.Height())
     	pRect->bottom = pRect->top + m_rectOldWindow.Height();*/

	CDialog::OnSizing(fwSide, pRect);
}

void Cbmd101ECGDemoDlg::OnBnClickedDispLine1()
{
	m_plot.GetLineByIndex(0)->IsShow = ((CButton*)GetDlgItem(IDC_DISP_LINE1))->GetCheck(); 
}

void Cbmd101ECGDemoDlg::OnBnClickedDispLine2()
{
	m_plot.GetLineByIndex(1)->IsShow = ((CButton*)GetDlgItem(IDC_DISP_LINE2))->GetCheck(); 
}


void Cbmd101ECGDemoDlg::OnEnChangeEdit1()
{
	 
}
