#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <fstream>

#ifndef WIN32
#include <pthread.h>


#else
#define __func__ __FUNCTION__
#endif


#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifndef WIN32
#include <sys/time.h>
#include <unistd.h>
#else
#include <sys/timeb.h>
#include <Windows.h>
#include <direct.h>
#endif

#include <time.h>
#include "Mutex.hpp"

namespace NEC { namespace Log {

const char * const version = "1.0.0";
const size_t MSG_SIZE = 3000;
const size_t MAX_BUFF_SIZE = (512 * 1024);

enum MessageType
{
    None = 0,
    Fatal,
    Error,
    Warn,
    Info,
    Stamp,
    Trace
};

class GoLogger
{
private:
    Mutex m_oMutex; /**< mutex contol instance */
    MessageType m_logLevel;
    std::string m_buffList;
    std::string m_LogFilePath;
    std::string m_LogFileName;
    size_t m_maxFileSize;
    size_t m_maxBuffSize;
    uint32_t m_fileCap;

    GoLogger(std::string &filePath, std::string &fileName,
              const size_t fileSize, const size_t buffSize,
              const uint32_t fileCap, MessageType logLevel) :
        m_logLevel(logLevel),
        m_LogFilePath(filePath),
        m_LogFileName(fileName),
        m_maxFileSize(fileSize),
        m_maxBuffSize((buffSize > MAX_BUFF_SIZE ? MAX_BUFF_SIZE : buffSize)),
        m_fileCap(fileCap)
    {
        m_maxBuffSize = (m_maxBuffSize > m_maxFileSize) ? 0 : m_maxBuffSize;
        m_buffList.reserve(m_maxBuffSize);
    }

    ~GoLogger()
    { WriteLog(); }

    // Prevent copy constructor and assignment operator
    GoLogger(const GoLogger&)
    { }
    void operator=(const GoLogger&)
    { }

    void GetDateTime(std::string& date, std::string& time);
    void GetLogLevelString(MessageType msgType, std::string &MsgLevelStr);
    void AddToLogBuff(std::string& buff, MessageType msgLevel);
    bool WriteLog();
    void ShiftLog();

    // Need to adopt templates or inline function to make the library header-only.
    // This is a new way to make a singleton in a header only library.
    static GoLogger* MyInstance(GoLogger* ptr)
    {
        static GoLogger* myInstance = NULL;
        // I don't want to restrict ptr from being NULL - After Destroy, I would expect
        // this to be NULL for obvious reasons. But then the Instance method will not be
        // able to return this. So now the safety of the logger boils down to the maintainer.
        if (ptr)
            myInstance = ptr;
        return myInstance;
    }

public:

    static void Init(std::string &filePath, std::string &fileName,
                     const size_t fileSize, const size_t buffSize,
                     const uint32_t fileCap, MessageType logLevel)
    {
#ifndef WIN32
        mkdir(filePath.c_str(), 0755);
#else
       _mkdir(filePath.c_str());
#endif
        GoLogger* pInst = new GoLogger(filePath, fileName, fileSize,
                                         buffSize, fileCap, logLevel);
        MyInstance(pInst);
    }

    static GoLogger* Instance()
    { return MyInstance(NULL); }

    static void Destroy()
    {
        GoLogger *pInst = MyInstance(NULL);
        if (pInst != NULL) {
            pInst->WriteLog();
            delete pInst;
        }
    }

    MessageType LogLevel() const
    { return m_logLevel; }

    void Log(std::string& strFileName, std::string& strFuncName,
             int nLineNum, MessageType msgLevel, std::string& strMessage);
};

inline void GoLogger::GetDateTime(std::string& Date, std::string& Time)
{
   
#ifndef WIN32
    struct timeval detail_time;

    time_t long_time = 0;
    time(&long_time);                /* Get time as long integer. */
    struct tm tm1;

    localtime_r(&long_time, &tm1);
    gettimeofday(&detail_time, NULL);

    std::stringstream strm;
    
    strm << std::setw(2) << std::setfill('0') << tm1.tm_hour << ":"
        << std::setw(2) << std::setfill('0') << tm1.tm_min << ":"
        << std::setw(2) << std::setfill('0') << tm1.tm_sec << "."
        << std::setw(6) << detail_time.tv_usec;

    Time = strm.str();

    strm.str(std::string());
    strm << (tm1.tm_year + 1900) << "-"
        << std::setw(2) << std::setfill('0') << (tm1.tm_mon + 1) << "-"
        << std::setw(2) << std::setfill('0') << tm1.tm_mday;
    Date = strm.str();
#else
   SYSTEMTIME systemTime;
   GetLocalTime(&systemTime);
   std::stringstream strm;
   strm << std::setw(2) << std::setfill('0') << systemTime.wHour << ":"
      << std::setw(2) << std::setfill('0') << systemTime.wMinute << ":"
      << std::setw(2) << std::setfill('0') << systemTime.wSecond << "."
      << std::setw(6) << systemTime.wMilliseconds;
   Time = strm.str();

   strm.str(std::string());
   strm << (systemTime.wYear) << "-" << std::setw(2) << std::setfill('0') << (systemTime.wMonth) << "-" << std::setw(2) << std::setfill('0') << systemTime.wDay;
   Date = strm.str();    
#endif
    
}

inline void GoLogger::GetLogLevelString(MessageType msgType,
                                         std::string &MsgLevelStr)
{
    switch (msgType)
    {
    case Fatal: { MsgLevelStr = "FATAL"; break; }
    case Error: { MsgLevelStr = "ERROR"; break; }
    case Warn: { MsgLevelStr = " WARN"; break; }
    case Info: { MsgLevelStr = " INFO"; break; }
    case Stamp: { MsgLevelStr = "STAMP"; break; }
    case Trace: { MsgLevelStr = "TRACE"; break; }
    default: { break; }
    }
}

inline void GoLogger::AddToLogBuff(std::string& buff, MessageType msgLevel)
{
    m_buffList.append(buff);

    if (m_buffList.size() > m_maxBuffSize)
        WriteLog();
    else if (msgLevel == Fatal || msgLevel == Error || msgLevel == Warn)
        WriteLog();
}

inline void GoLogger::ShiftLog()
{
    char fname[300] = {0};
    char strSrcFName[512] = {0};
    char strDstFName[512] = {0};
    char LastFile[512] = {0};

    strcpy(fname, m_LogFileName.c_str());
    char *baseName = strtok(fname, ".");
    char *ext = strtok(NULL, ".");
    sprintf(LastFile, "%s/%s.%d.%s", m_LogFilePath.c_str(), baseName, m_fileCap, ext);

    struct stat statbuf;
    uint32_t startIdx = 1;
    if (stat(LastFile, &statbuf) == 0)
    {
        for (uint32_t i = 2; i <= m_fileCap; i++)
        {
            sprintf(strSrcFName, "%s/%s.%d.%s", m_LogFilePath.c_str(), baseName, i, ext);
            sprintf(strDstFName, "%s/%s.%d.%s", m_LogFilePath.c_str(), baseName, i - 1, ext);
            rename(strSrcFName, strDstFName);
        }
        startIdx = m_fileCap;
    }

    for (uint32_t fileNo = startIdx; fileNo <= m_fileCap; fileNo++)
    {
        char renLogFile[512] = {0};
        sprintf(renLogFile, "%s/%s.%d.%s", m_LogFilePath.c_str(), baseName, fileNo, ext);

        struct stat statbuf;
        if (stat(renLogFile, &statbuf) != 0)
        {
            std::string oldName(m_LogFilePath);
            oldName += "/";
            oldName += m_LogFileName;

            rename(oldName.c_str(), renLogFile);
            break;
        }
    }
}

inline bool GoLogger::WriteLog()
{
    std::string logFile;
    logFile += m_LogFilePath;
    logFile += "/";
    logFile += m_LogFileName;
    std::ofstream logStream(logFile.c_str(), std::ios::out | std::ios::app);

    if (!logStream.is_open()) {
        printf("Unable to open log file: %s\n", m_LogFileName.c_str());
        return false;
    }

    logStream << m_buffList;
    m_buffList.clear();

    logStream.flush();
    uint64_t currentSize = logStream.tellp();
    logStream.close();

    if (currentSize >= m_maxFileSize)
        ShiftLog();
    return true;
}

inline void GoLogger::Log(std::string& strFileName, std::string& strFuncName,
                           int nLineNum, MessageType msgLevel, std::string& strMessage)
{
    if (msgLevel > m_logLevel)
        return;

    std::string DateStr, TimeStr;
    std::string MsgLevelStr;

    MutexLocker lock(m_oMutex);

    GetDateTime(DateStr, TimeStr);
    GetLogLevelString(msgLevel, MsgLevelStr);

    std::stringstream strm;

#ifndef WIN32
    strm << DateStr.c_str() << " " << TimeStr.c_str() << " " << std::hex << pthread_self() << std::dec << " " << MsgLevelStr << " ["<< strFileName.c_str() << ":" << nLineNum << "] " << strMessage << std::endl;
#else
    strm << DateStr.c_str() << " " << TimeStr.c_str() << " " << std::hex << GetCurrentThreadId() << std::dec << " " << MsgLevelStr << " ["<< strFileName.c_str() << ":" << nLineNum << "] " << strMessage << std::endl;
#endif

    std::string buff(strm.str());
#ifdef LOG_ON_SCREEN
    std::cout << buff;
#endif

    AddToLogBuff(buff, msgLevel);
}

inline std::string MakeMessage(const char *format, ...)
{
    va_list argList;
    char buf[MSG_SIZE] = {0};
    va_start(argList, format);
    vsnprintf(buf, MSG_SIZE, format, argList);
    va_end(argList);

    std::string strBuf(buf);
    return strBuf;
}

inline std::string MakeMessage(const std::string& msg)
{
    return msg;
}

inline void Print(const char *fileName, const char *funcName,
                  int nLineNum, MessageType msgLevel, std::string strMessage)
{
    GoLogger *pLogger = GoLogger::Instance();
    if (pLogger) {
        std::string strFileName = fileName;
        std::string strFuncName = funcName;
        pLogger->Log(strFileName, strFuncName, nLineNum, msgLevel, strMessage);
    }
}

inline std::string GetVersion()
{ return version; }

inline void dummy(...)
{ }

} }

#define LogTrace(msg)   do { \
    using namespace NEC::Log; \
    GoLogger *pLog = GoLogger::Instance(); \
    if (pLog) { \
        if (pLog->LogLevel() == None || pLog->LogLevel() < Trace) \
        break; \
        Print(__FILE__, __func__, __LINE__, Trace, MakeMessage msg); \
    } \
} while (0)

#define LogStamp(msg)   do { \
    using namespace NEC::Log; \
    GoLogger *pLog = GoLogger::Instance(); \
    if (pLog) { \
        if (pLog->LogLevel() == None || pLog->LogLevel() < Stamp) \
        break; \
        Print(__FILE__, __func__, __LINE__, Stamp, MakeMessage msg); \
    } \
} while (0)

#define LogInfo(msg)    do { \
    using namespace NEC::Log; \
    GoLogger *pLog = GoLogger::Instance(); \
    if (pLog) { \
        if (pLog->LogLevel() == None || pLog->LogLevel() < Info) \
        break; \
        Print(__FILE__, __func__, __LINE__, Info, MakeMessage msg); \
    } \
} while (0)

#define LogWarn(msg)    do { \
    using namespace NEC::Log; \
    GoLogger *pLog = GoLogger::Instance(); \
    if (pLog) { \
        if (pLog->LogLevel() == None || pLog->LogLevel() < Warn) \
        break; \
        Print(__FILE__, __func__, __LINE__, Warn, MakeMessage msg); \
    } \
} while (0)

#define LogError(msg)   do { \
    using namespace NEC::Log; \
    GoLogger *pLog = GoLogger::Instance(); \
    if (pLog) { \
        if (pLog->LogLevel() == None || pLog->LogLevel() < Error) \
        break; \
        Print(__FILE__, __func__, __LINE__, Error, MakeMessage msg); \
    } \
} while (0)

#define LogFatal(msg)   do { \
    using namespace NEC::Log; \
    GoLogger *pLog = GoLogger::Instance(); \
    if (pLog) { \
        if (pLog->LogLevel() == None || pLog->LogLevel() < Fatal) \
        break; \
        Print(__FILE__, __func__, __LINE__, Fatal, MakeMessage msg); \
    } \
} while (0)
#endif
