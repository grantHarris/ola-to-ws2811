all: main.cpp
	g++ main.cpp $(pkg-config --cflags --libs libola) -L lib/ -lws2811 -o ola2ws2811
clean: 
	$(RM) ola2ws2811

