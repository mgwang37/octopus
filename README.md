# octopus
# １、中文
## (1)特点
1. 支持TCP代理
1. 支持IPv６
1. 支持用户名密码验证
1. 支持DNS
1. 高并发高性能

## (2)编译
* $ cd builder/
* $ cmake ..
* $ make
* $ cpack

## (3)运行
　配置从命令行输入，详细参数可以使用帮助：

```
　$ octopus-socks5 -h
    --listen.port               listen port      
    --thread.sum                thread sum
    --log.dir                   log output dir
    --thread.connections.sum    the max connections of per thread
    --heartbeat.cycle           heartbeat cycle microseconds (must small 1000000!)
    --access.method             access method : 0 (anonymous), 2(USERNAME/PASSWORD)
    --userlist                  userlist file 
    --help or -v                this message

```

* --listen.port  ：监听的端口
* --thread.sum　：线程数量
* --log.dir　：log信息输出的路径
* --thread.connections.sum　：每个线程最大的连接数
* --heartbeat.cycle 　：超时心跳的间隔
* --access.method　：登录方法，０是匿名，２是用户名密码方式，用户名单用　    --userlist  　指定
* --userlist  ：在　--access.method　为２时指定用户名单
* --help or -v      ：打印帮助信息

## (4)常见问题
### １、连接数受限
1. ＃echo "1024 65535" > /proc/sys/net/ipv4/ip_local_port_range

1. 添加　*          -       nofile          500000　到文件　/etc/security/limits.conf　　



# ２、English
## (1) Features
1. Support TCP proxy
1. Support IPv6
1. Support user name and password verification
1. Support DNS
1. High concurrency and high performance

## (2)compile
* $ cd builder /
* $ cmake ..
* $ make
* $ cpack

## (3)run
  Configuration input from the command line, detailed parameters can be used to help:

```
　$ octopus-socks5 -h
    --listen.port               listen port     
    --thread.sum                thread sum
    --log.dir                   log output dir
    --thread.connections.sum    the max connections of per thread
    --heartbeat.cycle           heartbeat cycle microseconds (must small 1000000!)
    --access.method             access method : 0 (anonymous), 2(USERNAME/PASSWORD) --userlist set the USERNAME/PASSWORD
    --userlist                  userlist file   (see builder/userlist.txt)
    --help or -v                this message

```

## (4)Question
### １、connect　limit
1. ＃echo "1024 65535" > /proc/sys/net/ipv4/ip_local_port_range

1. add　*          -       nofile          500000　to　file　/etc/security/limits.conf　　


