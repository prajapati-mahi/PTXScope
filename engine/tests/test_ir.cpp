#include "test_framework.hpp"
#include "ptxscope/ir.hpp"

using namespace ptxscope;

TEST_CASE(IRTest, OperandProperties) {
    auto op_reg = Operand::make_reg("%r1", DataType::U32);
    auto op_int = Operand::make_int(42, DataType::U32);
    auto op_mem = Operand::make_mem("%rd0", 8, DataType::U64);

    EXPECT_EQ(op_reg.to_ptx(), "%r1");
    EXPECT_EQ(op_int.to_ptx(), "42");
    EXPECT_EQ(op_mem.to_ptx(), "[%rd0+8]");
}

TEST_CASE(IRTest, DefUseDetection) {
    Instruction inst;
    inst.opcode = Opcode::ADD;
    inst.opcode_str = "add";
    inst.type = DataType::U32;
    inst.operands = {
        Operand::make_reg("%r1", DataType::U32),
        Operand::make_reg("%r2", DataType::U32),
        Operand::make_reg("%r3", DataType::U32)
    };

    EXPECT_TRUE(inst.get_def_reg().has_value());
    EXPECT_EQ(*inst.get_def_reg(), "%r1");

    auto uses = inst.get_use_regs();
    EXPECT_EQ(uses.size(), 2);
    EXPECT_EQ(uses[0], "%r2");
    EXPECT_EQ(uses[1], "%r3");
    EXPECT_FALSE(inst.has_side_effects());
}

TEST_CASE(IRTest, SideEffectDetection) {
    Instruction st_inst;
    st_inst.opcode = Opcode::ST;
    st_inst.opcode_str = "st";
    st_inst.type = DataType::U32;
    EXPECT_TRUE(st_inst.has_side_effects());
}
