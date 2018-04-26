#include "HttpThread.h"


HttpThread::~HttpThread()
{
	this->thrd.join();
}
