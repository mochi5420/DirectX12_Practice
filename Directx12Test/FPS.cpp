#include "FPS.h"

void FPS::COUNTER(HWND hWnd)
{

	swprintf(str, _T("fps=%d avg_fps=%d"), frame, avg);
	frame++;
	if (timeGetTime() - time>1000)
	{
		first2++;
		fps[av_frame] = frame;
		if (first2>2)
		{
			av_frame++;
			avg = 0;
			for (int i = 0; i<av_frame; i++)
			{
				avg += fps[i];
			}
			avg = avg / av_frame;
		}
		time = timeGetTime();
		frame = 0;
		SetWindowTextW(hWnd, str);
	}
}