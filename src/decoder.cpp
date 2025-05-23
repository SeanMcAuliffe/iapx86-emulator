#include <cstdlib>
#include "stdio.h"
#include <iostream>
#include <format>
#include <filesystem>
#include <fstream>
#include <vector>
#include <cstdint>
#include <string>
#include <unordered_map>
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
#define I8(x) static_cast<i8>(x)
#define U16(x) static_cast<u16>(x)
#define I16(x) static_cast<i16>(x)
#define INT(x) static_cast<int>(x)
#define SIZE(x) static_cast<size_t>(x)


constexpr std::string_view GENERIC_OP = "asc";


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
  ASC_IMM_TO_REGMEM,
  ADD_REGMEM_WITH_REG,
  ADD_IMM_TO_ACC,
  SUB_REGMEM_WITH_REG,
  SUB_IMM_FROM_ACC,
  CMP_REGMEM_AND_REG,
  CMP_IMM_WITH_ACC,
  JMP_EQUAL,
  JMP_LESS,
  JMP_LESS_OR_EQUAL,
  JMP_BELOW,
  JMP_BELOW_OR_EQUAL,
  JMP_PARITY,
  JMP_OVERFLOW,
  JMP_SIGN,
  JMP_NOT_EQUAL,
  JMP_NOT_LESS,
  JMP_NOT_LESS_OR_EQUAL,
  JMP_NOT_BELOW,
  JMP_NOT_BELOW_OR_EQUAL,
  JMP_NOT_PARITY,
  JMP_NOT_OVERFLOW,
  JMP_NOT_SIGN,
  LOOP,
  LOOPZ,
  LOOPNZ,
  JMP_CX_ZERO,
  COUNT 
};


struct Opcode {
  Operation operation;
  u8 bits;
  u8 length;
};


constexpr std::array<Opcode, SIZE(Operation::COUNT)> opcodes {{
  {Operation::REGMEM_TO_FROM_REG,     0b100010, 6},
  {Operation::IMM_TO_REGMEM,          0b1100011, 7},
  {Operation::IMM_TO_REG,             0b1011, 4},
  {Operation::MEM_TO_ACC,             0b1010000, 7},
  {Operation::ACC_TO_MEM,             0b1010001, 7},
  {Operation::ASC_IMM_TO_REGMEM,      0b100000, 6},
  {Operation::ADD_REGMEM_WITH_REG,    0b0, 6},
  {Operation::ADD_IMM_TO_ACC,         0b10, 7},
  {Operation::SUB_REGMEM_WITH_REG,    0b001010, 6},
  {Operation::SUB_IMM_FROM_ACC,       0b0010110, 7},
  {Operation::CMP_REGMEM_AND_REG,     0b001110, 6},
  {Operation::CMP_IMM_WITH_ACC,       0b0011110, 7},
  {Operation::JMP_EQUAL,              0b01110100, 8},
  {Operation::JMP_LESS,               0b01111100, 8},
  {Operation::JMP_LESS_OR_EQUAL,      0b01111110, 8},
  {Operation::JMP_BELOW,              0b01110010, 8},
  {Operation::JMP_BELOW_OR_EQUAL,     0b01110110, 8},
  {Operation::JMP_PARITY,             0b01111010, 8},
  {Operation::JMP_OVERFLOW,           0b01110000, 8},
  {Operation::JMP_SIGN,               0b01111000, 8},
  {Operation::JMP_NOT_EQUAL,          0b01110101, 8},
  {Operation::JMP_NOT_LESS,           0b01111101, 8},
  {Operation::JMP_NOT_LESS_OR_EQUAL,  0b01111111, 8},
  {Operation::JMP_NOT_BELOW,          0b01110011, 8},
  {Operation::JMP_NOT_BELOW_OR_EQUAL, 0b01110111, 8},
  {Operation::JMP_NOT_PARITY,         0b01111011, 8},
  {Operation::JMP_NOT_SIGN,           0b01111001, 8},
  {Operation::JMP_NOT_OVERFLOW,       0b01110001, 8},
  {Operation::LOOP,                   0b11100010, 8},
  {Operation::LOOPZ,                  0b11100001, 8},
  {Operation::LOOPNZ,                 0b11100000, 8},
  {Operation::JMP_CX_ZERO,            0b11100011, 8} 
}};


std::string to_string(Operation operation) {
  switch (operation) {
    case Operation::REGMEM_TO_FROM_REG: return "To Register/Memory to/from Register";
    case Operation::IMM_TO_REGMEM: return "Immediate to Register/Memory";
    case Operation::IMM_TO_REG: return "Immediate to Register";
    case Operation::MEM_TO_ACC: return "Memory to Accumulator";
    case Operation::ACC_TO_MEM: return "Accumulator to Memory";
    case Operation::ASC_IMM_TO_REGMEM: return "Add/Sub/Comp Immediate to Register/Memory";
    case Operation::ADD_REGMEM_WITH_REG: return "Add Register/Memory w/ Register to Either";
    case Operation::ADD_IMM_TO_ACC: return "Add Immediate to Accumulator";
    case Operation::SUB_REGMEM_WITH_REG: return "Sub Register/Memory w/ Register from Either";
    case Operation::SUB_IMM_FROM_ACC: return "Sub Immediate from Accumulator";
    case Operation::CMP_REGMEM_AND_REG: return "Compare Register/Memory and Register";
    case Operation::CMP_IMM_WITH_ACC: return "Compare Immediate with Accumulator";
    default: return "Unknown Operation";
  }
}


std::string get_opcode_name(Operation operation) {
  switch (operation) {
    case Operation::REGMEM_TO_FROM_REG:
    case Operation::IMM_TO_REGMEM:
    case Operation::IMM_TO_REG:
    case Operation::MEM_TO_ACC:
    case Operation::ACC_TO_MEM:
      return "mov";
    case Operation::ASC_IMM_TO_REGMEM:
      return "asc";  // Not a real name, needs further decoding
    case Operation::ADD_REGMEM_WITH_REG:
    case Operation::ADD_IMM_TO_ACC:
      return "add";
    case Operation::SUB_REGMEM_WITH_REG:
    case Operation::SUB_IMM_FROM_ACC:
      return "sub";
    case Operation::CMP_REGMEM_AND_REG:
    case Operation::CMP_IMM_WITH_ACC:
      return "cmp";
    case Operation::JMP_EQUAL:
      return "je";
    case Operation::JMP_LESS:
      return "jl";
    case Operation::JMP_LESS_OR_EQUAL:
      return "jle";
    case Operation::JMP_BELOW:
      return "jb";
    case Operation::JMP_BELOW_OR_EQUAL:
      return "jbe";
    case Operation::JMP_PARITY:
      return "jp";
    case Operation::JMP_OVERFLOW:
      return "jo";
    case Operation::JMP_SIGN:
      return "js";
    case Operation::JMP_NOT_EQUAL:
      return "jne";
    case Operation::JMP_NOT_LESS:
      return "jnl";
    case Operation::JMP_NOT_LESS_OR_EQUAL:
      return "jnle";
    case Operation::JMP_NOT_BELOW:
      return "jnb";
    case Operation::JMP_NOT_BELOW_OR_EQUAL:
      return "jnbe";
    case Operation::JMP_NOT_PARITY:
      return "jnp";
    case Operation::JMP_NOT_OVERFLOW:
      return "jno";
    case Operation::JMP_NOT_SIGN:
      return "jns";
    case Operation::LOOP:
      return "loop";
    case Operation::LOOPZ:
      return "loopz";
    case Operation::LOOPNZ:
      return "loopnz";
    case Operation::JMP_CX_ZERO:
      return "jcxz";
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


std::string map_generic_to_name(Operation op, std::vector<u8>& instruction) {
  static constexpr std::array<std::pair<u8, const char*>, 3> GENERIC_OP_NAMES {{
    {U8(0b000), "add"},
    {U8(0b101), "sub"},
    {U8(0b111), "cmp"}
  }};
  static const std::unordered_map<u8, std::string> op_names(
      GENERIC_OP_NAMES.begin(), GENERIC_OP_NAMES.end()
  );
  switch (op) {
    case Operation::ASC_IMM_TO_REGMEM: {
      u8 identifier = (instruction[1] & 0b111000) >> 3;
      auto op_name = op_names.find(identifier);
      if (op_name == op_names.end()) {
        std::cerr << std::format("{}: could find find op name\n", __LINE__);
        std::exit(EXIT_FAILURE);
      }
      return op_name->second;
    }
    default:
      std::cerr << std::format("{}: Unhandled case\n", __LINE__);
      std::exit(EXIT_FAILURE);
      break;
  }
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


/* Table 4-08. MOD (Mode) Field Encoding of the 8086 manual. */
u8 num_displacement_bytes(u8 mod, u8 rm) {
	switch (mod) {
		case 0b00:
			return (rm == 0b110) ? 2 : 0;
		case 0b01:
			return 1;
		case 0b10:
			return 2;
		case 0b11:
			return 0;
		default:
			std::cerr << std::format("{}: Bad encoding\n", __LINE__);
			std::exit(EXIT_FAILURE);
	}
}


/* Determines the byte-length of an instruction encoding. */
u8 instruction_size(Operation op, std::vector<u8> &program, u8 index) {

  switch (op) {
    case Operation::JMP_EQUAL:
    case Operation::JMP_LESS:
    case Operation::JMP_LESS_OR_EQUAL:
    case Operation::JMP_BELOW:
    case Operation::JMP_BELOW_OR_EQUAL:
    case Operation::JMP_PARITY:
    case Operation::JMP_OVERFLOW:
    case Operation::JMP_SIGN:
    case Operation::JMP_NOT_EQUAL:
    case Operation::JMP_NOT_LESS:
    case Operation::JMP_NOT_LESS_OR_EQUAL:
    case Operation::JMP_NOT_BELOW:
    case Operation::JMP_NOT_BELOW_OR_EQUAL:
    case Operation::JMP_NOT_PARITY:
    case Operation::JMP_NOT_OVERFLOW:
    case Operation::JMP_NOT_SIGN:
    case Operation::LOOP:
    case Operation::LOOPZ:
    case Operation::LOOPNZ:
    case Operation::JMP_CX_ZERO:
      return 2;
    case Operation::REGMEM_TO_FROM_REG:
    case Operation::ADD_REGMEM_WITH_REG:
    case Operation::SUB_REGMEM_WITH_REG:
    case Operation::CMP_REGMEM_AND_REG: {
      u8 mod = (program[index + 1] & 0b11000000) >> 6;
      u8 rm = program[index + 1] & 0b00000111;
      return 2 + num_displacement_bytes(mod, rm);
    }
    case Operation::ASC_IMM_TO_REGMEM: {
      u8 mod = (program[index + 1] & 0b11000000) >> 6;
      u8 rm = program[index + 1] & 0b00000111;
      bool wide = program[index + 0] & 0b01;
      bool sign_extend = (program[index + 0] & 0b10) >> 1;
      u8 num_data_bytes = wide ? 2 : 1;
			/* Wide and sign extension bit being 1 means that despite this being a
			 * wide instruction, the immediate operand is actually only 1-byte, and
			 * should be sign-extended to 2-bytes. */
			if (wide && sign_extend) {
				num_data_bytes = 1;
			}
      return 2 + num_displacement_bytes(mod, rm) + num_data_bytes;
    }
    case Operation::IMM_TO_REGMEM: {
      u8 mod = (program[index + 1] & 0b11000000) >> 6;
      u8 rm = program[index + 1] & 0b00000111;
      bool wide = program[index + 0] & 0b01;
      u8 num_data_bytes = wide ? 2 : 1;
      return 2 + num_displacement_bytes(mod, rm) + num_data_bytes;
    }
    case Operation::IMM_TO_REG: {
      bool wide = (program[index + 0] & 0b1000) >> 3;
      u8 num_data_bytes = wide ? 2 : 1;
      return 1 + num_data_bytes;
    }
    case Operation::CMP_IMM_WITH_ACC:
    case Operation::SUB_IMM_FROM_ACC:
    case Operation::ADD_IMM_TO_ACC: {
      bool wide = program[index + 0] & 0b01;
      return 1 + (wide ? 2 : 1);
    }
    case Operation::MEM_TO_ACC:
      return 3;
    case Operation::ACC_TO_MEM:
      return 3;
    default:
      std::cerr << std::format("{} Unhandled case\n", __LINE__);
      std::exit(EXIT_FAILURE);
  }
}


std::string disassemble_regmem_to_from_reg(std::string name, std::vector<u8>& instruction) {
  using iterator = std::unordered_map<u8, std::string>::const_iterator;

  u8 high = instruction[0];
  u8 low = instruction[1];
  bool wide = high & 0b01;
  bool direction = (high & 0b10) >> 1;
  u8 mod = (low & 0b11000000) >> 6;
  u8 reg = (low & 0b00111000) >> 3;
  u8 rm = low & 0b00000111;

  auto register_names = get_register_name_map();
  auto rm_values = get_rm_map();

  /* Effective address calculation w/ 16-bit displacement */
  if (mod == 0b10) {
    u8 disp_high = U8(instruction[3]);
    u8 disp_low = U8(instruction[2]);
    i16 disp = I16((disp_high << 8) + disp_low);
    u8 key_reg = reg + (wide << 3);
    iterator reg_it = register_names.find(key_reg);
    iterator rm_it = rm_values.find(rm);
    if (direction) {
      return std::format(
        "{} {}, [{} {:+}]\n",
        name, (*reg_it).second, (*rm_it).second, disp);
    } else {
      return std::format(
        "{} [{} {:+}], {}\n",
        name, (*rm_it).second, disp, (*reg_it).second);
    }

  } else if (mod == 0b01) {
    /* Effective address calculation w/ 8-bit displacement */
    i8 disp = I8(instruction[2]);
    u8 key_reg = reg + (wide << 3);
    iterator reg_it = register_names.find(key_reg);
    iterator rm_it = rm_values.find(rm);
    if (direction) {
      return std::format(
        "{} {}, [{} {:+}]\n",
        name, (*reg_it).second, (*rm_it).second, disp);
    } else {
      return std::format(
        "{} [{} + {}], {}\n",
        name, (*rm_it).second, (int)disp, (*reg_it).second);
    }
  } else if (mod == 0b00) {
    if (rm == 0b110) {
      /* Direct address: 16-bit displacement follows */
      u8 key_reg = reg + (wide << 3);
      u8 disp_high = U8(instruction[3]);
      u8 disp_low = U8(instruction[2]);
      i16 disp = I16((disp_high << 8) + disp_low);
      iterator reg_it = register_names.find(key_reg);
      if (direction) {
        return std::format("{} {}, [{}{}]\n", name, (*reg_it).second, (disp < 0 ? "-" : ""), std::abs(disp));
      } else {
        return std::format("{} [{}{}], {}\n", name, (disp < 0 ? "-" : ""), std::abs(disp), (*reg_it).second);
      }
    } else {
      /* No displacement */
      u8 key_reg  = reg + (wide << 3);
      iterator reg_it = register_names.find(key_reg);
      iterator rm_it = rm_values.find(rm);
      if (reg_it == register_names.end() || rm_it == rm_values.end()) {
        std::cerr << std::format("{}: Bad register encoding\n", __LINE__);
        std::exit(EXIT_FAILURE);
      }
      if (direction) {
        return std::format(
          "{} {}, [{}]\n",
          name, (*reg_it).second, (*rm_it).second);
      } else {
        return std::format(
          "{} [{}], {}\n",
          name, (*rm_it).second, (*reg_it).second);
      }
    }
  } else if (mod == 0b11) {
    /* Register to Register */
    u8 key_rm = rm + (wide << 3);
    u8 key_reg  = reg + (wide << 3);
    iterator source_it {};
    iterator destination_it {};
    if (direction) {
      source_it = register_names.find(key_rm);
      destination_it = register_names.find(key_reg);
    } else {
      destination_it = register_names.find(key_reg);
      source_it = register_names.find(key_rm);
    }
    if (source_it == register_names.end() || destination_it == register_names.end()) {
      std::cerr << std::format("{}: Bad register encoding\n", __LINE__);
      std::exit(EXIT_FAILURE);
    }
    return std::format(
      "{} {}, {}\n", name, (*source_it).second, (*destination_it).second
    );
  }
  return std::format("Unknown: regmem_to_from_reg, mod = {}, rm = {}\n", mod, rm);
}


std::string disassemble_imm_to_regmem(std::string name, std::vector<u8>& instruction) {

  bool wide = instruction[0] & 0b01;
  bool sign_extend = (instruction[0] & 0b10) >> 1;
  u8 mod = (instruction[1] & 0b11000000) >> 6;
  u8 rm = instruction[1] & 0b111;

  auto rm_it = get_rm_map().find(rm);
  std::string length = wide ? "word" : "byte";

  if (name == GENERIC_OP) {
    name = map_generic_to_name(Operation::ASC_IMM_TO_REGMEM, instruction);
  } else {
    // The Immediate to register/memory operation does not have the S bit
    sign_extend = false;
  }

  switch(mod) {

    case 0b00: {
      i16 data {};
      if (rm == 0b110) {
        /* Memory mode, 16-bit displacement follows */
        i16 disp = I16(instruction[3] << 8) + instruction[2];
        if (wide) {
          if (sign_extend) { 
            data = I16(instruction[4]);
          } else {
            data = I16(instruction[5] << 8) + instruction[4];
          }
        } else {
          data = I16(instruction[4]);
        }
			  return std::format("{} {} [{}], {}\n", name, length, disp, data);
      } else {
        /* Memory mode, no displacement */
        if (wide) {
          if (sign_extend) { 
            data = I16(instruction[2]);
          } else {
            data = I16(instruction[3] << 8) + instruction[2];
          }
        } else {
          data = I16(instruction[2]);
        }
			  return std::format("{} {} [{}], {}\n", name, length, rm_it->second, data);
      }
    }

    case 0b01: {
      i8 disp = I8(instruction[2]);
      i16 data {};
      if (wide) {
        if (sign_extend) { 
          data = I16(instruction[3]);
        } else {
          data = I16(instruction[4] << 8) + instruction[3];
        }
      } else {
        data = I16(instruction[3]);
      }
			return std::format("{} {} [{} {} {}], {}\n",
				name, length, rm_it->second, (disp < 0 ? "-" : "+"), disp, data);
    }

    case 0b10: {
      i16 disp = I16((instruction[3] << 8) + instruction[2]);
      i16 data {};
      if (wide) {
        if (sign_extend) { 
          data = I16(instruction[4]);
        } else {
          /* 16-bit extension */
          data = I16(instruction[5] << 8) + instruction[4];
        }
      } else {
        /* 8-bit extension */
        data = I16(instruction[4]);
      }
    	return std::format("{} {} [{} {} {}], {}\n",
				name, length, rm_it->second, (disp < 0 ? "-" : "+"), disp, data);
    }

    case 0b11: {
      /* No displacement */
      u8 key_rm = rm + (wide << 3);
      auto dest = get_register_name_map().find(key_rm);
      if (dest == get_register_name_map().end()) {
        std::cerr << std::format("{}: Register encoding not found\n", __LINE__);
        std::exit(EXIT_FAILURE);
      }
      i16 data {};
      if (wide) {
        if (sign_extend) { 
          data = I16(instruction[2]);
        } else {
				  data = I16(instruction[2] << 8) + instruction[1];
        }
      } else {
        data = I16(instruction[2]);
      }
      std::string op_name = map_generic_to_name(Operation::ASC_IMM_TO_REGMEM , instruction);
      return std::format("{} {}, {}\n", op_name, dest->second, data);
    }

  };
  return std::format("{}: Unknown imm to regmem\n", __LINE__);
}


std::string disassemble_imm_to_reg(std::string name, std::vector<u8>& instruction) {
  using iterator = std::unordered_map<u8, std::string>::const_iterator;
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
    i16 data = I16(instruction[1] + (instruction[2] << 8));
    return std::format("{} {}, {}\n", name, reg_it->second, data);
  } else {
    i8 data = I8(instruction[1]);
    return std::format("{} {}, {}\n", name, reg_it->second, data);
  }
}


std::string disassemble_mem_to_acc(std::string name, std::vector<u8>& instruction) {
  bool wide = instruction[0] & 0b01;
  if (wide) {
    u16 addr = U16((instruction[2] << 8) + instruction[1]);
    return std::format("{} ax, [{}]\n", name, addr);
  } else {
    u8 addr = U8(instruction[1]);
    return std::format("{} al, [{}]\n", name, addr);
  }
}


std::string disassemble_acc_to_mem(std::string name, std::vector<u8>& instruction) {
  bool wide = instruction[0] & 0b01;
  if (wide) {
    u16 addr = U16((instruction[2] << 8) + instruction[1]);
    return std::format("{} [{}], ax\n", name, addr);
  } else {
    u8 addr = U8(instruction[1]);
    return std::format("{} [{}], al\n", name, addr);
  }
}


std::string disassemble_add_to_acc(std::string name, std::vector<u8>& instruction) {
  bool wide = instruction[0] & 0b01;
  i16 data {};
  std::string dest {};
  if (wide) {
    data = I16((instruction[2] << 8) + instruction[1]);
    dest = "ax";
  } else {
    data = I8(instruction[1]);
    dest = "al";
  }
  return std::format("{} {}, {}\n", name, dest, data);
}


std::string disassemble_jmp(std::string name, std::vector<u8>& instruction) {
  return std::format("{} {}\n", name, I8(instruction[1]));
}


std::string disassemble(std::string name, Operation op, std::vector<u8> &instruction) {

  switch (op) {
    case Operation::JMP_EQUAL:
    case Operation::JMP_LESS:
    case Operation::JMP_LESS_OR_EQUAL:
    case Operation::JMP_BELOW:
    case Operation::JMP_BELOW_OR_EQUAL:
    case Operation::JMP_PARITY:
    case Operation::JMP_OVERFLOW:
    case Operation::JMP_SIGN:
    case Operation::JMP_NOT_EQUAL:
    case Operation::JMP_NOT_LESS:
    case Operation::JMP_NOT_LESS_OR_EQUAL:
    case Operation::JMP_NOT_BELOW:
    case Operation::JMP_NOT_BELOW_OR_EQUAL:
    case Operation::JMP_NOT_PARITY:
    case Operation::JMP_NOT_OVERFLOW:
    case Operation::JMP_NOT_SIGN:
    case Operation::LOOP:
    case Operation::LOOPZ:
    case Operation::LOOPNZ:
    case Operation::JMP_CX_ZERO:
      return disassemble_jmp(name, instruction);
    case Operation::CMP_REGMEM_AND_REG:
    case Operation::SUB_REGMEM_WITH_REG:
    case Operation::ADD_REGMEM_WITH_REG:
    case Operation::REGMEM_TO_FROM_REG:
      return disassemble_regmem_to_from_reg(name, instruction);
    case Operation::ASC_IMM_TO_REGMEM:
    case Operation::IMM_TO_REGMEM:
      return disassemble_imm_to_regmem(name, instruction);
    case Operation::IMM_TO_REG:
      return disassemble_imm_to_reg(name, instruction);
    case Operation::MEM_TO_ACC:
      return disassemble_mem_to_acc(name, instruction);
    case Operation::ACC_TO_MEM:
      return disassemble_acc_to_mem(name, instruction);
    case Operation::CMP_IMM_WITH_ACC:
    case Operation::SUB_IMM_FROM_ACC:
    case Operation::ADD_IMM_TO_ACC:
      return disassemble_add_to_acc(name, instruction);
    default:
      std::cerr << std::format("{}: Unhandled case\n", __LINE__);
      std::exit(EXIT_FAILURE);
  }
  return "unk\n";
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
    std::vector<u8> instruction {};

    Operation operation = match_opcode(program_data[program_cursor]);
    std::string name = get_opcode_name(operation);

    unsigned int size = instruction_size(operation, program_data, program_cursor);

    for (unsigned int i = 0; i < size; i++) {
      instruction.push_back(program_data[program_cursor + i]);
    }
    program_cursor += size;

    std::cout << disassemble(name, operation, instruction);
  }
  
  std::exit(EXIT_SUCCESS);
}
