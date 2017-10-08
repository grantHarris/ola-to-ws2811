all: main.cpp
	g++ main.cpp -o ola2ws2811 `pkg-config --cflags --libs libola yaml-cpp` -L lib/ -lws2811
clean: 
	$(RM) ola2ws2811

