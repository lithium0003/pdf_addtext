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
OBJS  += $(JBIG2_OBJ)

CFLAGS = -O2 -std=c++17 -I. `pkg-config --cflags libjpeg` `pkg-config --cflags openssl` `pkg-config --cflags zlib` `pkg-config --cflags libpng` `pkg-config --cflags libopenjp2`
LIBS   = `pkg-config --libs libjpeg` `pkg-config --libs openssl` `pkg-config --libs zlib` `pkg-config --libs libpng` `pkg-config --libs libopenjp2`

.SUFFIXES: .o .c .cpp

all: $(TARGET) $(JBIG2_OBJ)

$(TARGET): $(OBJS)
	$(CXX) -g -o $@ $^ $(LIBS)

.cpp.o:
	$(CXX) -g -c $< $(CFLAGS) -o $@

main.o: main.cpp pdf.hpp json.hpp
pdf.o: pdf.cpp pdf.hpp json.hpp fax.hpp filters.hpp
crypt.o: crypt.cpp crypt.hpp pdf.hpp
json.o: json.cpp json.hpp
dummyfont.o: dummyfont.cpp
fax.o: fax.cpp fax.hpp
filters.o: filters.cpp filters.hpp

.PHONY : clean
clean: 
	@rm -rf $(TARGET) $(OBJS)
