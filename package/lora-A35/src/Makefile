TARGET = mp257-lora

CC ?= gcc

LOCAL_CPPFLAGS = -Ilib/inc -Iinclude
LOCAL_CFLAGS = -Wall -Wextra

SRCS = \
	main.c \
	lib/src/spi_lora.c

OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CPPFLAGS) $(LOCAL_CPPFLAGS) $(CFLAGS) $(LOCAL_CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean
