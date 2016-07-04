#include <stdio.h>

class ProgressManager
{
public:
	ProgressManager()
		: mRunning(false)
	{ }

	void Start(int totalWork)
	{
		mSize = totalWork;
		mCurr = 0;
		mLastPct = 0;
		mRunning = true;
	}

	void Stop()
	{
		mRunning = false;
		printf("\n");
	}

	void Tick()
	{
		mCurr++;
		int currPct = mCurr*100/(mSize);
		for(int i=mLastPct+1;i<=currPct;i++)
		{
			if(i%10==0)
			{
				printf(" %d%%",i/10*10);
				if(i==50) printf("\n");
				else printf(" ");
			}
			else printf(".");
			mLastPct = currPct;
		}
	}

	int mSize, mCurr, mLastPct;
	bool mRunning;

	void TryNewline()
	{
		if(!mRunning) return;
		printf("\n");
	}

};

 extern ProgressManager g_ProgressManager;
