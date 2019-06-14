#include "st8.hpp"
#include <struct.hpp>
#include <kernwin.hpp>


//================================================================
class out_stm8_t : public outctx_t
{
	void outreg(int r) { out_register(ph.reg_names[r]); }

public:
	void outmem(const op_t &x, ea_t ea);
	//bool outbit(ea_t ea, int bit);
	bool out_operand(const op_t &x);
	void out_insn(void);
	//void out_mnem(void);
};

DECLARE_OUT_FUNCS_WITHOUT_OUTMNEM(out_stm8_t)

//----------------------------------------------------------------------
void out_stm8_t::outmem(const op_t &x, ea_t ea)
{
	qstring buf;
	if (get_name_expr(&buf, insn.ea + x.offb, x.n, ea, BADADDR) <= 0)
	{
		const ioport_t *p = find_sym(x.addr);
		if (p == NULL)
		{
			out_tagon(COLOR_ERROR);
			out_btoa(x.addr, 16);
			out_tagoff(COLOR_ERROR);
			//QueueSet(Q_noName, insn.ea);
		}
		else
		{
			out_line(p->name.c_str(), COLOR_IMPNAME);
		}
	}
	else
	{
		bool complex = strchr(buf.c_str(), '+') || strchr(buf.c_str(), '-');
		if (complex) out_symbol(ash.lbrace);
		out_line(buf.c_str());
		if (complex) out_symbol(ash.rbrace);
	}
}

//----------------------------------------------------------------------
bool out_stm8_t::out_operand(const op_t &x)
{
	switch (x.type)
	{

	case o_void:
		return 0;

	case o_reg:
		outreg(x.reg);
		break;

	case o_imm:
		out_symbol('#');
		out_value(x, OOFS_IFSIGN | OOFW_IMM);
		break;

	case o_displ:
		// o_displ Short     Direct   Indexed  ld A,($10,X)             00..1FE                + 1
		// o_displ Long      Direct   Indexed  ld A,($1000,X)           0000..FFFF             + 2
		out_symbol('(');
		out_value(x, OOFS_IFSIGN | OOF_ADDR | OOFW_IMM);
		out_symbol(',');
		outreg(x.reg);
		out_symbol(')');
		break;

	case o_phrase:
		out_symbol('(');
		outreg(x.reg);
		out_symbol(')');
		break;

	case o_mem:
		// o_mem   Short     Direct            ld A,$10                 00..FF                 + 1
		// o_mem   Long      Direct            ld A,$1000               0000..FFFF             + 2
		// o_mem   Short     Indirect          ld A,[$10]               00..FF     00..FF byte + 2
		// o_mem   Long      Indirect          ld A,[$10.w]             0000..FFFF 00..FF word + 2
		// o_mem   Short     Indirect Indexed  ld A,([$10],X)           00..1FE    00..FF byte + 2
		// o_mem   Long      Indirect Indexed  ld A,([$10.w],X)         0000..FFFF 00..FF word + 2
		// o_mem   Relative  Indirect          jrne [$10]               PC+/-127   00..FF byte + 2
		// o_mem   Bit       Direct            bset $10,#7              00..FF                 + 1
		// o_mem   Bit       Indirect          bset [$10],#7            00..FF     00..FF byte + 2
		// o_mem   Bit       Direct   Relative btjt $10,#7,skip         00..FF                 + 2
		// o_mem   Bit       Indirect Relative btjt [$10],#7,skip       00..FF     00..FF byte + 3
		if (insn.auxpref & aux_index) out_symbol('(');
		if (insn.auxpref & aux_indir) out_symbol('[');
		outmem(x, calc_mem(insn, x.addr));
		if (insn.auxpref & aux_long) out_symbol('.');
		if (insn.auxpref & aux_long) out_symbol('w');
		if (insn.auxpref & aux_extd) out_symbol('.');
		if (insn.auxpref & aux_extd) out_symbol('e');
		if (insn.auxpref & aux_indir) out_symbol(']');
		if (insn.auxpref & aux_index)
		{
			out_symbol(',');
			outreg(x.reg);
			out_symbol(')');
		}
		break;

	case o_near:
		outmem(x, calc_mem(insn, x.addr));
		break;

	default:
		interr(insn, "out");
		break;
	}
	return 1;
}

//----------------------------------------------------------------------
void out_stm8_t::out_insn(void)
{
	out_mnemonic();
	bool showOp1 = insn.Op1.shown();
	if (showOp1)
		out_one_operand(0);

	if (insn.Op2.type != o_void)
	{
		out_symbol(',');
		out_char(' ');
		out_one_operand(1);
	}
	if (insn.Op3.type != o_void)
	{
		out_symbol(',');
		out_char(' ');
		out_one_operand(2);
	}

	out_immchar_cmts();
	flush_outbuf();
}

//--------------------------------------------------------------------------
void idaapi stm8_segstart(outctx_t &ctx, segment_t *Sarea)
{
	char buf[MAXSTR];
	char *const end = buf + sizeof(buf);
	if (is_spec_segm(Sarea->type)) return;

	char *ptr;
	const char *align;
	switch (Sarea->align)
	{
	case saAbs:        align = "at: ";   break;
	case saRelByte:    align = "byte";  break;
	case saRelWord:    align = "word";  break;
	case saRelPara:    align = "para";  break;
	case saRelPage:    align = "page";  break;
	case saRel4K:      align = "4k";    break;
	case saRel64Bytes: align = "64";    break;
	default:           align = NULL;    break;
	}
	if (align == NULL)
	{
		ctx.gen_cmt_line("Segment alignment '%s' can not be represented in assembly",
			get_segment_alignment(Sarea->align));
		align = "";
	}

	qstring sname;
	qstring sclas;
	get_segm_name(&sname, Sarea);
	get_segm_class(&sclas, Sarea);

	const char *comb;
	switch (Sarea->comb)
	{
	case scPub:
	case scPub2:
	case scPub3:    comb = "";        break;
	case scCommon:  comb = "common";  break;
	default:        comb = NULL;      break;
	}
	if (comb == NULL)
	{
		ctx.gen_cmt_line("Segment combination '%s' can not be represented in assembly",
			get_segment_combination(Sarea->comb));
		comb = "";
	}

	if (ash.uflag == ASM_COSMIC)
	{
		if (strieq(sclas.c_str(), "code"))
		{
			sclas = ".text";
		}
		else if (strieq(sclas.c_str(), "data"))
		{
			sclas = ".data";
		}
		else if (strieq(sclas.c_str(), "bss"))
		{
			sclas = ".bss";
		}

		ptr = buf + qsnprintf(buf, sizeof(buf),
			SCOLOR_ON SCOLOR_ASMDIR "%s: section",
			sname.c_str());

		bool has_attr = false;
		if (!streq(sclas.c_str(), ""))
		{
			ptr += qsnprintf(ptr, end - ptr, " %s", sclas.c_str());
			has_attr = true;
		}

		if (Sarea->align == saAbs)
		{
			if (has_attr)
			{
				APPEND(ptr, end, ",");
			}
			APPEND(ptr, end, " abs");
		}
	}
	else
	{
		ptr = buf + qsnprintf(buf, sizeof(buf),
			SCOLOR_ON SCOLOR_ASMDIR "%-*s segment %s ",
			inf.indent - 1,
			sname.c_str(),
			align);
		if (Sarea->align == saAbs)
		{
			ea_t absbase = get_segm_base(Sarea);
			ptr += btoa(ptr, end - ptr, absbase);
			APPCHAR(ptr, end, ' ');
		}
		ptr += qsnprintf(ptr, end - ptr, "%s '%s'", comb, sclas.c_str());
	}
	
	ctx.flush_buf(buf, 0);

	if (Sarea->align == saAbs)
	{
		ptr = buf + qsnprintf(buf, sizeof(buf),
			SCOLOR_ON SCOLOR_ASMDIR "org 0x%x",
			Sarea->start_ea);
		ctx.flush_buf(buf, 0);
	}
}

//--------------------------------------------------------------------------
void idaapi stm8_segend(outctx_t &ctx, segment_t *seg)
{
}

//--------------------------------------------------------------------------
//  Generate stack variable definition line
void idaapi stm8_gen_stkvar_def(outctx_t &ctx, const member_t *mptr, sval_t v)
{
	char sign = ' ';
	if (v < 0)
	{
		v = -v;
		sign = '-';
	}
	char num[MAX_NUMBUF];
	btoa(num, sizeof(num), v);

	qstring name = get_member_name(mptr->id);
	if (ash.uflag == ASM_COSMIC)
	{
		ctx.out_printf(COLSTR("%s", SCOLOR_LOCNAME)
			COLSTR(": ", SCOLOR_SYMBOL)
			COLSTR(".assign", SCOLOR_ASMDIR)
			COLSTR(" %c", SCOLOR_SYMBOL)
			COLSTR("%s", SCOLOR_DNUM),
			name.c_str(), sign, num);
	}
	else
	{
		ctx.out_printf(COLSTR("%-*s", SCOLOR_LOCNAME)
			COLSTR("= %c", SCOLOR_SYMBOL)
			COLSTR("%s", SCOLOR_DNUM),
			inf.indent, name.c_str(), sign, num);
	}
}

//--------------------------------------------------------------------------
void idaapi stm8_header(outctx_t &ctx)
{
	//D  gen_cmt_line("Processor       : %-8.8s", inf.procName);
	ctx.gen_header(GH_PRINT_PROC | GH_PRINT_HEADER);
	ctx.gen_cmt_line("Byte Order      : %s", inf.is_be() ? "Big endian" : "Little endian");
	ctx.gen_cmt_line("Byte Order      : %s", inf.is_be() ? "Big endian" : "Little endian");
	ctx.gen_empty_line();
}

//--------------------------------------------------------------------------
void idaapi stm8_footer(outctx_t &ctx)
{
	qstring name;
	if (get_colored_name(&name, inf.start_ea) > 0)
	{
		if (ash.end == NULL)
			ctx.gen_printf(inf.indent, COLSTR("%s end %s", SCOLOR_AUTOCMT), ash.cmnt, name.c_str());
		else
			ctx.gen_printf(inf.indent, COLSTR("%s", SCOLOR_ASMDIR)
				" "
				COLSTR("%s %s", SCOLOR_AUTOCMT), ash.end, ash.cmnt, name.c_str());
	}
}

