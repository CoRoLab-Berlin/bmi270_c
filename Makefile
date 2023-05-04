main: main.c bmi270.c
	gcc -o main -lm main.c bmi270.c

clean:
	rm -f main