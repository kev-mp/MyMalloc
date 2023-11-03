OUTPUT = mymalloc
CFLAGS = -g -Wall -Wvla -Werror -fsanitize=address
LFLAGS = -lm


%: %.c %.h
		gcc $(CFLAGS) -o $@ $< $(LFLAGS)

all: $(OUTPUT)

clean:
		rm -f *.o $(OUTPUT)

