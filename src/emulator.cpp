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
#define SIZE(x) static_cast<size_t>(x)


template <class E>
constexpr auto to_underlying(E e) {
  return static_cast<std::underlying_type_t<E>>(e);
}


/*
 * Represents an operation, which, once known, tells us how many more bytes
 * to fetch as part of this instruction, and how to decode them. See Table
 * 4-12 "8086 Instruction Encoding".
 */
enum class Operation {
  REGMEM_TO_FROM_REG,
  IMM_TO_REGMEM,
  IMM_TO_REG,
  MEM_TO_ACC,
  ACC_TO_MEM,
  COUNT 
};


struct Opcode {
  Operation operation;
  u8 bits;
  u8 length;
};


struct Instruction {

};


constexpr std::array<Opcode, SIZE(Operation::COUNT)> opcodes {{
  {Operation::REGMEM_TO_FROM_REG, 0b100010, 6},
  {Operation::IMM_TO_REGMEM, 0b1100011, 7},
  {Operation::IMM_TO_REG, 0b1011, 4},
  {Operation::MEM_TO_ACC, 0b1010000, 7},
  {Operation::ACC_TO_MEM, 0b1010001, 7}
}};


std::string to_string(Operation operation) {
  switch (operation) {
    case Operation::REGMEM_TO_FROM_REG: return "To Register/Memory to/from Register";
    case Operation::IMM_TO_REGMEM: return "Immediate to Register/Memory";
    case Operation::IMM_TO_REG: return "Immediate to Register";
    case Operation::MEM_TO_ACC: return "Memory to Accumulator";
    case Operation::ACC_TO_MEM: return "Accumulator to Memory";
    default: return "Unknown Operation";
  }
}


std::string get_opcode_name(Operation operation) {
  using O = Operation;
  switch (operation) {
    case O::REGMEM_TO_FROM_REG:
    case O::IMM_TO_REGMEM:
    case O::IMM_TO_REG:
    case O::MEM_TO_ACC:
    case O::ACC_TO_MEM:
      return "mov";
    default:
      std::cerr << std::format(
        "{}: Unrecognized opcode: {}\n", __LINE__, to_underlying(operation)
      );
      std::exit(EXIT_FAILURE);
  }
}


Operation match_opcode(u8 byte) {
  for (const Opcode& o : opcodes) {
    u8 shift = 8 - o.length;
    u8 mask = 0xFF << shift;
    u8 prefix = (byte & mask) >> shift;
    if (prefix == o.bits) {
      return o.operation;
    }
  }
  std::cerr << std::format(
    "{}: Could not match instruction {} to opcode\n", __LINE__, (int)byte
  );
  std::exit(EXIT_FAILURE);
}


/*
 * See 8086 user manual Table 409: REG (Register) Field Encoding,
 * using the W bit at the most significant bit.
 * 
 * Constant data used to construct a Meyer's Singleton unordered map
 * in get_register_name_map().
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


const std::unordered_map<u8, std::string>& get_register_name_map() {
  static const std::unordered_map<u8, std::string> register_names(
      REGISTER_ENCODING.begin(), REGISTER_ENCODING.end()
  );
  return register_names;
}

constexpr std::array<std::pair<u8, const char*>, 8> RM_ENCODING{{
  {U8(0b000), "bx + si"},
  {U8(0b001), "bx + di"},
  {U8(0b010), "bp + si"},
  {U8(0b011), "bp + di"},
  {U8(0b100), "si"},
  {U8(0b101), "di"},
  {U8(0b110), "bp"},
  {U8(0b111), "bx"}
}};


const std::unordered_map<u8, std::string>& get_rm_map() {
  static const std::unordered_map<u8, std::string> rm_values(
      RM_ENCODING.begin(), RM_ENCODING.end()
  );
  return rm_values;
}


void read_binary_file(const std::string& filename, std::vector<u8> &program_buffer) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file) {
      std::cerr << std::format("{}: Could not open file: {}\n", __LINE__, filename);
      std::exit(EXIT_FAILURE);
    }
    std::streamsize size = file.tellg();
    program_buffer.resize(size);
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(program_buffer.data()), program_buffer.size());
}


u8 additional_bytes(Operation op, std::vector<u8> &instruction, bool &more) {
  switch (op) {
    case Operation::REGMEM_TO_FROM_REG:
      more = true;
      return 1;
    case Operation::IMM_TO_REGMEM:
      more = true;
      return 1;
    case Operation::IMM_TO_REG:
	  {
	    bool wide = (instruction[0] & 0b1000) >> 3;
        more = false;
        return wide ? 2 : 1;
	  }
    case Operation::MEM_TO_ACC:
      more = false;
      return 2;
    case Operation::ACC_TO_MEM:
      more = false;
      return 2;
    default:
      std::cerr << std::format("{} Unhandled case\n", __LINE__);
      std::exit(EXIT_FAILURE);
  }
}


u8 additional_bytes(Operation op, std::vector<u8> &instruction) {
  u8 high = instruction[0];
  u8 low = instruction[1];
  bool wide = high & 0b01;
  u8 mod = (low & 0b11000000) >> 6;
  u8 rm = low & 0b00000111;
  switch (op) {
    case Operation::REGMEM_TO_FROM_REG:
      if ((mod == 0b10) || (mod == 0b00 && rm == 0b110)) {
        return 2;
      } else if ((mod == 0b11) || (mod = 0b00 && rm != 0b110)) {
        return 0;
      } else if (mod == 0b01) {
        return 1;
      } else {
        std::cerr << std::format("{}: Unhandled case, mod = {}, rm = {}\n", __LINE__, mod, rm);
        std::exit(EXIT_FAILURE);
      }
      break;
    case Operation::IMM_TO_REGMEM:
      {
        u8 data_bytes = wide ? 2 : 1;
        if ((mod == 0b10) || (mod == 0b00 && rm == 0b110)) {
          return 2 + data_bytes;
        } else if ((mod == 0b11) || (mod = 0b00 && rm != 0b110)) {
          return 0 + data_bytes;
        } else if (mod == 0b01) {
          return 1 + data_bytes;
        } else {
          std::cerr << std::format("{}: Unhandled case\n", __LINE__);
          std::exit(EXIT_FAILURE);
        }
      }
      break;
    default:
      std::cerr << std::format("{}: Unhandled case\n", __LINE__);
      std::exit(EXIT_FAILURE);
  }
}


std::string disassemble(std::string name, Operation op, std::vector<u8> &instruction) {
  using iterator = std::unordered_map<u8, std::string>::const_iterator;
  std::string instr_str{"unk\n"};

  switch (op) {

    case Operation::REGMEM_TO_FROM_REG:
      {
        u8 high = instruction[0];
        u8 low = instruction[1];
        bool wide = high & 0b01;
        bool direction = (high & 0b10) >> 1;
        u8 mod = (low & 0b11000000) >> 6;
        u8 reg = (low & 0b00111000) >> 3;
        u8 rm = low & 0b00000111;

        if (mod == 0b10) {
          /* Effective address calculation w/ 16-bit displacement */
          u8 disp_high = instruction[3];
          u8 disp_low = instruction[2];
        } else if (mod == 0b01) {
          /* Effective address calculation w/ 8-bit displacement */
          u8 disp = instruction[2];
        } else if (mod == 0b00) {
          if (rm == 0b110) {
            /* Direct address */
          } else {
            /* No displacement */
            auto register_names = get_register_name_map();
            auto rm_values = get_rm_map();
            u8 key_reg  = reg + (wide << 3);
            iterator reg_it = register_names.find(key_reg);
            iterator rm_it = rm_values.find(rm);
			if (reg_it == register_names.end() || rm_it == rm_values.end()) {
              std::cerr << std::format("{}: Bad register encoding\n", __LINE__);
              std::exit(EXIT_FAILURE);
            }
			if (direction) {
				instr_str = std::format("{} [{}], {}\n", name, (*rm_it).second, (*reg_it).second);
			} else {
				instr_str = std::format("{} {}, [{}]\n", name, (*reg_it).second, (*rm_it).second);
			}
          }
        } else if (mod == 0b11) {
            /* Register to Register */
            u8 key_rm = rm + (wide << 3);
            u8 key_reg  = reg + (wide << 3);
            auto register_names = get_register_name_map();
            auto not_found = register_names.end();
            iterator source_it {};
            iterator destination_it {};
            if (direction) {
              source_it = register_names.find(key_rm);
              destination_it = register_names.find(key_reg);
            } else {
              destination_it = register_names.find(key_reg);
              source_it = register_names.find(key_rm);
            }
            if (source_it == not_found || destination_it == not_found) {
              std::cerr << std::format("{}: Bad register encoding\n", __LINE__);
              std::exit(EXIT_FAILURE);
            }
            instr_str = std::format(
              "{} {}, {}\n", name, (*source_it).second, (*destination_it).second
            );
        }
      }
      break;

    case Operation::IMM_TO_REGMEM:
      break;
    case Operation::IMM_TO_REG:
	  {
		bool wide = (instruction[0] & 0b1000) >> 3;
		u8 reg = instruction[0] & 0b111;
	    u8 key_reg = reg + (wide << 3);
	    auto register_names = get_register_name_map();
	    iterator reg_it = register_names.find(key_reg);
	    if (reg_it == register_names.end()) {
	      std::cerr << std::format("{}: Bad register encoding: {}\n", __LINE__, (int)key_reg);
	      std::exit(EXIT_FAILURE);
	    }
	    if (wide) {
	        u16 data = instruction[1] + (instruction[2] << 8);
	        instr_str = std::format("{} {}, {}\n", name, reg_it->second, data);
	    } else {
	        u8 data = instruction[1];
	        instr_str = std::format("{}, {}, {}\n", name, reg_it->second, data);
	    }
	  }
      break;
    case Operation::MEM_TO_ACC:
      break;
    case Operation::ACC_TO_MEM:
      break;
    default:
      std::cerr << std::format("{}: Unhandled case\n", __LINE__);
      std::exit(EXIT_FAILURE);
  }
  return instr_str;
}


int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << std::format(
      "{}: Must specify program file as first positional argument\n", __LINE__
    );
    return EXIT_FAILURE;
  }

  char* program_filename = argv[1];
  if (!std::filesystem::exists(program_filename)) {
    std::cerr << std::format(
	  "{}: Could not find program file: {}\n", __LINE__, program_filename
	);
    return EXIT_FAILURE;
  }

  std::ifstream program_file(program_filename);
  std::vector<u8> program_data {};
  read_binary_file(program_filename, program_data);

  u16 program_cursor = 0;
  while (program_cursor < program_data.size()) {
    std::vector<u8> instruction {program_data[program_cursor]};

    Operation operation = match_opcode(program_data[program_cursor]);
    std::string name = get_opcode_name(operation);
    program_cursor += 1;

    bool more = false;
    u8 additional_num = additional_bytes(operation, instruction, more);
    for (unsigned int i = 0; i < additional_num; i++) {
      instruction.push_back(program_data[program_cursor + i]);
    }
    program_cursor += additional_num;

    if (more) {
      additional_num = additional_bytes(operation, instruction);
      for (unsigned int i = 0; i < additional_num; i++) {
        instruction.push_back(program_data[program_cursor + i]);
      }
      program_cursor += additional_num;
    }

    std::cout << disassemble(name, operation, instruction);
  }
  
  return 0;
}
