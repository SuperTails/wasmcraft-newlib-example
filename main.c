#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

void _start() {
	// printf works almost as normal, with one exception:
	// There's no way to print a partial line in Minecraft chat,
	// so new text won't appear until a newline is written.
	printf("Hello from the newlib example!\n");
	printf("My number is: %d\n", 42);


	// Malloc, free, etc all work exactly as you'd expect
	// (keeping in mind the limited available memory).
	char *my_buffer = malloc(20);
	if (my_buffer == NULL) {
		return; // error
	}


	// User code can just open a file normally and be none the wiser.
	int foo_file = open("some/path/foo.txt", O_RDONLY, 0777);
	if (foo_file < -1) {
		return; // error
	}

	ssize_t num_read = read(foo_file, my_buffer, 19);
	if (num_read < -1) {
		return; // error
	}

	my_buffer[num_read] = '\0';
	printf("The foo file contained:\n%s", my_buffer);

	if (close(foo_file) < 0) {
		return; // error
	}

	free(my_buffer);
}