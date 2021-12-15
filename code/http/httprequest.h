#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <regex>
#include <errno.h>     
#include <mysql/mysql.h>  //mysql

#include "../buffer/buffer.h"
#include "../log/log.h"
#include "../pool/sqlconnpool.h"
#include "../pool/sqlconnRAII.h"

class HttpRequest {
public:
    //枚举解析状态请求行0、解析头部字段1、解析请求正文2、解析结束3
    enum PARSE_STATE {
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH,        
    };
    //枚举HTTP协议code:没有请求0、获得请求1、错误请求2、没资源3、禁止的请求4、文件请求5、内部错误6、关闭连接7
    enum HTTP_CODE {
        NO_REQUEST = 0,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURSE,
        FORBIDDENT_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION,
    };
    //http请求对象初始化
    HttpRequest() { Init(); }
    ~HttpRequest() = default;

    void Init();
    //解析缓冲区
    bool parse(Buffer& buff);

    std::string path() const;
    std::string& path();
    //http请求类型
    std::string method() const;
    //http版本
    std::string version() const;
    
    std::string GetPost(const std::string& key) const;
    std::string GetPost(const char* key) const;

    //是否保持连接
    bool IsKeepAlive() const;

    /* 
    todo 
    void HttpConn::ParseFormData() {}
    void HttpConn::ParseJson() {}
    */

private:
    //解析请求行
    bool ParseRequestLine_(const std::string& line);
    //解析头部字段
    void ParseHeader_(const std::string& line);
    //解析请求正文
    void ParseBody_(const std::string& line);

    //解析
    void ParsePath_();
    void ParsePost_();
    void ParseFromUrlencoded_();
    //数据库用户登录确认-用户名、密码、已经登录
    static bool UserVerify(const std::string& name, const std::string& pwd, bool isLogin);

    PARSE_STATE state_;
    //HTTP报文协议字段：请求方法、路径URL、协议版本、请求正文
    std::string method_, path_, version_, body_;
    //HTTP报文头部字段
    std::unordered_map<std::string, std::string> header_;
    //
    std::unordered_map<std::string, std::string> post_;

    static const std::unordered_set<std::string> DEFAULT_HTML;
    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;
    static int ConverHex(char ch);
};


#endif //HTTP_REQUEST_H
