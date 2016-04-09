// bmd101ECGDemoDlg.cpp : implementation file
//filter 2015/7/1  
//添加心率与信号的输出框 2015/7/2  
//添加识别，峰值为单边

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
queue<double> q1,q2;  //全局变量raw y轴值
int mycount=0,row=0,concludei=0,selectq=1;
int signal=1,heartRate=0;
double temp[mymax];
int tempCount=0;
char heart[4],sl[4];
DaisyFilter *iir = DaisyFilter::SinglePoleIIRFilter(0.3f);         //滤波处理
DaisyFilter *avg = DaisyFilter::MovingAverageFilter(3);

double ytemp[801];
int ymark=1,id=-1;

double Theta1[8][14];
double   Theta2[4][9];

////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////
/** 线程退出标志 */ 
bool CSerialPort::s_bExit = false;
/** 当串口无数据时，sleep至下次查询间隔的时间，单位：s */ 
const UINT SLEEP_TIME_INTERVAL = 5;

CSerialPort::CSerialPort(void)
: m_hListenThread(INVALID_HANDLE_VALUE)
{
	FILE *f1;										// 读取Theta1
	f1 = fopen("Theta1.txt","r");
	for (int i=0;i<8;i++)
		for(int j=0;j<14;j++)
			fscanf(f1,"%lf",&Theta1[i][j]);
	fclose(f1);

	FILE *f2;                                       //读取Theta2
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

	/** 临时变量，将制定参数转化为字符串形式，以构造DCB结构 */ 
	char szDCBparam[50];
	sprintf_s(szDCBparam, "baud=%d parity=%c data=%d stop=%d", baud, parity, databits, stopsbits);

	/** 打开指定串口，该函数内部已经有临界区保护，上面请不要加保护 */ 
	if (!openPort(portNo))
	{
		return false;
	}

	/** 进入临界段 */ 
	EnterCriticalSection(&m_csCommunicationSync);

	/** 是否有错误发生 */ 
	BOOL bIsSuccess = TRUE;

    /** 在此可以设置输入输出缓冲区大小，如果不设置，则系统会设置默认值
	 *  自己设置缓冲区大小时，要注意设置稍大一些，避免缓冲区溢出
	 */
	/*if (bIsSuccess )
	{
		bIsSuccess = SetupComm(m_hComm,10,10);
	}*/

	/** 设置串口的超时时间，均设为0，表示不使用超时限制 */
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
		// 将ANSI字符串转换为UNICODE字符串
		DWORD dwNum = MultiByteToWideChar (CP_ACP, 0, szDCBparam, -1, NULL, 0);
		wchar_t *pwText = new wchar_t[dwNum] ;
		if (!MultiByteToWideChar (CP_ACP, 0, szDCBparam, -1, pwText, dwNum))
		{
			bIsSuccess = TRUE;
		}

		/** 获取当前串口配置参数，并且构造串口DCB参数 */ 
		bIsSuccess = GetCommState(m_hComm, &dcb) && BuildCommDCB((LPCWSTR)pwText, &dcb) ;
		/** 开启RTS flow控制 */ 
		dcb.fRtsControl = RTS_CONTROL_ENABLE; 

		/**释放内存空间 */ 
		delete [] pwText;
	}

	if ( bIsSuccess )
	{
		/** 使用DCB参数配置串口状态 */ 
		bIsSuccess = SetCommState(m_hComm, &dcb);
	}
		
	/** 清空串口缓冲区  */
	PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

	/**离开临界段 */ 
	LeaveCriticalSection(&m_csCommunicationSync);

	return bIsSuccess==TRUE;
}

bool CSerialPort::InitPort( UINT portNo ,const LPDCB& plDCB )
{
	/** 打开指定串口，该函数内部已经有临界区保护，上面请不要加保护 */ 
	if (!openPort(portNo))
	{
		return false;
	}
	
	/** 进入临界段 */ 
	EnterCriticalSection(&m_csCommunicationSync);

	/** 配置串口参数 */ 
	if (!SetCommState(m_hComm, plDCB))
	{
		return false;
	}

	/**  清空串口缓冲区 */
	PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

	/** 离开临界段 */ 
	LeaveCriticalSection(&m_csCommunicationSync);

	return true;
}

void CSerialPort::ClosePort()
{
	/** 如果有串口被打开，关闭它 */
	if( m_hComm != INVALID_HANDLE_VALUE )
	{
		CloseHandle( m_hComm );
		m_hComm = INVALID_HANDLE_VALUE;
	}
}

bool CSerialPort::openPort( UINT portNo )
{
	/** 进入临界段 */ 
	EnterCriticalSection(&m_csCommunicationSync);

	/** 把串口的编号转换为设备名 */ 
    char szPort[50];
	sprintf_s(szPort, "COM%d", portNo);

	/**打开指定的串口 */ 
	m_hComm = CreateFileA(szPort,		                /** 设备名，COM1，COM2等 */ 
						 GENERIC_READ | GENERIC_WRITE,  /** 访问模式，可同时读写 */   
						 0,                             /** 共享模式，0表示不共享 */ 
					     NULL,							/** 安全性设置，一般使用NULL */ 
					     OPEN_EXISTING,					/** 该参数表示设备必须存在，否则创建失败 */ 
						 0,    
						 0);    

	/** 如果打开失败，释放资源并返回 */ 
	if (m_hComm == INVALID_HANDLE_VALUE)
	{
		LeaveCriticalSection(&m_csCommunicationSync);
		return false;
	}

	/** 退出临界区 */ 
	LeaveCriticalSection(&m_csCommunicationSync);

	return true;
}

bool CSerialPort::OpenListenThread()
{
	/** 检测线程是否已经开启了 */ 
	if (m_hListenThread != INVALID_HANDLE_VALUE)
	{
		/** 线程已经开启 */ 
		return false;
	}

	s_bExit = false;
	/** 线程ID */ 
	UINT threadId;
	/** 开启串口数据监听线程 */ 
	m_hListenThread = (HANDLE)_beginthreadex(NULL, 0, ListenThread, this, 0, &threadId);
	if (!m_hListenThread)
	{
		return false;
	}
	/** 设置线程的优先级，高于普通线程 */ 
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
		/** 通知线程退出 */ 
		s_bExit = true;

		/** 等待线程退出 */ 
		Sleep(10);

		/** 置线程句柄无效 */ 
		CloseHandle( m_hListenThread );
		m_hListenThread = INVALID_HANDLE_VALUE;
	}
	return true;
}

UINT CSerialPort::GetBytesInCOM()
{
	DWORD dwError = 0;	/** 错误码*/ 
	COMSTAT  comstat;   /** COMSTAT结构体，记录通信设备的状态信息 */ 
	memset(&comstat, 0, sizeof(COMSTAT));

	UINT BytesInQue = 0;
	/** 在调用ReadFile和WriteFile之前，通过本函数清楚以前遗留的错误标志 */ 
	if ( ClearCommError(m_hComm, &dwError, &comstat) )
	{
		BytesInQue = comstat.cbInQue; /** 获取在输入缓冲区中的字节数 */ 
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
	/** 得到本类的指针 */ 
	CSerialPort *pSerialPort = reinterpret_cast<CSerialPort*>(pParam);

	///////////////////////////////////////////
	double raw=0;

	int check = 0,i=0,mark=0;
	char cRecved = 0x00;
	int rawdata=0;
	unsigned char cR[2]={0x00};
	unsigned char payload[256],paylength,cs;

	 
	q1.push(0);	
 
	// 线程循环，轮询方式读取串口数据
	while (!pSerialPort->s_bExit) 
	{
		UINT BytesInQue = pSerialPort->GetBytesInCOM();
		/** 如果串口输入缓冲区中无数据，则休息一会在查询 */ 
		if ( BytesInQue == 0 )
		{
			Sleep(SLEEP_TIME_INTERVAL);
			continue;
		}

		/** 读取输入缓冲区中的数据并输出显示 */
		 
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

	/** 临界区保护 */ 
	EnterCriticalSection(&m_csCommunicationSync);

	/** 从缓冲区读取一个字节的数据 */ 
	bResult = ReadFile(m_hComm, &cRecved, 1, &BytesRead, NULL);
	if ((!bResult))
	{ 
		/** 获取错误码，可以根据该错误码查出错误的原因 */ 
		DWORD dwError = GetLastError();

		/** 清空串口缓冲区 */ 
		PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_RXABORT);
		LeaveCriticalSection(&m_csCommunicationSync);

		return false;
	}

	/** 离开缓冲区 */ 
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

	/** 临界区保护 */ 
	EnterCriticalSection(&m_csCommunicationSync);

	/** 向缓冲区写入指定量的数据 */ 
	bResult = WriteFile(m_hComm, pData, length, &BytesToSend, NULL);
	if (!bResult)  
	{
		DWORD dwError = GetLastError();
		/** 清空串口缓冲区 */ 
		PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_RXABORT);
		LeaveCriticalSection(&m_csCommunicationSync);

		return false;
	}

	/** 离开临界区 */ 
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
