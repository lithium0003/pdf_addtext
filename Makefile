TARGET = pdf_addtext

SRCS   = main.cpp
SRCS  += pdf.cpp
SRCS  += dummyfont.cpp
SRCS  += json.cpp
SRCS  += crypt.cpp
SRCS  += fax.cpp
SRCS  += filters.cpp

JBIG2_SRC = $(wildcard JBIG2/*.cpp)
JBIG2_OBJ = $(JBIG2_SRC:.cpp=.o)

OBJS   = $(SRCS:.cpp=.o)
OBJS  += transupp.o
OBJS  += $(JBIG2_OBJ)

CFLAGS = -O2 -std=c++17 -I. -Ilibjpeg-turbo/src -Ilibjpeg `pkg-config --cflags openssl` `pkg-config --cflags zlib` `pkg-config --cflags libpng`
LIBS   = libjpeg/libjpeg.a `pkg-config --libs openssl` `pkg-config --libs zlib` `pkg-config --libs libpng`

.SUFFIXES: .o .c .cpp

all: $(TARGET) $(JBIG2_OBJ)

$(TARGET): $(OBJS)
	$(CXX) -g -o $@ $^ $(LIBS)

.cpp.o:
	$(CXX) -g -c $< $(CFLAGS) -o $@

main.o: main.cpp pdf.hpp json.hpp
pdf.o: libjpeg pdf.cpp pdf.hpp json.hpp fax.hpp filters.hpp
crypt.o: crypt.cpp crypt.hpp pdf.hpp
json.o: json.cpp json.hpp
dummyfont.o: dummyfont.cpp
fax.o: fax.cpp fax.hpp
filters.o: filters.cpp filters.hpp

libjpeg:
	cmake -B libjpeg -S libjpeg-turbo
	make -C libjpeg

transupp.o: libjpeg-turbo/src/transupp.c
	$(CC) -c $< -O2 -Ilibjpeg

.PHONY : clean
clean: 
	@rm -rf $(TARGET) $(OBJS) libjpeg
