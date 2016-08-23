CC=gcc
OBJS=hybrid_fuse.c
TARGET=hb_fuse

all:$(TARGET)

$(TARGET):$(OBJS)
	$(CC) -o $@ $(OBJS) -D_FILE_OFFSET_BITS=64 `pkg-config fuse --cflags --libs`

clean:
	rm -f $(TARGET)
