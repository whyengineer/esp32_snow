CFLAGS += -DARM

./src/subband.o ./src/scalfact.o ./src/dqchan.o ./src/huffman.o: CFLAGS += -Wno-unused-but-set-variable

COMPONENT_SRCDIRS:=src
