#ifndef _ST8_HPP
#define _ST8_HPP

#define NO_OBSOLETE_FUNCS

#pragma warning(disable:4005 4312 4244 4520 4018 4267 4800 4996 4482 4101 4002 4102 4101) 

#include "../idaidp.hpp"
#include <diskio.hpp>
#include "ins.hpp"
#include "../iohandler.hpp"

// o_void  Inherent      nop
// o_imm   Immediate     ld A,#$55
// o_mem   Direct        ld A,$55
// o_displ Indexed       ld A,($55,X)
// o_mem   Indirect      ld A,([$55],X)
// o_near  Relative      jrne loop
// o_mem   Bit operation bset byte,#5

//  type            Mode                    Syntax             Destination Ptradr PtrSz Len
// ------- --------------------------- ------------------------ ---------- ------ ---- ---
// o_void  Inherent                    nop                                             + 0
// o_imm   Immediate                   ld A,#$55                                       + 1
// o_mem   Short     Direct            ld A,$10                 00..FF                 + 1
// o_mem   Long      Direct            ld A,$1000               0000..FFFF             + 2
// o_phras No Offset Direct   Indexed  ld A,(X)                 00..FF                 + 0
// o_displ Short     Direct   Indexed  ld A,($10,X)             00..1FE                + 1
// o_displ Long      Direct   Indexed  ld A,($1000,X)           0000..FFFF             + 2
// o_mem   Short     Indirect          ld A,[$10]               00..FF     00..FF byte + 2
// o_mem   Long      Indirect          ld A,[$10.w]             0000..FFFF 00..FF word + 2
// o_mem   Extended  Indirect          ld A,[$10.e]             
// o_mem   Short     Indirect Indexed  ld A,([$10],X)           00..1FE    00..FF byte + 2
// o_mem   Long      Indirect Indexed  ld A,([$10.w],X)         0000..FFFF 00..FF word + 2
// o_mem   Extended  Indirect Indexed  ld A,([$10.e],X)          
// o_near  Relative  Direct            jrne loop                PC+/-127               + 1
// o_mem   Relative  Indirect          jrne [$10]               PC+/-127   00..FF byte + 2
// o_mem   Bit       Direct            bset $10,#7              00..FF                 + 1
// o_mem   Bit       Indirect          bset [$10],#7            00..FF     00..FF byte + 2
// o_mem   Bit       Direct   Relative btjt $10,#7,skip         00..FF                 + 2
// o_mem   Bit       Indirect Relative btjt [$10],#7,skip       00..FF     00..FF byte + 3

#define aux_long       0x0001  // long addressing mode
#define aux_indir      0x0002  // o_mem: indirect addressing mode
#define aux_index      0x0004  // o_mem: indexed addressing mode
#define aux_16         0x0008  // 16bit displacement
#define aux_extd       0x0010  // extended addessing mode

//------------------------------------------------------------------
#ifdef _MSC_VER
#define ENUM8BIT : uint8
#else
#define ENUM8BIT
#endif
enum regnum_t ENUM8BIT
{
  A, X, Y, CC, SP,
  XL, XH, YL, YH,
  ds, cs,
};

//------------------------------------------------------------------
enum
{
	ASM_STASM,
	ASM_COSMIC
};


struct opcode_t
{
	uchar itype;
	uchar opcode;
	ushort dst;
	ushort src;
	uchar len;
	bool extended;
	uchar iform;
};

//------------------------------------------------------------------
extern qstring device;

ea_t calc_mem(const insn_t &insn, ea_t ea);         // map virtual to physical ea
const ioport_t *find_sym(ea_t address);
const struct opcode_t &get_opcode_info(uint8 opcode);
//------------------------------------------------------------------
void interr(const insn_t &insn, const char *module);
int idaapi stm8_is_align_insn(ea_t ea);
void idaapi stm8_gen_stkvar_def(outctx_t &ctx, const member_t *mptr, sval_t v);
int  idaapi ana(insn_t *_insn);
int  idaapi emu(const insn_t &insn);
bool idaapi stm8_is_switch(switch_info_t *si, const insn_t &insn);

int is_sane_insn(const insn_t &insn, int nocrefs);
int get_frame_retsize(func_t *);
int is_jump_func(const func_t *pfn, ea_t *jump_target);
int may_be_func(void);           // can a function start here?

DECLARE_PROC_LISTENER(idb_listener_t, struct stm8_t);

struct stm8_t : public procmod_t
{
  idb_listener_t idb_listener = idb_listener_t(*this);
  netnode helper;
  iohandler_t ioh = iohandler_t(helper);

  virtual ssize_t idaapi on_event(ssize_t msgid, va_list va) override;

  void stm8_header(outctx_t &ctx);
  void handle_operand(const op_t &x, bool forced_op, bool isload, const insn_t &insn);
  void stm8_segstart(outctx_t &ctx, segment_t *Sarea) const;
  void stm8_segend(outctx_t &ctx, segment_t *seg) const;
  void stm8_footer(outctx_t &ctx) const;

  void load_from_idb();
};
#define PROCMOD_NAME stm8

#endif // _ST8_HPP
