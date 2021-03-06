#ifndef PARSED_OUTPUT_HPP
#define PARSED_OUTPUT_HPP

#include <vector>
#include <map>
#include "location.hh"

namespace f64_assembler
{
    struct SemanticException : std::runtime_error
    {
      SemanticException (const yy::location& l, const std::string& m)
		  : std::runtime_error(m),
	  	    location(l) { }
			
      yy::location location;
    };
	
	class ParsedInstruction
	{
	public:
		enum { INSTRUCTION_BITS = 16 };
		typedef std::map<std::string, unsigned short> OffsetTable;
		
		virtual int instructionLength() const { return INSTRUCTION_BITS; }
		void setOffset(unsigned short newOffset) { byteOffset = newOffset; }
		unsigned short offset() const { return byteOffset; }
		
		virtual void resolve(OffsetTable& offsetTable) { /* defaults to no-op */ }
		virtual void pushOffset(OffsetTable& offsetTable) { /* defaults to no-op */ }
		
		unsigned short instruction() const { return instructionBytes; }
		
	protected:
		ParsedInstruction(unsigned short data, yy::location& loc)
			: instructionBytes(data),
			  fileLocation(loc)
		{
			// empty
		}
		
		unsigned short instructionBytes;
		yy::location fileLocation;
		
	private:
		unsigned short byteOffset;
	};
	
	// Simple wrapper to support the .raw psuedo-op.
	class RawInstruction : public ParsedInstruction
	{
	public:
		RawInstruction(unsigned short instruction, yy::location& loc)
			: ParsedInstruction(instruction, loc) { }
	};
	
	class JumpDestination : public ParsedInstruction
	{
	public:
		JumpDestination(std::string &label, yy::location& loc);
		
		virtual int instructionLength() const { return 0; /* psuedo instruction */ }
		virtual void pushOffset(OffsetTable& offsetTable);
		
	private:
		std::string destLabel;
	};
	
	// One argument instruction
	template<int prefix_len>
	class OneArgInstruction : public ParsedInstruction
	{
	public:
		OneArgInstruction(unsigned short prefix, unsigned short param1, yy::location& loc)
			: ParsedInstruction(
				(prefix << PREFIX_SHIFT) | 
				(param1 & PARAM1_MASK),
				loc
			  )
		{
			// empty
		}
		
		static bool DoesFinalArgumentOverflow(short param)
		{
			int final_arg_bits = ParsedInstruction::INSTRUCTION_BITS - prefix_len;
			int final_arg_max = (1 << (final_arg_bits + 1)) - 1;
			int final_arg_min = ~final_arg_max + 1;
	
			return (param > final_arg_max || param < final_arg_min);
		}
		
	private:
		enum 
		{
			PREFIX_SHIFT = INSTRUCTION_BITS - prefix_len,
			PARAM1_MASK = ~(0xFFFF << PREFIX_SHIFT)
		};
	};
	
	class BranchInstruction : public OneArgInstruction<4>
	{
	public:
		BranchInstruction(unsigned short prefix, std::string& branchDestination, yy::location& loc);
		
		virtual void resolve(OffsetTable& offsetTable);
		
	private:
		std::string branchName;
	};
	
	// Three argument instruction
	template<int prefix_len, int param1_len, int param2_len>
	class ThreeArgInstruction : public ParsedInstruction
	{
	public:
		ThreeArgInstruction(unsigned short prefix, unsigned short param1, unsigned short param2, unsigned short param3, yy::location& loc)
			: ParsedInstruction(
				(prefix << PREFIX_SHIFT) | 
				((param1 << PARAM1_SHIFT) & PARAM1_MASK) | 
				((param2 << PARAM2_SHIFT) & PARAM2_MASK) |
				(param3 & PARAM3_MASK),
				loc
			  )
		{
			// empty
		}
		
		static bool DoesFinalArgumentOverflow(short param)
		{
			int final_arg_bits = ParsedInstruction::INSTRUCTION_BITS - prefix_len - param1_len - param2_len;
			int final_arg_max = (1 << (final_arg_bits + 1)) - 1;
			int final_arg_min = ~final_arg_max + 1;
	
			return (param > final_arg_max || param < final_arg_min);
		}
		
	private:
		enum
		{
			PREFIX_SHIFT = INSTRUCTION_BITS - prefix_len,
			PARAM1_SHIFT = PREFIX_SHIFT - param1_len,
			PARAM1_MASK = ~(0xFFFF << PREFIX_SHIFT) & (0xFFFF << (PREFIX_SHIFT - param2_len)),
			PARAM2_SHIFT = PARAM1_SHIFT - param2_len,
			PARAM2_MASK = ~(0xFFFF << PARAM1_SHIFT) & (0xFFFF << (PARAM1_SHIFT - param2_len)),
			PARAM3_MASK = ~(0xFFFF << PARAM2_SHIFT)
		};
	};
	
	class InstructionFactory
	{
	public:
		enum BranchType
		{
			UNCONDITIONAL,
			GREATER_THAN,
			GREATER_EQUAL,
			LESS_THAN,
			LESS_EQUAL,
			ZERO
		};
		
		enum LoadStoreType
		{
			LOAD,
			STORE
		};
		
		enum AluInstruction
		{
			ADD,
			AND,
			OR,
			NOT
		};
		
		enum ShiftInstruction
		{
			SHIFT_LEFT,
			SHIFT_RIGHT
		};
		
		static ParsedInstruction* MakeJumpDestination(std::string label, yy::location& loc);
		static ParsedInstruction* MakeBranchInstruction(BranchType type, std::string label, yy::location& loc);
		static ParsedInstruction* MakeLoadStoreInstruction(LoadStoreType type, short register1, short register2, short offset, yy::location& loc);
		static ParsedInstruction* MakeAluInstructionWithConstOperand(AluInstruction type, short register1, short register2, short offset, yy::location& loc);
		static ParsedInstruction* MakeAluInstructionWithRegOperand(AluInstruction type, short register1, short register2, short register3, yy::location& loc);
		static ParsedInstruction* MakeShiftInstruction(ShiftInstruction type, short register1, short register2, short offset, yy::location& loc);
		static ParsedInstruction* MakeRawInstruction(unsigned short instruction, yy::location& loc);
		
		static short RegisterNumberFromName(std::string& register_name);
	};
};

#endif // PARSED_OUTPUT_HPP