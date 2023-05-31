main: main.c bmi270.c
	gcc -o main bmi270.c main.c -lm

clean:
	rm -f main