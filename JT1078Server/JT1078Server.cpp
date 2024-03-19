#include"JT1078/JT1078.h"

#include<muduo/net/EventLoop.h>
#include<muduo/net/TcpServer.h>

#include<memory>
#include<set>

using muduo::net::EventLoop;
using muduo::net::InetAddress;
using muduo::net::TcpConnection;
using muduo::net::TcpServer;
using muduo::net::Buffer;
using muduo::net::TcpConnectionPtr;
using muduo::Timestamp;
using boost::any;

class JT1078Server {
public:
	JT1078Server(EventLoop*loop_,const InetAddress &ip_,int thread_num_=4)
		:loop_(loop_), ip_(ip_), thread_num_(thread_num_), server_(loop_, ip_, "JT1078Server", TcpServer::kReusePort) 
	{
		server_.setMessageCallback(bind(&JT1078Server::read_cb,this,placeholders::_1,placeholders::_2,placeholders::_3));
		server_.setConnectionCallback(bind(&JT1078Server::conn_cb,this,placeholders::_1));
		server_.start();
	}
	~JT1078Server() {
		for (auto conn : all_conns_)
		{
			any any_val=conn->getContext();
			if (!any_val.empty())
			{
				JT1078*parser=boost::any_cast<JT1078*>(any_val);
				delete parser;
				any_val.clear();
			}
		}
	}

	void read_cb(const TcpConnectionPtr& conn, Buffer* buf, Timestamp curr_time)
	{
		auto jt1078parser = boost::any_cast<JT1078*>(conn->getContext());
		
		if (jt1078parser)
		{
			_read_cb(conn, buf, jt1078parser);
		}
		else close_conn(conn);
	}
private:
	void conn_cb(const TcpConnectionPtr& conn)
	{
		if (conn->connected())//连接
		{
			JT1078* jt1078parser(new (std::nothrow)JT1078);
			conn->setContext(jt1078parser);
			all_conns_.insert(conn);
		}
		else {//断开连接
			any any_val = conn->getContext();
			JT1078* parser=boost::any_cast<JT1078*>(any_val);
			delete parser;
			any_val.clear();
			all_conns_.erase(conn);
		}
	}
	void close_conn(const TcpConnectionPtr& conn)
	{
		conn->forceCloseWithDelay(2); 
	}

	void _read_cb(const TcpConnectionPtr& conn, Buffer* buf,JT1078* jt1078parser)
	{
		const uint8_t* p_ = reinterpret_cast<const uint8_t*>(buf->peek());
		size_t length = buf->readableBytes();
		size_t much = 0;
		int ret = jt1078parser->parse(p_, length, much);
		if (ret >= 0)
		{
			buf->retrieve(much);
		}
		else if (ret < 0) { close_conn(conn);cerr << "parse error!close connection!";
		}
	}
private:
	EventLoop *loop_;
	InetAddress ip_;
	int thread_num_;
	TcpServer server_;
	set<TcpConnectionPtr> all_conns_;
};

int main(int argc,char**argv)
{
	if (argc < 2)
	{
		cout << "Usage:<Port>" << endl;
		return 0;
	}
	muduo::net::EventLoop loop;
	muduo::net::InetAddress ip(atoi(argv[1]));
	JT1078Server server_(&loop, ip);
	loop.loop();
	return 0;
}
