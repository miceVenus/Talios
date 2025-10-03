head.o:	head.S
	gcc -E head.S > head.s
	as 	-o head.o head.s
	rm 	head.s