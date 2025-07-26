TARGET = pdf_addtext
SRCS   = main.cpp
SRCS  += pdf.cpp
SRCS  += dummyfont.cpp
SRCS  += json.cpp
OBJS   = $(SRCS:.cpp=.o)

CFLAGS = -O2 -std=c++17 `pkg-config --cflags libjpeg` `pkg-config --cflags zlib`
LIBS   = `pkg-config --libs libjpeg` `pkg-config --libs zlib`

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(LIBS)

.cpp.o:
	$(CXX) -c $< $(CFLAGS)

main.o: main.cpp pdf.hpp json.hpp
main.o: pdf.hpp pdf.hpp json.hpp
json.o: json.cpp json.hpp
dummyfont.o: dummyfont.cpp

.PHONY : clean
clean: 
	@rm -rf $(TARGET) $(OBJS)
