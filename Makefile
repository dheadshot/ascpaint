ascpaint: main.o libs/libqdinp2.a
	${CC} main.o -L./libs -lqdinp2 -o ascpaint
	chmod +x ascpaint

main.o: main.c ascpaint.h
	${CC} -c main.c
