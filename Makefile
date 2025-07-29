TARGET = pdf_addtext
SRCS   = main.cpp
SRCS  += pdf.cpp
SRCS  += dummyfont.cpp
SRCS  += json.cpp
SRCS  += crypt.cpp
OBJS   = $(SRCS:.cpp=.o)

CFLAGS = -O2 -std=c++17 `pkg-config --cflags libjpeg` `pkg-config --cflags openssl` `pkg-config --cflags zlib` `pkg-config --cflags libpng`
LIBS   = `pkg-config --libs libjpeg` `pkg-config --libs openssl` `pkg-config --libs zlib` `pkg-config --libs libpng`

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $@ -g $^ $(LIBS)

.cpp.o:
	$(CXX) -c $< -g $(CFLAGS)

main.o: main.cpp pdf.hpp json.hpp
pdf.o: pdf.cpp pdf.hpp json.hpp
crypt.o: crypt.cpp crypt.hpp pdf.hpp
json.o: json.cpp json.hpp
dummyfont.o: dummyfont.cpp

.PHONY : clean
clean: 
	@rm -rf $(TARGET) $(OBJS)
