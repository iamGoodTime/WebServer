#include "log.h"

using namespace std;
//构造日志对象初始行数为0，异步，日志写入线程为空，队列为空，日期为0，文件描述符为空指针
Log::Log()
{
    lineCount_ = 0;
    isAsync_ = false;
    writeThread_ = nullptr;
    deque_ = nullptr;
    toDay_ = 0;
    fp_ = nullptr;
}
//
Log::~Log()
{
    //日志写入线程存在且是可执行线程
    if (writeThread_ && writeThread_->joinable())
    {
        //队列非空
        while (!deque_->empty())
        {
            //数据从缓冲区写出
            deque_->flush();
        };
        //关闭日志队列
        deque_->Close();
        //等待调用写入线程结束
        writeThread_->join();
    }
    //
    if (fp_)
    {
        lock_guard<mutex> locker(mtx_);
        flush();
        fclose(fp_);
    }
}

int Log::GetLevel()
{
    lock_guard<mutex> locker(mtx_);
    return level_;
}

void Log::SetLevel(int level)
{
    lock_guard<mutex> locker(mtx_);
    level_ = level;
}
//初始化日志等级、文件路径、后缀、队最大值
void Log::init(int level = 1, const char *path, const char *suffix,
               int maxQueueSize)
{
    isOpen_ = true;
    level_ = level;
    //队列最大值大于0，同步
    if (maxQueueSize > 0)
    {
        isAsync_ = true;
        //队列非空
        if (!deque_)
        {
            unique_ptr<BlockDeque<std::string>> newDeque(new BlockDeque<std::string>);
            deque_ = move(newDeque);

            std::unique_ptr<std::thread> NewThread(new thread(FlushLogThread));
            writeThread_ = move(NewThread);
        }
    }
    else
    {
        isAsync_ = false;
    }

    lineCount_ = 0;

    //时间实例t获取时间
    time_t timer = time(nullptr);
    struct tm *sysTime = localtime(&timer);
    struct tm t = *sysTime;
    path_ = path;
    suffix_ = suffix;
    char fileName[LOG_NAME_LEN] = {0};
    //将 路径/年_月_日后缀 传入到fileName中
    snprintf(fileName, LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d%s",
             path_, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, suffix_);
    //日期号数1-31
    toDay_ = t.tm_mday;

    {
        lock_guard<mutex> locker(mtx_);
        buff_.RetrieveAll();
        //文件描述符非空
        if (fp_)
        {
            flush();
            //关闭文件描述符
            fclose(fp_);
        }
        //打开文件，“a”：在文件末尾添加内容
        fp_ = fopen(fileName, "a");
        //文件描述符为空
        if (fp_ == nullptr)
        {
            //创建一个以path_命名的文件夹，所有用户都有读、写、执行的权利
            mkdir(path_, 0777);
            //cout<<path_<<endl;
            fp_ = fopen(fileName, "a");
        }
        //fp_非空，继续执行
        assert(fp_ != nullptr);
    }
}

void Log::write(int level, const char *format, ...)
{
    struct timeval now = {0, 0};
    gettimeofday(&now, nullptr);
    //获取系统时间秒数
    time_t tSec = now.tv_sec;
    struct tm *sysTime = localtime(&tSec);
    struct tm t = *sysTime;
    va_list vaList;

    /* 日志日期 日志行数 */
    //日期不同||(日志行数>0&&日志满页)
    if (toDay_ != t.tm_mday || (lineCount_ && (lineCount_ % MAX_LINES == 0)))
    {
        unique_lock<mutex> locker(mtx_);
        locker.unlock();

        char newFile[LOG_NAME_LEN];
        char tail[36] = {0};
        //将年_月_日传入到tail中
        snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
        //日期不是当前日期
        if (toDay_ != t.tm_mday)
        {
            //将 path_/tail(+)suffix_写入nowFile
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s%s", path_, tail, suffix_);
            //更新日期
            toDay_ = t.tm_mday;
            //日志行数至0
            lineCount_ = 0;
        }
        else
        {
            //将path_/tail-当天日志数+1后缀
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s-%d%s", path_, tail, (lineCount_ / MAX_LINES), suffix_);
        }

        locker.lock();
        flush();
        fclose(fp_);
        //打开newFile文件
        fp_ = fopen(newFile, "a");
        //fp_文件描述符非空，程序继续执行
        assert(fp_ != nullptr);
    }

    {
        unique_lock<mutex> locker(mtx_);
        //行数加一
        lineCount_++;
        //将年-月-日-小时-分-秒写入到buff_.BeginWrite()中
        int n = snprintf(buff_.BeginWrite(), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
                         t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                         t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);

        buff_.HasWritten(n);
        //获取日志级别
        AppendLogLevelTitle_(level);

        va_start(vaList, format);
        int m = vsnprintf(buff_.BeginWrite(), buff_.WritableBytes(), format, vaList);
        va_end(vaList);

        buff_.HasWritten(m);
        buff_.Append("\n\0", 2);

        if (isAsync_ && deque_ && !deque_->full())
        {
            deque_->push_back(buff_.RetrieveAllToStr());
        }
        else
        {
            fputs(buff_.Peek(), fp_);
        }
        buff_.RetrieveAll();
    }
}

void Log::AppendLogLevelTitle_(int level)
{
    switch (level)
    {
    case 0:
        buff_.Append("[debug]: ", 9);
        break;
    case 1:
        buff_.Append("[info] : ", 9);
        break;
    case 2:
        buff_.Append("[warn] : ", 9);
        break;
    case 3:
        buff_.Append("[error]: ", 9);
        break;
    default:
        buff_.Append("[info] : ", 9);
        break;
    }
}

void Log::flush()
{
    if (isAsync_)
    {
        deque_->flush();
    }
    fflush(fp_);
}
//同步写入日志
void Log::AsyncWrite_()
{
    string str = "";
    while (deque_->pop(str))
    {
        lock_guard<mutex> locker(mtx_);
        fputs(str.c_str(), fp_);
    }
}

Log *Log::Instance()
{
    static Log inst;
    return &inst;
}

void Log::FlushLogThread()
{
    Log::Instance()->AsyncWrite_();
}