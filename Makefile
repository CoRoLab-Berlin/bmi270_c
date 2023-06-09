main: example/main.c driver/bmi270.c
	gcc -o main example/main.c driver/bmi270.c -Idriver -lm

clean:
	rm -f main
