#ifndef _FPS_H_
#define _FPS_H_

#include <Windows.h>
#include <stdio.h>
#include <tchar.h>

//�K�v�ȃ��C�u�����t�@�C���̃��[�h
#pragma comment(lib,"winmm.lib")

#pragma warning(disable:4996)

class FPS
{
private:
	DWORD time;
	int frame;
	int av_frame;
	int fps[3600];//�v����1���Ԉȓ��ɂ��邱��
	int avg;
	int first2;
	TCHAR str[60];
public:
	void COUNTER(HWND hWnd);

};
#endif // _FPS_H_ 
