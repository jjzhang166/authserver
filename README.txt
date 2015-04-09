README
服务器安装配置:
服务器操作数据库依赖来apr-1.5.1和apr-util-1.5.4
可以执行installServer脚本完成apr包的编译安装和服务器的安装
默认apr装在/usr/local/lib  server装在/usr/local/bin

服务器功能总结:
服务器采用多线程，
服务器实现来自定义协议PDU的生成和解析
已经完成认证和心跳功能

开发者注意:
线程运行时要自我detach
注意控制线程数 用型号量sem
如果处理数据包是可能阻塞，创建线程做，不能阻塞handle_tcp_thread

