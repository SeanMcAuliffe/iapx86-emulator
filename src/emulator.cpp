#include <bitset>
#include <cstdlib>
#include <iostream>
#include <format>
#include <filesystem>
#include <fstream>
#include <strstream>
#include <vector>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <sstream>
#include <array>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

#define U8(x) static_cast<u8>(x)
#define INT(x) static_cast<int>(x)

/*
 * See 8086 user manual Table 409: REG (Register) Field Encoding,
 * using the W bit at the most significant bit.
 */
constexpr std::array<std::pair<u8, const char*>, 16> REGISTER_ENCODING {{
  {U8(0b0000), "al"},
  {U8(0b0001), "cl"},
  {U8(0b0010), "dl"},
  {U8(0b0011), "bl"},
  {U8(0b0100), "ah"},
  {U8(0b0101), "ch"},
  {U8(0b0110), "dh"},
  {U8(0b0111), "bh"},
  {U8(0b1000), "ax"},
  {U8(0b1001), "cx"},
  {U8(0b1010), "dx"},
  {U8(0b1011), "bx"},
  {U8(0b1100), "sp"},
  {U8(0b1101), "bp"},
  {U8(0b1110), "si"},
  {U8(0b1111), "di"}
}};

const std::unordered_map<u8, std::string>& getRegisterNameMap() {
  static const std::unordered_map<u8, std::string> register_names(
      REGISTER_ENCODING.begin(), REGISTER_ENCODING.end()
  );
  return register_names;
}

std::vector<u8> read_binary_file(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file) {
      std::cerr << "Could not open file: " << filename << "\n";
      std::abort();
    }
    std::streamsize size = file.tellg();
    if (size % 2 != 0) {
      std::cerr << "Ill-formed program: odd number of bytes\n";
      std::abort();
    }
    std::vector<u8> buffer(size);
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(buffer.data()), size);
    return buffer;
}

struct Instruction {
  unsigned char high {};
  unsigned char low {};
};

std::string instruction_mnemonic(const Instruction &instruction) {
  /* Switch on the highest 6 bits
   * From the 8086 user manual:
   *
   * "The first six bits of a multibyte instruction generally contain an opcode
   * that identifies the basic instruction type: ADD, XOR, etc."
   */
  u8 high_six = (instruction.high & 0xfc) >> 2;
  switch (high_six) {
    case 0b100010:
      return std::string("mov");
      break;
    default:
      std::cerr << "Unknown command: " << INT(high_six) << "\n";
      std::abort();
  }
}

std::string identify_mov_registers(const Instruction &instruction) {
  auto register_names = getRegisterNameMap();

  bool direction = instruction.high & 0b10;
  bool wide = instruction.high & 0b01;
  u8 mod = (instruction.low & 0b11000000) >> 6;
  u8 reg = (instruction.low & 0b00111000) >> 3;
  u8 rm = instruction.low & 0b00000111;

  rm = rm + (wide << 3);
  reg = reg + (wide << 3);
  auto not_found = register_names.end();

  std::unordered_map<u8, std::string>::const_iterator source_it {};
  std::unordered_map<u8, std::string>::const_iterator destination_it {};

  if (direction) {
    source_it = register_names.find(rm);
    destination_it = register_names.find(reg);
  } else {
    destination_it = register_names.find(reg);
    source_it = register_names.find(rm);
  }

  if (source_it == not_found || destination_it == not_found) {
    std::cout << "Ill-formed program; bad register encoding\n";
    exit(EXIT_FAILURE);
  }

  return std::format("{}, {}", (*source_it).second, (*destination_it).second);

}

/* Produces a string representation of the original ASM text which
 * produced the provided binary encoded instruction.
 */
std::string disassemble_instruction(const Instruction &instruction) {
  std::ostringstream output {};
  output << instruction_mnemonic(instruction) << " ";
  output << identify_mov_registers(instruction);
  return output.str();
}

int main(int argc, char **argv) {
  if (argc < 2) {
    return 0;
  }

  char* program_filename = argv[1];
  if (!std::filesystem::exists(program_filename)) {
    std::cerr << "Could not find assembly file\n";
    std::abort();
  }

  std::ifstream program_file(program_filename);
  std::vector<u8> program_data = read_binary_file(program_filename);
  std::vector<Instruction> program {};

  for (unsigned int i = 0; i < program_data.size(); i += 2) {
    program.push_back({.high = program_data[i], .low = program_data[i + 1]});
  }

  for (auto instruction : program) {
    std::cout << disassemble_instruction(instruction) << "\n";
  }

  return 0;
}
