CC = gcc
CFLAGS = -O3 -march=native -maes -mpclmul -mavx2 -I.
TARGET = bench
SRC = benchmark.c \
      src/sm4_basic.c \
      src/sm4_ttable.c \
      src/sm4_shuffle.c \
      src/aes_basic.c \
      src/aes_ni.c \
      src/ctr.c \
      src/gift_basic.c \
      src/twine_basic.c

$(TARGET): $(SRC)
$(CC) $(CFLAGS) -o $@ $^

clean:
rm -f $(TARGET)
