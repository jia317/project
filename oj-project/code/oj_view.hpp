#pragma once
#include <iostream>
#include <vector>

#include <ctemplate/template.h>

#include "oj_model.hpp"

/*
 * OjView这个类里封装了渲染html页面的函数实现接口
 */
class OjView
{
    public:
	/*
 	* questions是我们调用oj_model.hpp中GetAllQuestion的返回值，即一个包含所有试题信息的vector
 	* html是我们渲染html页面的url，是一个出参
 	*/ 
        static void DrawAllQuestions(std::vector<Question>& questions, std::string* html)
        {
            //1. 创建template字典
            /*
 	    * ctemplate是一个命名空间
 	    * TemplateDictionary是TemplateDictionary类的构造函数，用来创建一个字典对象
 	    * 这里的构造函数的参数，即"all_questions"是没有意义的
 	    */ 
            ctemplate::TemplateDictionary dict("all_questions"); // 创建一个根字典

            //2.遍历vector， 遍历vector就相当于遍历多个试题， 每一个试题构造一个子字典
            for(const auto& ques : questions)
            {
                /*
 		* 在一个字典里创建子字典的函数原型  
 		* TemplateDictionary* AddSectionDictionary(const TemplateString section_name);
 		*   section_name：预定义html当中的标记名
                */
		ctemplate::TemplateDictionary* sub_dict = dict.AddSectionDictionary("question");
                /*
 		 * 填充字典的函数原型：
		 * void SetValue(const TemplateString variable, const TemplateString value);
                 *   variable: 指定的是在预定义的html当中的变量名称
                 *   value: 替换的值
                 * */
                sub_dict->SetValue("id", ques.id_);
                sub_dict->SetValue("id", ques.id_);
                sub_dict->SetValue("title", ques.title_);
                sub_dict->SetValue("star", ques.star_);
            }

            //3.填充html模板
            /*
 	    * 填充html模板的函数原型： 
 	    * static Template *GetTemplate(const TemplateString& filename,Strip strip);
 	    *   filename：传入预定义的html页面的路径
 	    *   strip:
 	    *         DO_NOT_STRIP：逐字逐句输出到模板文件中去
 	    *         STRIP_BLACK_LINES：删除空行之后再输出到模板文件中去
 	    *         STROIP_WHITESPACE:删除空行和每一行的首尾空白字符
 	    */ 
	    // 将我们保存在/home/test/oj-project/bin/template中预定义的html页面源码文件从磁盘拿回来
            ctemplate::Template* tl = ctemplate::Template::GetTemplate("./template/all_questions.html", ctemplate::DO_NOT_STRIP); 
            //渲染
            tl->Expand(html, &dict); // html是一个出参
        }

        static void DrawOneQuestion(const Question& ques, std::string* html)
        {
            ctemplate::TemplateDictionary dict("question");
            dict.SetValue("id", ques.id_);
            dict.SetValue("title", ques.title_);
            dict.SetValue("star", ques.star_);
            dict.SetValue("desc", ques.desc_);
            dict.SetValue("id", ques.id_);
            dict.SetValue("code", ques.header_cpp_);
            ctemplate::Template* tl = ctemplate::Template::GetTemplate("./template/question.html", ctemplate::DO_NOT_STRIP);
            //渲染
            tl->Expand(html, &dict);
        }


        static void DrawCaseResult(const std::string& err_no, const std::string& q_result, const std::string& reason, std::string* html)
        {
            ctemplate::TemplateDictionary dict("question");
            dict.SetValue("errno", err_no);
            dict.SetValue("compile_result", reason);
            dict.SetValue("case_result", q_result);

            ctemplate::Template* tl = ctemplate::Template::GetTemplate("./template/case_result.html", ctemplate::DO_NOT_STRIP);
            //渲染
            tl->Expand(html, &dict);
        }
};
