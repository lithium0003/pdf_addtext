with open('Dummy.otf','rb') as rf:
    data = rf.read()

print("""\
#include <vector>
#include <cstdint>

std::vector<uint8_t> dummyFont = {
""", end='')

for i in range(len(data)):
    if i % 16 == 0:
        print("\t", end='')
    print("0x"+data[i:i+1].hex()+",", end='')
    if i % 16 == 15:
        print()
    else:
        print(" ", end='')
print()
print("};")
