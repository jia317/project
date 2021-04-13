#pragma once


#include <iostream>
#include <string>
#include <unordered_map>
#include <fstream>
//#include <boost/algorithm/string.hpp>
#include <vector>

#include "tools.hpp"

/*将文件属性保存在struct Question
 *
 */
struct Question
{
    std::string id_; //题目id
    std::string title_; //题目标题
    std::string star_; //题目的难易程度
    std::string path_; //题目路径

    std::string desc_; //题目的描述
    std::string header_cpp_; //题目预定义的头
    std::string tail_cpp_; //题目的尾， 包含测试用例以及调用逻辑
};


class OjModel
{
    public:
	// 创建一个Ojmodel类型的对象，调用构造函数，将磁盘中oj_config.cfg文件中的内容读回来，保存在file文件中
        OjModel()
        {
            Load("./oj_data/oj_config.cfg"); // 这是我们自己在类中定义的一个函数，传递参数是试题配置文件的文件名
	    /*
 	    * 在这个函数中，我们完成的事情：
 	    * 1. 打开oj_config_cfg这个文件
 	    * 2. 读取一行内容进行分割，将一个试题的属性，即题号、题目名称、难易程度、试题路径保存在一个vector中
 	    * 3. 再用vector中的值填充struct Question变量
 	    * 4. 
 	    *
 	    *
 	    */
        }

        ~OjModel()
        {

        }

        //从文件当中获取题目信息
        bool Load(const std::string& filename) // 传递了要打开的文件名
        {
            // C++：fstream
            std::ifstream file(filename.c_str()); // 打开一个，文件名类型是string，用c_str()转换成字符串
            if(!file.is_open())
            {
                std::cout << "config file open failed" << std::endl;
                return false;
            }

            //1.打开文件成功的情况
            //  1.1 从文件当中获取每一个行信息
            //        1.1.1 对于每一行信息， 还需要获取题号， 题目名称， 题目难易程度， 题目路径
            //        1.1.2 保存在结构体当中
            //
            //2.把多个question， 组织在map当中
            
            std::string line;
            while(std::getline(file, line)) // 每次读一行，直到将oj_config.cfg文件中内容读完结束
            {
                //boost::spilt
                std::vector<std::string> vec;
                StringUtil::Split(line, "\t", &vec);
                //is_any_of:支持多个字符作为分割符
                //token_compress_off: 是否将多个分割字符看作是一个
                //boost::split(vec, line, boost::is_any_of(" "), boost::token_compress_off);

                Question ques;
                ques.id_ = vec[0];
                ques.title_ = vec[1];
                ques.star_ = vec[2];
                ques.path_ = vec[3];

                std::string dir = vec[3]; // dir = ./oj_data/1
		/*
 		* ReadFile函数在tools.hpp文件的FileUtil类中是一个static修饰的函数，所以可以通过类名::成员函数名的方式来调用 
 		* dir+"/desc.txt" = ./oj_data/1/desc.cpp 
 		* 将./oj_data/1/desc.cpp文件中读到的信息，保存在struct Question类型的变量ques中的desc_变量中
 		* 下边两个是同样的逻辑
 		*/
                FileUtil::ReadFile(dir + "/desc.txt", &ques.desc_);
                FileUtil::ReadFile(dir + "/header.cpp", &ques.header_cpp_);
                FileUtil::ReadFile(dir + "/tail.cpp", &ques.tail_cpp_);
		
		// ques结构体变量的id作为ques_map_的key值，ques结构体变量作为value插入到ordered_map类型的ques_map_变量中
                ques_map_[ques.id_] = ques;
            }

            file.close();
            return true;
        }

        /*
        * 提供给上层调用这一个获取所有试题的接口
        * questions是一个保存了Question类型数据的vector的出参
        */
        bool GetAllQuestion(std::vector<Question>* questions)
        {
            //1.遍历unordered_map类型的变量ques_map_， 将试题信息返回给调用者
            //对于每一个试题， 都是一个struct Question对象
            for(const auto& kv : ques_map_)
            {
                questions->push_back(kv.second);
            }

            //2.排序
            std::sort(questions->begin(), questions->end(), [](const Question& l, const Question& r){
                    //比较Question当中的题目编号， 按照升序规则
                    return std::stoi(l.id_) < std::stoi(r.id_);
                    });
            return true;
        }

        //提供给上层调用者一个获取单个试题的接口
        /*
         *  id: 输入条件，查找题目的ID
         *  ques : 输出参数， 将查到的结果返回给调用者
         * */
        bool GetOneQuestion(const std::string& id, Question* ques)
        {
            auto it = ques_map_.find(id);
            if(it == ques_map_.end())
            {
                return false;
            }
            *ques = it->second; // 将ques_map_中value的值，即struct Question类型的变量赋值给ques这个出参
            return true;
        }
    private:
	/* 将多个试题保存在unordered_map中
 	* <key, value> -> <题号， struct Question类型的变量>
 	*/ 
        std::unordered_map<std::string, Question> ques_map_;
};
