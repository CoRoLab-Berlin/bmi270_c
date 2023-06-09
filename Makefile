main: main.c driver/bmi270.c
	gcc -o main main.c driver/bmi270.c -Idriver -lm

clean:
	rm -f main
