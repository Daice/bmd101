#include "stdafx.h"
#include "myport.h"
#define people 4 
//extern double Theta1[25][501];
//extern double Theta2[people][26];
extern double Theta1[8][14];
extern double Theta2[4][9];
double ytemp[2][401];
double feature[14];

int identify(double y[801])
{
	int d0,d1,dmm,i,j,length,f0,f1;
	double dmax0,dmax1,dmaxm,dmin0,dmin1;
	double *ecgtemp;
	for(i=1;i<401;i++)
	{
		ytemp[0][i]=y[i];
		ytemp[1][i]=y[i+400];
	}
	d0=findmax(ytemp[0],1,401);
	dmax0=ytemp[0][d0];
	d1=findmax(ytemp[1],1,401);
	dmax1=ytemp[1][d1];

	length=400+d1-d0+1;
	if(length<0)
		return -1;
	else
	{
		ecgtemp=new double[length+1];
		for(i=1,j=d0;j<=d1+400;j++)
			ecgtemp[i++]=y[j];
		f0=findmin(ecgtemp,1,90);
		dmin0=ecgtemp[f0];
		f1=findmin(ecgtemp,90,length);
		dmin1=ecgtemp[f1];
		dmm=findmax(ecgtemp,f0,f1);
		dmaxm=ecgtemp[dmm];

		feature[0]=1;
		feature[1]=dmax0;
		feature[2]=dmaxm;
		feature[3]=dmax1;
		feature[4]=dmin0;
		feature[5]=dmin1;
		feature[6]=dmax0-dmin0;
		feature[7]=dmaxm-dmin0;
		feature[8]=dmaxm-dmin1;
		feature[9]=dmax1-dmin1;
		feature[10]=double(length);
		feature[11]=double(f0)/length;
		feature[12]=double(f1-f0)/length;
		feature[13]=double(dmm)/length;

		return InterpolationTemp(feature);
		//return Interpolation(ecgtemp,length);
	}
}

int Interpolation(double  *ecgtemp,int length)              
{
	int l,m;
	int i,j;
	double ecg[501];
	double x[501];
	double NT1[26];
	double NT2[people+1];
	double d=(length-1)/499.0;
	x[0]=x[1]=1;
	for(i=2;i<501;i++)
		x[i]=x[i-1]+d;
	for(i=2;i<=length;i++)
	{
		for(j=1;j<501;j++)
		{
			if(x[j]<i&&x[j]>=i-1)
				ecg[j]=(ecgtemp[i]-ecgtemp[i-1])*(x[j]-i+1)+ecgtemp[i-1];
			if(x[j]>=i)
				break;
		}
	}
	ecg[500]=ecgtemp[length];
	ecg[0]=1;
	 
	memset(NT1,0,sizeof(NT1));
	NT1[0]=1;
	for(l=1;l<26;l++)
	{
		for(m=0;m<501;m++)
		{
			NT1[l]=Theta1[l-1][m]*ecg[m]+NT1[l];		
		}
		NT1[l]=1.0/(1.0+exp(-NT1[l]));
	}
	 
	memset(NT2,0,sizeof(NT2));
	for(l=1;l<5;l++)
	{
		for(m=0;m<26;m++)
		{
			NT2[l]=NT2[l]+Theta2[l-1][m]*NT1[m];
		}
		NT2[l]=1.0/(1.0+exp(-NT2[l]));
	}
	int id=findmax(NT2,1,5);
	return id;
}

int InterpolationTemp(double *ft)
{
	int l,m;
	double NT1[9];
	double NT2[people+1];

	memset(NT1,0,sizeof(NT1));
	NT1[0]=1;
	for(l=1;l<9;l++)
	{
		for(m=0;m<14;m++)
		{
			NT1[l]=Theta1[l-1][m]*ft[m]+NT1[l];		
		}
		NT1[l]=1.0/(1.0+exp(-NT1[l]));
	}
	 
	memset(NT2,0,sizeof(NT2));
	for(l=1;l<people+1;l++)
	{
		for(m=0;m<9;m++)
		{
			NT2[l]=NT2[l]+Theta2[l-1][m]*NT1[m];
		}
		NT2[l]=1.0/(1.0+exp(-NT2[l]));
	}
	int id=findmax(NT2,1,people+1);
	return id;
}
int findmax(double *y,int s,int length)
{
	int max=s,i=1;
	double maxvalue=y[i];
	for(;i<length;i++)
	{
		if(y[i]>maxvalue)
		{
			maxvalue=y[i];
			max=i;
		}
	}
	return max;
}

int findmin(double *y,int s,int e)
{
	int min=s,i=s;
	double minvalue=y[s];
	for(;i<e;i++)
	{
		if(y[i]<minvalue)
		{
			minvalue=y[i];
			min=i;
		}
	}
	return min;
}