## Build
```
mkdir build && cd build

cmake ..

make

sudo make install 
```

## Example
gst-launch-1.0 tcpclientsrc host=127.0.0.1 port=5000 ! frameparser ! videoconvert ! xvimagesink






