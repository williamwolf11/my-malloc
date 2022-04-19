CFLAGS=-Wall -pedantic -g

test-my-malloc: test-my-malloc.c my-malloc.so
	gcc $(CFLAGS) -o test-my-malloc test-my-malloc.c

my-malloc.so: my-malloc.c
	gcc $(CFLAGS) -rdynamic -shared -fPIC -o my-malloc.so my-malloc.c

my-malloc: my-malloc.c
	gcc $(CFLAGS) -o my-malloc my-malloc.c

.PHONY: clean
clean:
	rm -f my-malloc test-my-malloc *.o *.so

.PHONY: test
test: test-my-malloc
	gdb --args env LD_PRELOAD=./my-malloc.so ./test-my-malloc -al

.PHONY: gdb
gdb: test-my-malloc
	gdb --args env LD_PRELOAD=./my-malloc.so ./test-my-malloc

.PHONY: ls_gdb
ls_gdb: my-malloc.so
	gdb --args env LD_PRELOAD=./my-malloc.so ls

.PHONY: ls
ls: my-malloc.so 
	LD_PRELOAD=./my-malloc.so ls

.PHONY: ls_al
ls_al: my-malloc.so
	LD_PRELOAD=./my-malloc.so ls -al

.PHONY: ls_al_gdb
ls_al_gdb: my-malloc.so
	gdb --args env LD_PRELOAD=./my-malloc.so ls -al

.PHONY: ls_lr
ls_lr: my-malloc.so
	LD_PRELOAD=./my-malloc.so ls -lR /usr

.PHONY: ls_lr_gdb
ls_lr_gdb: my-malloc.so
	gdb --args env LD_PRELOAD=./my-malloc.so ls -lR /usr

.PHONY: ls_l
ls_l: my-malloc.so
	LD_PRELOAD=./my-malloc.so ls -l

.PHONY: ls_l_gdb
ls_l_gdb: my-malloc.so
	gdb --args env LD_PRELOAD=./my-malloc.so ls -l
