#include"httpserver.hpp"


#define WWWROOT "./wwwroot/"
#define HOST "127.0.0.1"
#define PORT 3306
#define USER "root"
#define PASSWARD "#Yaz546040011213"
#define DBNAME "calculate_db"

std::string request_str(const HttpRequest& req) {
    std::stringstream ss;
    ss << req._method << " " << req._path << " " << req._version << "\r\n";
    for(auto& head : req._headers) {
        ss << head.first << ": " << head.second << "\r\n";
    }
    for(auto& arg : req._params) {
        ss << arg.first << ": " << arg.second << "\r\n";
    }
    ss << "\r\n";
    return ss.str();
}

void Hello(const HttpRequest& req, HttpResponse* resp) {
    std::string str = request_str(req);
    resp->SetContent(str, "text/plain");
}
void Login(const HttpRequest& req, HttpResponse* resp) {
    std::string str = request_str(req);
    resp->SetContent(str, "text/plain");
}
void Put(const HttpRequest& req, HttpResponse* resp) {
    std::string str = request_str(req);
    resp->SetContent(str, "text/plain");
}
void Post(const HttpRequest& req, HttpResponse* resp) {
    std::string str = request_str(req);
    resp->SetContent(str, "text/plain");
}
void Calculate(const HttpRequest& req, HttpResponse* resp) {
    // 获取用户提交上来的表达式
    std::string expr = req._body;
    // 将表达式拆分, 先去除前缀expression=
    expr.erase(0, 11);
    std::cout << "expr= " << expr << std::endl;
    //将表达式中的%2B替换成+，%2D替换成-，%2A替换成*，%2F替换成/
    std::string::size_type pos = 0;
    std::string op;
    while((pos = expr.find("%2B", pos)) != std::string::npos) {
        expr.replace(pos, 3, "+");
        op = "+";
        pos += 1;
    }
    pos = 0;
    while((pos = expr.find("-", pos)) != std::string::npos) {
        op = "-";
        break;
    }
    pos = 0;
    while((pos = expr.find("*", pos)) != std::string::npos) {
        op = "*";
        break;
    }
    pos = 0;
    while((pos = expr.find("/", pos)) != std::string::npos) {
        op = "/";
        break;
    }
    if(op != "+") {
        for(int i = 0; i < expr.size(); ++i) {
            if(!isdigit(expr[i])) {
                expr[i] = op[0];
            }
        }
    }
    std::cout << "expr= " << expr << std::endl;
    std::vector<std::string> tokens;
    Utils::split(expr, op, &tokens);
    std::cout << "tokens.size() = " << tokens.size() << std::endl;
    for(auto& token : tokens) {
        std::cout << token << std::endl;
    }
    //将拆分的表达式进行计算
    double x, y;
    x = std::stof(tokens[0]);
    std::cout << "x = " << x << std::endl;
    std::cout << "op: " << op << std::endl;
    y = std::stof(tokens[1]);
    std::cout << "y = " << y << std::endl;
    double result = 0;
    if(op == "+") {
        result = x + y;
    } else if(op == "-") {
        result = x - y;
    } else if(op == "*") {
        result = x * y;
    } else if(op == "/") {
        result = x / y;
    }
    std::cout << "result = " << result << std::endl;
    MYSQL* mysql = mysql_util::mysql_create(HOST, USER, PASSWARD, DBNAME, PORT);
    char buffer[1024] = {0};
    snprintf(buffer, sizeof(buffer) - 1, "insert into CalculatorHistory (operand1, operand2, operator, result) values (%f, %f, '%c', %f);", x, y, op, result);
    bool ret = mysql_util::mysql_exec(mysql, buffer);
    mysql_util::mysql_destroy(mysql);
    // 构造html页面
    std::stringstream ss;
    ss << "<html>";
    ss << "<head>";
    ss << "<meta charset=\"UTF-8\">";
    ss << "<title>Calculator</title>";
    ss << "<style>";
    ss << "body {";
    ss << "  display: flex;";
    ss << "  justify-content: center;";
    ss << "  align-items: center;";
    ss << "  height: 100vh;";
    ss << "}";
    ss << "h1 {";
    ss << "  font-size: 48px;";
    ss << "  text-align: center;";
    ss << "}";
    ss << "</style>";
    ss << "</head>";
    ss << "<body>";
    ss << "<h1>";
    ss << result;
    ss << "</h1>";
    ss << "</body>";
    ss << "</html>";

    // 将html页面返回给浏览器
    resp->SetContent(ss.str(), "text/html");
}
int main()
{
    HttpServer server(8082);
    server.SetThreadCount(3);
    server.SetBaseDir(WWWROOT);
    server.Get("/hello", Hello);
    server.Post("/login", Login);
    // server.Put("/1234.txt", Put);
    // server.Delete("/1234.txt", Post);
    server.Post("/calculate", Calculate);
    server.Listen();
    return 0;
}