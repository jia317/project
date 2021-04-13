#pragma once
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <iostream>
#include <string>
#include <atomic>

#include <json/json.h>
#include "tools.hpp"


enum ErrorNo
{
    OK = 0,
    PRAM_ERROR,
    INTERNAL_ERROR,
    COMPILE_ERROR,
    RUN_ERROR
};

class Compiler
{
    public:
        /*
         *  Json::Value Req : 请求的json
         *      {"code":"xxxx", "stdin":"xxxxx"}
         *  Json::Value* Resp: 出参， 返回给调用者
         *      {"errorno":"xx", "reason":"xxxx"}
         * */
        static void CompileAndRun(Json::Value Req, Json::Value* Resp)
        {
            //1.参数是否是错误的， json串当中的code是否为空
            if(Req["code"].empty())
            {
                (*Resp)["errorno"] = PRAM_ERROR;
                (*Resp)["reason"] = "Pram error";

                return;
            }
           
	    //2.将代码写到文件当中去
            std::string code = Req["code"].asString(); // 拼接好的源码
            //文件命名约定的时候， tmp_时间戳_src.cpp
            //用来区分不同的请求
            std::string file_nameheader = WriteTmpFile(code);
            if(file_nameheader == "") // 没有产生源码文件
            {
                (*Resp)["errorno"] = INTERNAL_ERROR;
                (*Resp)["reason"] = "write file failed";

                return;
            }
           
	    //3.编译
            if(!Compile(file_nameheader)) // 如果编译失败走以下逻辑
            {
                (*Resp)["errorno"] = COMPILE_ERROR;
                std::string reason; // 出参
                FileUtil::ReadFile(CompileErrorPath(file_nameheader), &reason); // 将编译错误文件中的内容保存到reason中，再将reason中的内容用来构造Resp
                (*Resp)["reason"] = reason;
                return;
            }

            //4.运行
            int ret = Run(file_nameheader);
            /*
 	     *  -1：子进程创建失败   
 	     *  -2：重定向标准输出和标准错误的时候打开文件失败
 	     *  大于0：
 	     *  等于0：Run正常结束
             */ 
            if(ret != 0)
            {
                (*Resp)["errorno"] = RUN_ERROR;
                (*Resp)["reason"] = "program exit by sig: " + std::to_string(ret);
                return;
            }

            //5.构造响应
            
            (*Resp)["errorno"] = OK;
            (*Resp)["reason"] = "Compile and Run ok";

            std::string stdout_str;
            FileUtil::ReadFile(StdoutPath(file_nameheader), &stdout_str);
            (*Resp)["stdout"] = stdout_str;

            std::string stderr_str;
            FileUtil::ReadFile(StderrPath(file_nameheader), &stderr_str);
            (*Resp)["stderr"] = stderr_str;


            //6.删除临时文件
            //Clean(file_nameheader);
            return;
        }

    private:
	// 处理完一个请求就删除其相关所有临时文件，因为这些文件会占用磁盘大小
        static void Clean(const std::string& filename)
        {
	    /*
 	     * 系统调用接口 
 	     * #include <unistd.h>
 	     * int unlink(const char *pathname);
 	     */ 
            unlink(SrcPath(filename).c_str());
            unlink(CompileErrorPath(filename).c_str());
            unlink(ExePath(filename).c_str());
            unlink(StdoutPath(filename).c_str());
            unlink(StderrPath(filename).c_str());
        }

        static int Run(const std::string& file_name)
        {
            //1.创建子进程
            //2.子进程进行进程程序替换
            
            int pid = fork();
            if(pid < 0) // 创建子进程失败
            {
                return -1;
            }
            else if(pid > 0) // 父进程逻辑：等待子进程退出
            {
                //father
                int status = 0; // 出参 其实只用到后边2个字节，高八位是退出状态，低八位的第八个比特位是coredump标志位，后面是终止信号 
                waitpid(pid, &status, 0); // 第二个参数可以拿到

                // 将终止信号返还给调用者
                return status & 0x7f; // 保留status的低七位(终止信号)
            }
            else
            {
                /* 
 		 * 时间限制
 		 * 防止用户提交代码运行时间过长，比如代码中写了一个死循环
 		 */
		//注册一个定时器， alarm
                //[限制运行时间]
                alarm(1);
                //child
                //进程程序替换， 替换编译创建出来的可执行程序
                
		/*
 		 * [限制内存] 系统调用函数 
 		 * #include <sys/resource.h>
 		 * setrlimit(int resource, const struct rlimit *rlim);
 		 * resource：
 		 *          RLIMIT_AS：进程最大虚拟内存空间
 		 *          RLIMIT_CORE：内存转储文件的最大大小
 		 *          RLIMIT_CPU：进程使用CPU最大时间
 		 *          RLIMIT_DATA：进程数据段的最大大小
 		 *          RLIMIT_STACK：进程堆栈大小
 		 * rlim:
 		 *
 		 * struct rlimit{
 		 * 	rlim_t rlim_cur; // 软限制
 		 * 	rlim_t rlim_max; // 硬限制
 		 * };
 		 */ 
                struct rlimit rlim;
                rlim.rlim_cur = 30000 * 1024;   //3wKB
                rlim.rlim_max = RLIM_INFINITY;
                setrlimit(RLIMIT_AS, &rlim);

                int stdout_fd = open(StdoutPath(file_name).c_str(), O_CREAT | O_WRONLY, 0666);
                if(stdout_fd < 0)
                {
                    return -2;
                }
                dup2(stdout_fd, 1); // 将标准输出1中的内容重定向到stdout_fd指向的StdoutPath(file_name)文件

                int stderr_fd = open(StderrPath(file_name).c_str(), O_CREAT | O_WRONLY, 0666);
                if(stderr_fd < 0)
                {
                    return -2;
                }
                dup2(stderr_fd, 2);
                
                
                execl(ExePath(file_name).c_str(), ExePath(file_name).c_str(), NULL);
                exit(0);
            }

            return 0;
        }

        static bool Compile(const std::string& file_name)
        {
            //1.创建子进程
            //    1.1 父进程进行进程等待
            //    1.2 子进程进行进程程序替换
            
            int pid = fork();
            if(pid > 0)
            {
                //father
                waitpid(pid, NULL, 0);
            }
            else if (pid == 0)
            {
                //child
                //进程程序替换--》 g++ SrcPath(filename) -o ExePath(filename) "-std=c++11"
                int fd = open(CompileErrorPath(file_name).c_str(), O_CREAT | O_WRONLY, 0666);
                if(fd < 0)
                {
                    return false;
                }

                //将标准错误（2）重定向为 fd, 标准错误的输出， 就会输出在文件当中
                dup2(fd, 2);

                execlp("g++", "g++", SrcPath(file_name).c_str(), "-o", ExePath(file_name).c_str(), "-std=c++11", "-D", "CompileOnline", NULL);

                close(fd);
                //如果替换失败了， 就直接让子进程退出了，如果替换成功了， 不会走该逻辑了
                exit(0);
            }
            else
            {
                return false;
            }

            //如果说编译成功了， 在tmp_file这个文件夹下， 一定会产生一个可执行程序， 如果
            //当前代码走到这里，判断有该可执行程序， 则我们认为g++执行成功了， 否则， 认为执行失败
            
            struct stat st;
            int ret = stat(ExePath(file_name).c_str(), &st);
            if(ret < 0)
            {
                return false;
            }

            return true;
        }
        
	static std::string StdoutPath(const std::string& filename)
        {
            return "./tmp_file/" + filename + ".stdout";
        }

        static std::string StderrPath(const std::string& filename)
        {
            return "./tmp_file/" + filename + ".stderr";
        }

        static std::string CompileErrorPath(const std::string& filename)
        {
            return "./tmp_file/" + filename + ".Compilerr";
        }

        static std::string ExePath(const std::string& filename)
        {
            return "./tmp_file/" + filename + ".executable";
        }
        
	// 构造源码文件名
        static std::string SrcPath(const std::string& filename)
        {
            //./tmp_file/ + tmp_" + std::to_string(TimeUtil::GetTimeStampMs()) + "." + std::to_string(id) + .cpp
            return "./tmp_file/" + filename + ".cpp";
        }
	
	// 用来将拼接好的源码写入文件
        static std::string WriteTmpFile(const std::string& code)
        {
            //1.组织文件名称， 区分源码文件，以及后面生成的可执行程序文件
            /*
 	     * C++中提供的atomic_uint 
 	     */ 
            static std::atomic_uint id(0); // 初始化这个变量id为0
	    /*
 	     *TimeUtil::GetTimeStampMs()的返回值是int64_t要强转成string类型 
 	     */ 
            std::string tmp_filename = "tmp_" + std::to_string(TimeUtil::GetTimeStampMs()) + "." + std::to_string(id);
            id++;
            //2.code 写到文件当中去
            //tmp_stamp.cpp
            FileUtil::WriteFile(SrcPath(tmp_filename), code);
            return tmp_filename;
        }
};
