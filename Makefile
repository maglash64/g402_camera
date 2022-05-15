g402_capture: g402_capture.cpp
	g++ -o gcap g402_capture.cpp `sdl2-config --cflags --libs`

launch: g402_capture
	@sudo chmod 777 /dev/usbd && ./gcap

install:
	@sudo insmod ~/Work/ldd/usb/usbd.ko 
