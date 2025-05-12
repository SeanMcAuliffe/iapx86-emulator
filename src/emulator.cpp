#include <cstdlib>
#include "stdio.h"
#include <iostream>
#include <format>
#include <filesystem>
#include <fstream>
#include <locale>
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


const std::string GENERIC_OP = "asc";


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
	ADD_REGMEM_WITH_REG,
	ASC_IMM_TO_REGMEM,
	ADD_IMM_TO_ACC,
  COUNT 
};


struct Opcode {
  Operation operation;
  u8 bits;
  u8 length;
};


constexpr std::array<Opcode, SIZE(Operation::COUNT)> opcodes {{
  {Operation::REGMEM_TO_FROM_REG, 0b100010, 6},
  {Operation::IMM_TO_REGMEM, 0b1100011, 7},
  {Operation::IMM_TO_REG, 0b1011, 4},
  {Operation::MEM_TO_ACC, 0b1010000, 7},
  {Operation::ACC_TO_MEM, 0b1010001, 7},
  {Operation::ADD_REGMEM_WITH_REG, 0b0, 6},
  {Operation::ASC_IMM_TO_REGMEM, 0b100000, 6},
  {Operation::ADD_IMM_TO_ACC, 0b10, 7},
}};


std::string to_string(Operation operation) {
  switch (operation) {
    case Operation::REGMEM_TO_FROM_REG: return "To Register/Memory to/from Register";
    case Operation::IMM_TO_REGMEM: return "Immediate to Register/Memory";
    case Operation::IMM_TO_REG: return "Immediate to Register";
    case Operation::MEM_TO_ACC: return "Memory to Accumulator";
    case Operation::ACC_TO_MEM: return "Accumulator to Memory";
    case Operation::ADD_REGMEM_WITH_REG: return "Add/Sub/Comp Register/Memory w/ Register to Either";
    case Operation::ASC_IMM_TO_REGMEM: return "Add/Sub/Comp Immediate to Register/Memory";
    case Operation::ADD_IMM_TO_ACC: return "Add/Sub/Comp Immediate to Accumulator";
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
    case O::ADD_REGMEM_WITH_REG:
    case O::ADD_IMM_TO_ACC:
			return "add";
    case O::ASC_IMM_TO_REGMEM:
			return "asc";  // Not a real name, needs further decoding
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


u8 additional_bytes(Operation op, std::vector<u8> &instruction, bool &more) {
  switch (op) {
    case Operation::REGMEM_TO_FROM_REG:
		case Operation::ADD_REGMEM_WITH_REG:
      more = true;
      return 1;
		case Operation::ASC_IMM_TO_REGMEM:
    case Operation::IMM_TO_REGMEM:
      more = true;
      return 1;
    case Operation::IMM_TO_REG: {
			bool wide = (instruction[0] & 0b1000) >> 3;
      more = false;
      return wide ? 2 : 1;
		}
		case Operation::ADD_IMM_TO_ACC: {
			bool wide = instruction[0] & 0b01;
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
	bool sign_bit = (high & 0b10) >> 1;
  u8 mod = (low & 0b11000000) >> 6;
  u8 rm = low & 0b00000111;
  switch (op) {
		case Operation::ADD_REGMEM_WITH_REG:
    case Operation::REGMEM_TO_FROM_REG:
			if (mod == 0b01) {
				return 1;
			}
			if (mod == 0b10) {
				return 2;
			}
			if (mod == 0b11) {
				return 0;
			}
			if (mod == 0b00) {
				if (rm == 0b110) {
					return 2;
				} else {
					return 0;
				}
			}
			std::cerr << std::format(
				"{}: Unhandled case, mod = {}, rm = {}\n", __LINE__, mod, rm
			);
      std::exit(EXIT_FAILURE);
      break;
    case Operation::IMM_TO_REGMEM: {
			u8 data_bytes = wide ? 2 : 1;
			if (mod == 0b01) {
				return 1 + data_bytes;
			}
			if (mod == 0b10) {
				return 2 + data_bytes;
			}
			if (mod == 0b11) {
				return data_bytes;
			}
			if (mod == 0b00) {
				if (rm == 0b110) {
					return 2 + data_bytes;
				} else {
					return data_bytes;
				}
			}
			std::cerr << std::format("{}: Unhandled case\n", __LINE__);
			std::exit(EXIT_FAILURE);
		}
		case Operation::ASC_IMM_TO_REGMEM: {
			u8 data_bytes = sign_bit ? 0 : 1;
			data_bytes += wide ? 1 : 0;
			if (mod == 0b01) {
				return 1 + data_bytes;
			}
			if (mod == 0b10) {
				return 2 + data_bytes;
			}
			if (mod == 0b11) {
				return data_bytes;
			}
			if (mod == 0b00) {
				if (rm == 0b110) {
					return 2 + data_bytes;
				} else {
					return data_bytes;
				}
			}
			std::cerr << std::format("{}: Unhandled case\n", __LINE__);
			std::exit(EXIT_FAILURE);
		}

    default:
      std::cerr << std::format("{}: Unhandled case\n", __LINE__);
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
				"{} {}, [{} {} {}]\n",
				name, (*reg_it).second, (*rm_it).second, (disp < 0 ? "-" : "+"), std::abs(disp));
		} else {
			return std::format(
				"{} [{} {} {}], {}\n",
				name, (*rm_it).second, (disp < 0 ? "-" : "+"), std::abs(disp), (*reg_it).second);
		}

	} else if (mod == 0b01) {
		/* Effective address calculation w/ 8-bit displacement */
		i8 disp = I8(instruction[2]);
		u8 key_reg = reg + (wide << 3);
		iterator reg_it = register_names.find(key_reg);
		iterator rm_it = rm_values.find(rm);
		if (direction) {
			return std::format(
				"{} {}, [{} {} {}]\n",
				name, (*reg_it).second, (*rm_it).second, (disp < 0 ? "-" : "+"), std::abs(disp));
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
				return std::format("{} {} [{}{}]\n", name, (*reg_it).second, (disp < 0 ? "-" : ""), std::abs(disp));
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
	bool sign_extension = (instruction[0] & 0b10) >> 1;
	u8 mod = (instruction[1] & 0b11000000) >> 6;
	u8 rm = instruction[1] & 0b111;

	auto rm_it = get_rm_map().find(rm);
	std::string length = wide ? "word" : "byte";

	if (name == GENERIC_OP) {
		name = map_generic_to_name(Operation::ASC_IMM_TO_REGMEM, instruction);
	}

	#if defined DEBUG
	std::cout << std::format("name: {}, mod: {}, rm: {}, wide: {}, sign extension: {}\n",
		name, mod, rm, wide, sign_extension);
	for (auto byte : instruction) {
		std::cout << std::hex << (int)byte << " ";
	}
	std::cout << std::dec << std::endl;
	#endif

	switch(mod) {

		case 0b00: {
			i16 data {};
			if (wide) {
				data = I16(instruction[3] << 8) + instruction[2];
			} else {
				data = I16(instruction[2]);
			}
			i16 addr = I16((instruction[3] << 8) + instruction[2]);
			std::string dest = (rm == 0b110) ? std::format("{}", addr): rm_it->second;
			if (sign_extension) {
				return std::format("{} [{}], {} {}\n", name, dest, length, data);
			} else {
				return std::format("{} {} [{}], {}\n", name, length, dest, data);
			}
		}

		case 0b01: {
			i8 disp = I8(instruction[2]);
			i16 data {};
			if (wide) {
				data = I16(instruction[4] << 8) + instruction[3];
			} else {
				data = I16(instruction[3]);
			}
			if (sign_extension) {
				return std::format("{} [{} {} {}], {} {}\n",
					name, rm_it->second, (disp < 0 ? "-" : "+"), disp, length, data);
			} else {
				return std::format("{} {} [{} {} {}], {}\n",
					name, length, rm_it->second, (disp < 0 ? "-" : "+"), disp, data);
			}
		}

		case 0b10: {
			i16 disp = I16((instruction[3] << 8) + instruction[2]);
			i16 data {};
			if (wide) {
				data = I16(instruction[5] << 8) + instruction[4];
			} else {
				data = I16(instruction[4]);
			}
			if (sign_extension) {
				return std::format("{} [{} {} {}], {} {}\n",
					name, rm_it->second, (disp < 0 ? "-" : "+"), disp, length, data);
			} else {
				return std::format("{} {} [{} {} {}], {}\n",
					name, length, rm_it->second, (disp < 0 ? "-" : "+"), disp, data);
			}
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
			if (!sign_extension) {
				data = I16(instruction[3] << 8) + instruction[2];
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
		return std::format("{}, {}, {}\n", name, reg_it->second, data);
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
	#if defined DEBUG
	std::cout << std::format("name: {}, wide: {}, dest: {}, data: {}\n",
		name, wide, dest, data);
	for (auto byte : instruction) {
		std::cout << std::hex << (int)byte << " ";
	}
	std::cout << std::dec << std::endl;
	#endif
	return std::format("{} {}, {}\n", name, dest, data);
}

std::string disassemble(std::string name, Operation op, std::vector<u8> &instruction) {

  switch (op) {
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
