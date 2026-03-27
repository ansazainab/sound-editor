FLAGS = -fno-sanitize=all -fPIC -Wvla -Werror -fsanitize=address -g

sound.o: sound_seg.c helper_functions.h
	gcc -c sound_seg.c -o sound.o $(FLAGS)

helper_functions.o: helper_functions.c helper_functions.h
	gcc -c helper_functions.c -o helper_functions.o $(FLAGS)

sound_seg.o: sound.o helper_functions.o
	ld -r -o sound_seg.o sound.o helper_functions.o