#include <iostream>
#include <cstdio>

#include <json/json.h>

#include "httplib.h"

#include "oj_model.hpp"
#include "oj_view.hpp"
#include "compile.hpp"
#include "tools.hpp"

int main()
{
    using namespace httplib;
    OjModel model; // 创建一个在oj_model.hpp中封装的OjModel类型的对象，该对象实例化时会调用构造函数，构造函数会调用OjModel类中的Load函数
    Server svr; // 初始化httplib库的Server对象

    //2.提供三个接口， 分别处理三个请求
    /*
    * 2.1 获取整个试题列表， get
    * Get函数原型：Server &Get(const char *pattern, Handler handler)
    * Get函数中的handler回调函数我们用Lambda表达式的形式来写
    * [ 给Lambda表达式传递的参数 ]( Lambda表达式的参数 ){
    * 	Lambda表达式的实现
    * }
    */
    svr.Get("/all_questions", [&model](const Request& req, Response& resp){
            /*
 	    * 如果req中的url和pattern("/all_question")匹配，就会进行下边Lambda表达式的实现
 	    */ 
	    //1.返回试题列表
            std::vector<Question> questions; // 出参
            model.GetAllQuestion(&questions); // 调用完返回vector<Question>类型的questions
            
	    std::string html; // 出参
            OjView::DrawAllQuestions(questions, &html); // 调用完返回string类型的html对象

	    /*
            * void set_content(const std::string& s, const char* content_type);
            */ 
            resp.set_content(html, "text/html");
            });

    //2.2 获取单个试题
    //  如果标识浏览器想要获取的是哪一个试题？？
    //  正则表达式
    //  浏览器提交的资源路径是  /question/[试题编号] 
    //                          /question/[\d+[0, 正无穷]]
    //  \d : 表示一个数字， [0,9]
    //  \d+ : 表示多位数字 
    /*
    * matches[0] = question ; matches[1] = id
    */ 
    svr.Get(R"(/question/(\d+))", [&model](const Request& req, Response& resp){
            //1.获取url当中关于试题的数字 & 获取单个试题的信息
            //std::cout << req.matches[0] << " "<< req.matches[1] << std::endl;
            /*
            * http请求格式
            *  首行：方法 url 版本
            *  Header：请求的属性，冒号分割的键值对；每组属性之前用\n分隔；遇到空行表示Header部分结束
            *  Body：空行后面的内容都是Body。如果Body存在，则在Header中的Content-Length属性来标识Body长度
            
            * 下边两行打印的是http请求
	    */ 
            std::cout << req.version << " " << req.method << std::endl;
            std::cout << req.path <<  std::endl;
            Question ques; // 出参
            model.GetOneQuestion(req.matches[1].str(), &ques); // 调用完返回Question类型的对象ques，拿到一个单个试题

            //2.渲染模版的html文件
            std::string html;
            OjView::DrawOneQuestion(ques, &html);
            resp.set_content(html, "text/html");
            });

    //2.3 编译运行
    //  目前还没有区分到底是提交的是哪一个试题
    //  url : /compile/[num]
    svr.Post(R"(/compile/(\d+))", [&model](const Request& req, Response& resp){
            //1.获取试题编号 & 获取试题内容
            Question ques;
            model.GetOneQuestion(req.matches[1].str(), &ques);
            //ques.tail_cpp_ ==> main函数调用+测试用例

            //post 方法在提交代码的时候， 是经过encode的， 要想正常获取浏览器提交的内容， 需要进行decode， 使用decode完成的代码和tail.cpp进行合并， 产生待编译的源码
            //key: value
            //    code: xcsnasucnbjasbcsau
            //std::cout << UrlUtil::UrlDecode(req.body) << std::endl;


            std::unordered_map<std::string, std::string> body_kv;
            UrlUtil::PraseBody(req.body, &body_kv);

            std::string user_code = body_kv["code"];
            //2.构造json对象， 交给编译运行模块
            // Json(命名空间)::Value(类名) req_json(对象名)
            Json::Value req_json; // 入参
            req_json["code"] = user_code + ques.tail_cpp_;
            req_json["stdin"] = "";

            std::cout << req_json["code"].asString() << std::endl;

            Json::Value resp_json; // 出参
            Compiler::CompileAndRun(req_json, &resp_json);

            //获取的返回结果都在 resp_json当中
            std::string err_no = resp_json["errorno"].asString();
            std::string case_result = resp_json["stdout"].asString();
            std::string reason = resp_json["reason"].asString();

            std::string html;
            OjView::DrawCaseResult(err_no, case_result, reason, &html);

            resp.set_content(html, "text/html");
            });

    LOG(INFO, "listn_port") << ": 17878" << std::endl;
    svr.set_base_dir("./www"); // 作为当前服务器的逻辑根目录
    svr.listen("0.0.0.0", 17878);
    return 0;
}
