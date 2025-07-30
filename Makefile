TARGET = pdf_addtext

SRCS   = main.cpp
SRCS  += pdf.cpp
SRCS  += dummyfont.cpp
SRCS  += json.cpp
SRCS  += crypt.cpp

OBJS   = $(SRCS:.cpp=.o)
OBJS  += transupp.o

CFLAGS = -O2 -std=c++17 -Ilibjpeg-turbo/src -Ilibjpeg `pkg-config --cflags openssl` `pkg-config --cflags zlib` `pkg-config --cflags libpng`
LIBS   = libjpeg/libjpeg.a `pkg-config --libs openssl` `pkg-config --libs zlib` `pkg-config --libs libpng`

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(LIBS)

.cpp.o:
	$(CXX) -c $< $(CFLAGS)

main.o: main.cpp pdf.hpp json.hpp
pdf.o: libjpeg pdf.cpp pdf.hpp json.hpp
crypt.o: crypt.cpp crypt.hpp pdf.hpp
json.o: json.cpp json.hpp
dummyfont.o: dummyfont.cpp

libjpeg:
	cmake -B libjpeg -S libjpeg-turbo
	make -C libjpeg

transupp.o: libjpeg-turbo/src/transupp.c
	$(CC) -c $< -O2 -Ilibjpeg

.PHONY : clean
clean: 
	@rm -rf $(TARGET) $(OBJS) libjpeg
