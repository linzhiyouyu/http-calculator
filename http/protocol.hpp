#ifndef __PROTOCOL_HPP__
#define __PROTOCOL_HPP__
#include"utils.hpp"
#include<regex>
class HttpRequest {
    public:
        std::string _method;      //请求方法
        std::string _path;        //资源路径
        std::string _version;     //协议版本
        std::string _body;        //请求正文
        std::smatch _matches;     //资源路径的正则提取数据
        std::unordered_map<std::string, std::string> _headers;  //头部字段
        std::unordered_map<std::string, std::string> _params;   //查询字符串
    public:
        HttpRequest():_version("HTTP/1.1") {}
        void ReSet() {
            _method.clear();
            _path.clear();
            _version = "HTTP/1.1";
            _body.clear();
            std::smatch match;
            _matches.swap(match);
            _headers.clear();
            _params.clear();
        }
        //插入头部字段
        void SetHeader(const std::string &key, const std::string &val) {
            _headers.insert(std::make_pair(key, val));
        }
        //判断是否存在指定头部字段
        bool HasHeader(const std::string &key) const {
            auto it = _headers.find(key);
            if (it == _headers.end()) {
                return false;
            }
            return true;
        }
        //获取指定头部字段的值
        std::string GetHeader(const std::string &key) const {
            auto it = _headers.find(key);
            if (it == _headers.end()) {
                return "";
            }
            return it->second;
        }
        //插入查询字符串
        void SetParam(const std::string &key, const std::string &val) {
            _params.insert(std::make_pair(key, val));
        }
        //判断是否有某个指定的查询字符串
        bool HasParam(const std::string &key) const {
            auto it = _params.find(key);
            if (it == _params.end()) {
                return false;
            }
            return true;
        }
        //获取指定的查询字符串
        std::string GetParam(const std::string &key) const {
            auto it = _params.find(key);
            if (it == _params.end()) {
                return "";
            }
            return it->second;
        }
        //获取正文长度
        size_t ContentLength() const {
            // Content-Length: 1234\r\n
            bool ret = HasHeader("Content-Length");
            if (ret == false) {
                return 0;
            }
            std::string clen = GetHeader("Content-Length");
            return std::stol(clen);
        }
        //判断是否是短链接
        bool Close() const {
            // 没有Connection字段，或者有Connection但是值是close，则都是短链接，否则就是长连接
            if (HasHeader("Connection") == true && GetHeader("Connection") == "keep-alive") {
                return false;
            }
            return true;
        }
};

class HttpResponse {
    public:
        int _statu;
        bool _redirect_flag;
        std::string _body;
        std::string _redirect_url;
        std::unordered_map<std::string, std::string> _headers;
    public:
        HttpResponse():_redirect_flag(false), _statu(200) {}
        HttpResponse(int statu):_redirect_flag(false), _statu(statu) {} 
        void ReSet() {
            _statu = 200;
            _redirect_flag = false;
            _body.clear();
            _redirect_url.clear();
            _headers.clear();
        }
        //插入头部字段
        void SetHeader(const std::string &key, const std::string &val) {
            _headers.insert(std::make_pair(key, val));
        }
        //判断是否存在指定头部字段
        bool HasHeader(const std::string &key) {
            auto it = _headers.find(key);
            if (it == _headers.end()) {
                return false;
            }
            return true;
        }
        //获取指定头部字段的值
        std::string GetHeader(const std::string &key) {
            auto it = _headers.find(key);
            if (it == _headers.end()) {
                return "";
            }
            return it->second;
        }
        void SetContent(const std::string &body,  const std::string &type = "text/html") {
            _body = body;
            SetHeader("Content-Type", type);
        }
        void SetRedirect(const std::string &url, int statu = 302) {
            _statu = statu;
            _redirect_flag = true;
            _redirect_url = url;
        }
        //判断是否是短链接
        bool Close() {
            // 没有Connection字段，或者有Connection但是值是close，则都是短链接，否则就是长连接
            if (HasHeader("Connection") == true && GetHeader("Connection") == "keep-alive") {
                return false;
            }
            return true;
        }
};

typedef enum {
    RECV_HTTP_ERROR,
    RECV_HTTP_LINE,
    RECV_HTTP_HEAD,
    RECV_HTTP_BODY,
    RECV_HTTP_OVER
}HttpRecvStatu;

#define MAX_LINE 8192
class HttpContext {
    private:
        int _resp_statu; //响应状态码
        HttpRecvStatu _recv_statu; //当前接收及解析的阶段状态
        HttpRequest _request;  //已经解析得到的请求信息
    private:
        bool ParseHttpLine(const std::string &line) {
            std::smatch matches;
            std::regex e("(GET|HEAD|POST|PUT|DELETE) ([^?]*)(?:\\?(.*))? (HTTP/1\\.[01])(?:\n|\r\n)?", std::regex::icase);
            bool ret = std::regex_match(line, matches, e);
            if (ret == false) {
                _recv_statu = RECV_HTTP_ERROR;
                _resp_statu = 400;//BAD REQUEST
                return false;
            }
            _request._method = matches[1];
            std::transform(_request._method.begin(), _request._method.end(), _request._method.begin(), ::toupper);
            //资源路径的获取，需要进行URL解码操作，但是不需要+转空格
            _request._path = Utils::urldecode(matches[2], false);
            //协议版本的获取
            _request._version = matches[4];
            //查询字符串的获取与处理
            std::vector<std::string> query_string_arry;
            std::string query_string = matches[3];
            //查询字符串的格式 key=val&key=val....., 先以 & 符号进行分割，得到各个字串
            Utils::split(query_string, "&", &query_string_arry);
            //针对各个字串，以 = 符号进行分割，得到key 和val， 得到之后也需要进行URL解码
            for (auto &str : query_string_arry) {
                size_t pos = str.find("=");
                if (pos == std::string::npos) {
                    _recv_statu = RECV_HTTP_ERROR;
                    _resp_statu = 400;//BAD REQUEST
                    return false;
                }
                std::string key = Utils::urldecode(str.substr(0, pos), true); 
                std::string val = Utils::urldecode(str.substr(pos + 1), true);
                _request.SetParam(key, val);
            }
            return true;
        }
        bool RecvHttpLine(Buffer *buf) {
            if (_recv_statu != RECV_HTTP_LINE) return false;
            //1. 获取一行数据，带有末尾的换行 
            std::string line = buf->get_line_and_pop();
            //2. 需要考虑的一些要素：缓冲区中的数据不足一行， 获取的一行数据超大
            if (line.size() == 0) {
                //缓冲区中的数据不足一行，则需要判断缓冲区的可读数据长度，如果很长了都不足一行，这是有问题的
                if (buf->readable_size() > MAX_LINE) {
                    _recv_statu = RECV_HTTP_ERROR;
                    _resp_statu = 414;//URI TOO LONG
                    return false;
                }
                //缓冲区中数据不足一行，但是也不多，就等等新数据的到来
                return true;
            }
            if (line.size() > MAX_LINE) {
                _recv_statu = RECV_HTTP_ERROR;
                _resp_statu = 414;//URI TOO LONG
                return false;
            }
            bool ret = ParseHttpLine(line);
            if (ret == false) {
                return false;
            }
            //首行处理完毕，进入头部获取阶段
            _recv_statu = RECV_HTTP_HEAD;
            return true;
        }
        bool RecvHttpHead(Buffer *buf) {
            if (_recv_statu != RECV_HTTP_HEAD) return false;
            //一行一行取出数据，直到遇到空行为止， 头部的格式 key: val\r\nkey: val\r\n....
            while(1){
                std::string line = buf->get_line_and_pop();
                //2. 需要考虑的一些要素：缓冲区中的数据不足一行， 获取的一行数据超大
                if (line.size() == 0) {
                    //缓冲区中的数据不足一行，则需要判断缓冲区的可读数据长度，如果很长了都不足一行，这是有问题的
                    if (buf->readable_size() > MAX_LINE) {
                        _recv_statu = RECV_HTTP_ERROR;
                        _resp_statu = 414;//URI TOO LONG
                        return false;
                    }
                    //缓冲区中数据不足一行，但是也不多，就等等新数据的到来
                    return true;
                }
                if (line.size() > MAX_LINE) {
                    _recv_statu = RECV_HTTP_ERROR;
                    _resp_statu = 414;//URI TOO LONG
                    return false;
                }
                if (line == "\n" || line == "\r\n") {
                    break;
                }
                bool ret = ParseHttpHead(line);
                if (ret == false) {
                    return false;
                }
            }
            //头部处理完毕，进入正文获取阶段
            _recv_statu = RECV_HTTP_BODY;
            return true;
        }
        bool ParseHttpHead(std::string &line) {
            if (line.back() == '\n') line.pop_back();
            if (line.back() == '\r') line.pop_back();
            size_t pos = line.find(": ");
            if (pos == std::string::npos) {
                _recv_statu = RECV_HTTP_ERROR;
                _resp_statu = 400;
                return false;
            }
            std::string key = line.substr(0, pos);  
            std::string val = line.substr(pos + 2);
            _request.SetHeader(key, val);
            return true;
        }
        bool RecvHttpBody(Buffer *buf) {
            if (_recv_statu != RECV_HTTP_BODY) return false;
            //1. 获取正文长度
            size_t content_length = _request.ContentLength();
            if (content_length == 0) {
                //没有正文，则请求接收解析完毕
                _recv_statu = RECV_HTTP_OVER;
                return true;
            }
            //2. 当前已经接收了多少正文,其实就是往  _request._body 中放了多少数据了
            size_t real_len = content_length - _request._body.size();//实际还需要接收的正文长度
            //3. 接收正文放到body中，但是也要考虑当前缓冲区中的数据，是否是全部的正文
            //  3.1 缓冲区中数据，包含了当前请求的所有正文，则取出所需的数据
            if (buf->readable_size() >= real_len) {
                _request._body.append(buf->read_start_position(), real_len);
                buf->move_read_offset(real_len);
                _recv_statu = RECV_HTTP_OVER;
                return true;
            }
            //  3.2 缓冲区中数据，无法满足当前正文的需要，数据不足，取出数据，然后等待新数据到来
            _request._body.append(buf->read_start_position(), buf->readable_size());
            buf->move_read_offset(buf->readable_size());
            return true;
        }
    public:
        HttpContext():_resp_statu(200), _recv_statu(RECV_HTTP_LINE) {}
        void ReSet() {
            _resp_statu = 200;
            _recv_statu = RECV_HTTP_LINE;
            _request.ReSet();
        }
        int RespStatu() { return _resp_statu; }
        HttpRecvStatu RecvStatu() { return _recv_statu; }
        HttpRequest &Request() { return _request; }
        //接收并解析HTTP请求
        void RecvHttpRequest(Buffer *buf) {
            switch(_recv_statu) {
                case RECV_HTTP_LINE: RecvHttpLine(buf);
                case RECV_HTTP_HEAD: RecvHttpHead(buf);
                case RECV_HTTP_BODY: RecvHttpBody(buf);
            }
            return;
        }
};

#endif