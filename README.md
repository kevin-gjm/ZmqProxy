### 使用 ZMQ 代理模式
参考ZMQ-guid  Figure 38 模式实现
![zmq fig38 Detail of Asynchronous Server](https://github.com/imatix/zguide/raw/master/images/fig38.png)

相应的Woker参考地址
https://github.com/kevin-gjm/ZmqWorker.git

### 备注
- 使用C++11
- 依赖库:glog zmq 
- 用户自定义数据为最后一帧,且仅有一帧,若多帧用户自己修改即可