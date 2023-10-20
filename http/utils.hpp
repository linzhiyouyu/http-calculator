#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include<string>
#include<iostream>
#include<vector>
#include"../source/buffer.hpp"
#include"../source/log.hpp"
#include<unordered_map>
#include<unistd.h>
#include<cstdio>
#include<sys/stat.h>
#include<mysql/mysql.h>
class Utils
{
public:
    static size_t split(const std::string& src, const std::string& sep, std::vector<std::string>* out)
    {
        int offset = 0;
        while(offset < src.size()) {
            int pos = src.find(sep, offset);
            if(pos == std::string::npos) {
                out->push_back(src.substr(offset));
                break;
            }
            if(pos == offset) {
                offset += sep.size();
                continue;
            }
            out->push_back(src.substr(offset, pos - offset));
            offset = pos + sep.size();
        }
        return out->size();
    }
    static bool read_file(const std::string& filename, std::string* out) {
        //使用c接口
        FILE* fp = ::fopen(filename.c_str(), "rb");
        if(fp == nullptr) {
            printf("open file %s failed\n", filename.c_str());
            return false;
        }
        //获取文件大小
        ::fseek(fp, 0, SEEK_END);
        int64_t length = ::ftell(fp);
        ::fseek(fp, 0, SEEK_SET);
        //读取文件
        out->resize(length);
        ::fread(&(*out)[0], 1, length, fp);
        ::fclose(fp);
        return true;
    }
    static bool write_file(const std::string& filename, const std::string& content)
    {
        //使用c接口
        FILE* fp = ::fopen(filename.c_str(), "wb");
        if(fp == nullptr) {
            printf("open file %s failed\n", filename.c_str());
            return false;
        }
        //写入文件
        ::fwrite(content.c_str(), 1, content.size(), fp);
        ::fclose(fp);
        return true;
    }
    static std::string urlencode(const std::string& url, bool convert_space_to_plus) {
        std::string out;
        std::for_each(begin(url), end(url), [&](const char& c) {
            if(isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                out += c;
            } else if (c == ' ' && convert_space_to_plus) {
                out += '+';
            } else {
                char buf[4];
                snprintf(buf, sizeof(buf), "%%%02X", c);
                out += buf;
            }
        });
        return out;
    }
    static char hextoi(const char& c) {
        if(c >= '0' && c <= '9') {
            return c - '0';
        } else if (c >= 'a' && c <= 'f') {
            return c - 'a' + 10;
        } else if (c >= 'A' && c <= 'F') {
            return c - 'A' + 10;
        }
        return 0;
    }
    static std::string urldecode(const std::string& url, bool convert_plus_to_space) {
        std::string res;
        for(int i = 0; i < url.size(); ++i) {
            if(url[i] == '%' && i + 2 < url.size()) {
                char v1 = hextoi(url[i + 1]);
                char v2 = hextoi(url[i + 2]);
                char c = v1 * 16 + v2;
                res += c;
                i += 2;
                continue;
            }
            res += url[i];
        }
        if(convert_plus_to_space) {
            std::replace(res.begin(), res.end(), '+', ' ');
        }
        return res;
    }
    static std::string status_to_desc(int status) {
        std::unordered_map<int, std::string> m = {
            {200, "OK"},
            {301, "Moved Permanently"},
            {302, "Found"},
            {304, "Not Modified"},
            {400, "Bad Request"},
            {401, "Unauthorized"},
            {403, "Forbidden"},
            {404, "Not Found"},
            {405, "Method Not Allowed"},
            {413, "Request Entity Too Large"},
            {414, "Request-URI Too Long"},
            {500, "Internal Server Error"},
            {501, "Not Implemented"},
            {503, "Service Unavailable"},
        };
        return m[status];
    }
    static std::string extension_mime(const std::string& filename) {
        std::unordered_map<std::string, std::string> m = {
            {".html", "text/html"},
            {".xml", "text/xml"},
            {".xhtml", "application/xhtml+xml"},
            {".txt", "text/plain"},
            {".rtf", "application/rtf"},
            {".pdf", "application/pdf"},
            {".word", "application/msword"},
            {".png", "image/png"},
            {".gif", "image/gif"},
            {".jpg", "image/jpeg"},
            {".jpeg", "image/jpeg"},
            {".au", "audio/basic"},
            {".mpeg", "video/mpeg"},
            {".mpg", "video/mpeg"},
            {".avi", "video/x-msvideo"},
            {".gz", "application/x-gzip"},
            {".tar", "application/x-tar"},
            {".css", "text/css"},
            {".js", "text/javascript"},
        };
        auto pos = filename.rfind('.');
        if(pos == std::string::npos) {
            return "application/octet-stream";
        }
        std::string ext = filename.substr(pos);
        return m[ext];
    }
    static bool is_regular(const std::string& filename) {
        struct stat st;
        int ret = stat(filename.c_str(), &st);
        if(ret < 0) {
            return false;
        }
        return S_ISREG(st.st_mode);
    }
    static bool is_dir(const std::string& filename) {
        struct stat st;
        int ret = stat(filename.c_str(), &st);
        if(ret < 0) {
            return false;
        }
        return S_ISDIR(st.st_mode);
    }
    static bool valid_path(const std::string& filename) {
        std::vector<std::string> subdir;
        split(filename, "/", &subdir);   
        int level = 0;
        for(auto& dir : subdir) {
            if(dir == "..") {
                --level;
                if(level < 0) {
                    return false;
                }
                continue;
            }
            ++level;
        }
        return true;
    }
};

class mysql_util
{
public:
    static MYSQL* mysql_create(const std::string& host, const std::string& username,
    const std::string& password, const std::string& dbname, uint16_t port = 3306) {
        MYSQL* mysql = ::mysql_init(nullptr);
        if(mysql == nullptr) {
            std::cerr << "mysql_init failed" << std::endl;
            return nullptr;
        }
        if(::mysql_real_connect(mysql, host.c_str(), username.c_str(), password.c_str(), dbname.c_str(), port, nullptr, 0) == nullptr) {
            std::cerr << "mysql_real_connect failed" << std::endl;
            ::mysql_close(mysql);
            return nullptr;
        }
        //设置客户端字符集
        if(::mysql_set_character_set(mysql, "utf8") != 0) {
            std::cerr << "mysql_set_character_set failed" << std::endl;
            ::mysql_close(mysql);
            return nullptr;
        }
        return mysql;
    }
    static bool mysql_exec(MYSQL* mysql, const std::string& sql) {
        int ret = ::mysql_query(mysql, sql.c_str());
        if(ret != 0) {
            std::cerr << "mysql_query failed: " << sql << std::endl;
            return false;
        }
        return true;
    }
    static void mysql_destroy(MYSQL* mysql) {
        if(mysql != nullptr) {
            ::mysql_close(mysql);
        }
    }
};


#endif // __UTILS_HPP__
