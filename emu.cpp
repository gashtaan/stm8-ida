#include "st8.hpp"
//#include <frame.hpp>

static int flow;
//------------------------------------------------------------------------
static void process_immediate_number(const insn_t &insn, int n)
{
	set_immd(insn.ea);
	if (is_defarg(insn.flags, n)) return;
	switch (insn.itype)
	{
	case ST8_bres:
	case ST8_bset:
	case ST8_btjf:
	case ST8_btjt:
		op_dec(insn.ea, n);
		break;
	}
}

//----------------------------------------------------------------------
ea_t calc_mem(const insn_t &insn, ea_t ea)
{
	return to_ea(insn.cs, ea);
}

//----------------------------------------------------------------------
static void process_operand(const insn_t &insn, const op_t &x, bool isload)
{
	ea_t ea;
	switch (x.type)
	{
	case o_reg:
	case o_phrase:
		break;

	case o_imm:
		if (!isload) interr(insn, "emu");
		process_immediate_number(insn, x.n);
		if (is_off(insn.flags, x.n))
			insn.add_off_drefs(x, dr_O, 0);
		break;

	case o_mem:
		if (!is_forced_operand(insn.ea, x.n))
		{
			ea = calc_mem(insn, x.addr);
			insn.create_op_data(ea, x.offb, x.dtype);
			dref_t dref = isload || (insn.auxpref & aux_indir) ? dr_R : dr_W;
			insn.add_dref(ea, x.offb, dref);
		}
		break;

	case o_near:
		{
			cref_t ftype = fl_JN;
			ea = calc_mem(insn, x.addr);
			if (has_insn_feature(insn.itype, CF_CALL))
			{
				if (!func_does_return(ea))
					flow = false;
				ftype = fl_CN;
			}
			insn.add_cref(ea, x.offb, ftype);
		}
		break;

	case o_displ:
		process_immediate_number(insn, x.n);
		if (is_off(insn.flags, x.n) && !is_forced_operand(insn.ea, x.n))
			insn.add_off_drefs(x, isload ? dr_R : dr_W, OOF_ADDR);
		break;

	default:
		interr(insn, "emu");
	}
}


//----------------------------------------------------------------------
int idaapi emu(const insn_t &insn)
{
	uint32 Feature = insn.get_canon_feature();

	flow = ((Feature & CF_STOP) == 0);

	if (Feature & CF_USE1) 
		process_operand(insn, insn.Op1, true);
	if (Feature & CF_USE2) 
		process_operand(insn, insn.Op2, true);
	if (Feature & CF_USE3) 
		process_operand(insn, insn.Op3, true);
	if (Feature & CF_CHG1) 
		process_operand(insn, insn.Op1, false);
	if (Feature & CF_CHG2)
		process_operand(insn, insn.Op2, false);
	if (Feature & CF_CHG3)
		process_operand(insn, insn.Op3, false);

	//
	// 确定是否应该执行下一条指令, 关键, 这里处理不对, 一次只会分析一条指令了
	//
	if (segtype(insn.ea) == SEG_XTRN) 
		flow = 0;
	if (flow) 
		insn.add_cref(insn.ea + insn.size, 0, fl_F);

	return 1;
}

//----------------------------------------------------------------------
int is_jump_func(const func_t * /*pfn*/, ea_t *jump_target)
{
	*jump_target = BADADDR;
	return 1; // means "no"
}

//----------------------------------------------------------------------
int may_be_func(void)           // can a function start here?
								// arg: none, the instruction is in 'cmd'
								// returns: probability 0..100
								// 'cmd' structure is filled upon the entrace
								// the idp module is allowed to modify 'cmd'
{
	//  if ( insn.itype == H8_push && isbp(insn.Op1.reg) ) return 100;  // push.l er6
	return 0;
}

//----------------------------------------------------------------------
int is_sane_insn(const insn_t &insn, int /*nocrefs*/)
{
	if (insn.itype == ST8_nop)
	{
		for (int i = 0; i < 8; i++)
			if (get_word(insn.ea - i * 2) != 0) return 1;
		return 0; // too many nops in a row
	}
	return 1;
}

//----------------------------------------------------------------------
// Known STM8 switch idioms:
//  Switch statements are straightforward:
//     cp      a, #$E        ; Arithmetic Compare
//     jrnc    out_of_bounds ; Jump if C = 0
//     clrw    x             ; x = 0
//     ld      xl, a         ; x |= a & 0xff
//     sllw    x             ; index = x * sizeof(u16)
//     ldw     x, ($F693,x)  ; x = *(0xf693 + index)
//     jp      (x)           ; Absolute Jump
//   out_of_bounds:
//     jra     return        ; Jump relative always
//  Indirect calls are less code but harder to resolve,
//  as they lookup the target address from memory that is
//  modified during runtime.
//     ld      a, byte_1E0   ; a = index
//     clrw    y             ; y = 0
//     ld      yl, a         ; y |= a & 0xff
//     sllw    y             ; y *= sizeof(u16)
//     ldw     y, (7,y)      ; t = *(7 + y) // data at 7 is changed at runtime
//     call    (y)           ; Call subroutine
#include <jptcmn.cpp>

class stm8_jump_pattern_t : public jump_pattern_t
{
public:
	stm8_jump_pattern_t(switch_info_t *si)
		: jump_pattern_t(si, roots, depends)
		, indirect_register(-1)
		, index_register(-1)
	{
	}

	static char const roots[];
	static char const depends[][2];

	int indirect_register;
	int index_register;

	bool jpi0()
	{
		bool valid = insn.itype == ST8_jp;
		if (valid)
			indirect_register = insn.Op1.reg;
		return valid;
	}
	bool jpi1()
	{
		bool valid = insn.itype == ST8_ldw && insn.Op1.reg == indirect_register;
		if (valid)
			si->jumps = insn.Op2.addr;
		return valid;
	}
	bool jpi2()
	{
		return insn.itype == ST8_sllw && insn.Op1.reg == indirect_register;
	}
	bool jpi3()
	{
		uint16 dst_reg = insn.Op1.reg;
		if ((indirect_register == regnum_t::X) && (dst_reg != regnum_t::XL))
			return false;
		else if ((indirect_register == regnum_t::Y) && (dst_reg != regnum_t::YL))
			return false;

		index_register = insn.Op2.reg;

		return insn.itype == ST8_ld;
	}
	bool jpi4()
	{
		return insn.itype == ST8_clrw && insn.Op1.reg == indirect_register;
	}
	bool jpi6()
	{
		bool valid = insn.itype == ST8_cp && insn.Op1.reg == index_register;
		if (valid)
			si->ncases = insn.Op2.value;
		return valid;
	}
};

char const stm8_jump_pattern_t::roots[] = { 1 };
char const stm8_jump_pattern_t::depends[][2] =
{
	{ 1 },    // jp      (x)
	{ 2 },    // ldw     x, ($F693,x)
	{ 3 },    // sllw    x
	{ 4, 6 }, // ld      xl, a
	{ 0 },    // clrw    x
	{ 0 },    // jrnc    out_of_bounds
	{ 0 }     // cp      a, #$E
};

bool idaapi stm8_is_switch(switch_info_t *si, const insn_t &insn)
{
	return stm8_jump_pattern_t(si).match(insn);
}

//----------------------------------------------------------------------
int idaapi stm8_is_align_insn(ea_t ea)
{
	insn_t insn;
	if (decode_insn(&insn, ea) < 1)
		return 0;
	switch (insn.itype)
	{
	case ST8_nop:
		break;
	default:
		return 0;
	}
	return insn.size;
}

