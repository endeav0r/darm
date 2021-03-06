#include "darm.h"

#include <stdio.h>

const char darm_reg_str[][4] = {
    "R0",  "R1",  "R2",  "R3",
    "R4",  "R5",  "R6",  "R7",
    "R8",  "R9",  "R10", "R11",
    "R12", "R13", "R14", "PC"
};

int darm_dis (struct _darm * darm, uint32_t ins)
{
    darm->cond = ins >> 28;

    // Branch Exchange
    if (((ins >> 4) & 0x00ffffff) == 0x0012fff1) {
        darm->op = ARM_BX;
        darm->rn = ins & 0xf;
    }

    // Branch
    if ((ins & 0x0e000000) == 0x0a000000) {
        darm->L = (ins >> 24) & 0x1;

        if (darm->L)
            darm->op = ARM_BL;
        else
            darm->op = ARM_B;

        darm->offset = ins & 0x00ffffff;
        if (darm->offset & 0x00800000)
            darm->offset |= 0xff000000;
    }

    // Multiply and Multiply Accumulate
    else if ((ins & 0x0fc000f0) == 0x00000090) {
        darm->A = (ins >> 21) & 0x1;

        if (darm->A)
            darm->op = ARM_MLA;
        else
            darm->op = ARM_MUL;

        darm->S  = (ins >> 20) & 0x1;
        darm->rd = (ins >> 16) & 0xf;
        darm->rn = (ins >> 12) & 0xf;
        darm->rs = (ins >>  8) & 0xf;
    }

    // MRS
    else if ((ins & 0x0fbf0fff) == 0x010f0000) {
        darm->op = ARM_MRS;
        darm->Ps = (ins >> 22) & 0x1;
        darm->rd = (ins >> 12) & 0xf;
    }

    // MSR
    else if ((ins & 0x0fbffff0) == 0x0129f000) {
        darm->op = ARM_MSR;
        darm->Pd = (ins >> 22) & 0x1;
        darm->rm = ins & 0xf;
    }
    else if ((ins & 0x0fbffff0) == 0x0128f000) {
        darm->op = ARM_MSR_flg;
        darm->I  = (ins >> 25) & 0x1;
        if (darm->I) {
            darm->rotate = (ins >> 8) & 0xf;
            darm->imm    = ins & 0xff;
        }
        else {
            darm->rm = ins & 0xff;
        }
    }

    // Multiply Long, Multiply Accumulate Long
    else if ((ins & 0x0f8000f0) == 0x00800090) {
        darm->U = (ins >> 22) & 0x1;
        darm->A = (ins >> 21) & 0x1;
        darm->S = (ins >> 20) & 0x1;

        if (darm->A)
            darm->op = ARM_MLAL;
        else
            darm->op = ARM_MULL;

        darm->rdhi = (ins >> 16) & 0xf;
        darm->rdlo = (ins >> 12) & 0xf;
        darm->rs   = (ins >> 8)  & 0xf;
        darm->rm   = ins & 0xf;
    }

    // Single Data Transfer
    else if ((ins & 0x0c000000) == 0x04000000) {
        darm->I = (ins >> 25) & 0x1;
        darm->P = (ins >> 24) & 0x1;
        darm->U = (ins >> 23) & 0x1;
        darm->B = (ins >> 22) & 0x1;
        darm->W = (ins >> 21) & 0x1;
        darm->L = (ins >> 20) & 0x1;

        if (darm->L)
            darm->op = ARM_LDR;
        else
            darm->op = ARM_STR;

        darm->rn = (ins >> 16) & 0xf;
        darm->rd = (ins >> 12) & 0xf;
        if (darm->I)
            darm->imm = ins & 0xfff;
        else {
            darm->shift = (ins >> 4) & 0xff;
            darm->rm    = ins & 0xf;
        }
    }

    // Single-step swap
    else if ((ins & 0x0fb00ff0) == 0x01000090) {
        darm->B  = (ins >> 22) & 0x1;
        darm->rn = (ins >> 16) & 0xf;
        darm->rd = (ins >> 12) & 0xf;
        darm->rm = ins & 0xf;

        if (darm->B)
            darm->op = ARM_SWPB;
        else
            darm->op = ARM_SWP;
    }

    // Halfword and Signed Data Transfer
    else if (    ((ins & 0x0e400f90) == 0x00000090)
              || ((ins & 0x0e400090) == 0x00400090)) {
        darm->P  = (ins >> 24) & 0x1;
        darm->U  = (ins >> 23) & 0x1;
        darm->I  = (ins >> 22) & 0x1;
        darm->W  = (ins >> 21) & 0x1;
        darm->L  = (ins >> 20) & 0x1;
        darm->rn = (ins >> 16) & 0xf;
        darm->rd = (ins >> 12) & 0xf;
        darm->S  = (ins >> 6)  & 0x1;
        darm->H  = (ins >> 5)  & 0x1;
        darm->rm = ins & 0xf;

        if (darm->L) {
            if (darm->H) {
                if (darm->S)
                    darm->op = ARM_LDRSH;
                else
                    darm->op = ARM_LDRH;
            }
            else if (darm->S)
                darm->op = ARM_LDRSB;
            else
                return -1;
        }
        else {
            if ((darm->H) && (darm->S == 0))
                darm->op = ARM_STRH;
            else
                return -1;
        }

        darm->offset = ((ins >> 4) & 0xf0) | (ins & 0xf);
    }

    // Block Data Transfer (LDM, STM)
    else if ((ins & 0x0e000000) == 0x04000000) {
        darm->P  = (ins >> 24) & 0x1;
        darm->U  = (ins >> 23) & 0x1;
        darm->S  = (ins >> 22) & 0x1;
        darm->W  = (ins >> 21) & 0x1;
        darm->L  = (ins >> 20) & 0x1;
        darm->rn = (ins >> 16) & 0xf;

        if ((darm->L) && (darm->P) && (darm->U))
            darm->op = ARM_LDMED;
        else if ((darm->L) && (! darm->P) && (darm->U))
            darm->op = ARM_LDMFD;
        else if ((darm->L) && (darm->P) && (! darm->U))
            darm->op = ARM_LDMEA;
        else if ((darm->L) && (! darm->P) && (! darm->U))
            darm->op = ARM_LDMFA;
        else if ((! darm->L) && (darm->P) && (darm->U))
            darm->op = ARM_STMFA;
        else if ((! darm->L) && (! darm->P) && (darm->U))
            darm->op = ARM_STMEA;
        else if ((! darm->L) && (darm->P) && (! darm->U))
            darm->op = ARM_STMFD;
        else
            darm->op = ARM_STMED;
    }

    // Software Interrupt
    else if ((ins & 0x0f000000) == 0x0f000000) {
        darm->op      = ARM_SWI;
        darm->comment = ins & 0x00ffffff;
    }

    // Coprocessor Data Operations
    else if ((ins & 0x0f000010) == 0x0e000000) {
        darm->op = ARM_CDP;
        darm->cpopc = (ins >> 20) & 0xf;
        darm->crn   = (ins >> 16) & 0xf;
        darm->crd   = (ins >> 12) & 0xf;
        darm->cpnum = (ins >>  8) & 0xf;
        darm->cp    = (ins >> 5)  & 0x7;
        darm->crm   = ins & 0xf;
    }

    // Coprocessor Data Transfers
    else if ((ins & 0x0e000000) == 0x0c000000) {
        darm->P = (ins >> 24) & 0x1;
        darm->U = (ins >> 23) & 0x1;
        darm->N = (ins >> 22) & 0x1;
        darm->W = (ins >> 21) & 0x1;
        darm->L = (ins >> 20) & 0x1;
        darm->rn = (ins >> 16) & 0xf;
        darm->crd = (ins >> 12) & 0xf;
        darm->cpnum = (ins >> 8) & 0xf;
        darm->offset = ins & 0xff;

        if (darm->L)
            darm->op = ARM_LDC;
        else
            darm->op = ARM_STC;
    }

    // Coprocessor Register Transfers
    else if ((ins & 0x0f000010) == 0x0e000010) {
        darm->cpopc = (ins >> 21) & 0x7;
        darm->L     = (ins >> 20) & 0x1;
        darm->crn   = (ins >> 16) & 0xf;
        darm->rd    = (ins >> 12) & 0xf;
        darm->cpnum = (ins >> 8) & 0xf;
        darm->cp    = (ins >> 5) & 0x7;
        darm->crm   = ins & 0xf;

        if (darm->L)
            darm->op = ARM_MRC;
        else
            darm->op = ARM_MCR;
    }

    // Data Processing
    else if ((ins & 0x0c000000) == 0x00000000) {
        darm->op = (ins >> 21) & 0xf;
        darm->I  = (ins >> 25) & 0x1;
        darm->S  = (ins >> 20) & 0x1;
        darm->rn = (ins >> 16) & 0xf;
        darm->rd = (ins >> 12) & 0xf;

        if (darm->I) {
            darm->rotate = (ins >> 8) & 0xf;
            darm->imm    = ins & 0xff;
        }
        else {
            darm->shift = (ins >> 4) & 0xff;
            darm->rm    = ins & 0xf;
        }

        return 0;
    }

    else if ((ins & 0x0e000010) == 0x06000010) {
        darm->op = ARM_UNDEF;
    }

    else
        return -1;

    return 0;
}


char * darm_operand2_str (struct _darm * darm, char * buf, size_t size)
{
    // rotate an immediate value
    if (darm->I) {
        // sign extend 8-bit immediate value to 32 bits
        uint32_t value = darm->imm;
        if (value & 0x00000080)
            value |= 0xffffff00;

        uint32_t rotate = darm->rotate * 2;
        value = (value << rotate) | (value >> (32 - rotate));

        snprintf(buf, size, "%x", value);

        return buf;
    }
    // shift applied to rm
    else {
        char * type;

        switch ((darm->shift >> 1) & 0x3) {
        case 0 : type = "LSL"; break;
        case 1 : type = "LSR"; break;
        case 2 : type = "SAR"; break;
        case 3 : type = "ROR"; break;
        }

        switch (darm->shift & 1) {
        case 0 :
            if ((darm->shift >> 3) == 0)
                snprintf(buf, size, "%s", darm_reg_str[darm->rm]);
            else
                snprintf(buf, size, "%s,%s #%d",
                         darm_reg_str[darm->rm],
                         type,
                         (darm->shift >> 3) & 0x1f);
            break;
        case 1 :
            snprintf(buf, size, "%s,%s %s",
                     darm_reg_str[darm->rm],
                     type,
                     darm_reg_str[darm->rs]);
        }

        return buf;
    }
}

/*
*   ARM LDR/STR operations support optional increment/decrement, where the
*   CPU decrements/increments a register, an offset from which being address
*   where the memory operation takes place.
*   The P bit determines when the increment/decrement will take place. If P is
*   set, the inc/dec will take place BEFORE the load/store op. If P is not set,
*   the inc/dec will take place AFTER.
*   In the case of pre-indexed (P is set) indexing, we can set the W bit. This
*   will cause the final value of the pre-indexing operation to be written back
*   into the base register.
*   In the case of post-indexed (P is not set) indexing, the W bit should be 0.
*   If a post-indexed operation, if W is set, it forces non-privileged mode for
*   the transfer. This is useful for generating user-mode addresses if your
*   memory is, well, funky MMU stuff I guess.
*
*   This table is my understanding of how the flags affect this instruction
*   
*   Notes: Immediate-Offset of 0 will not be displayed (IE: [Rn])
*          If Rn is omitted, it should default to PC (which is 8 bytes
*              ahead of the current instruction)
*   |---|---|---|---|-------------------- |
*   | W | P | I | U | <Address>           |
*   | 0 | 0 | 0 | 0 | [Rn],-#             | Post Sub Immediate-Offset Write-Back
*   | 0 | 0 | 0 | 1 | [Rn],#              | Post Add Immediate-Offset Write-Back
*   | 0 | 0 | 1 | 0 | [Rn],-Rm{,<shift>}  | Post Sub Register-Offset  Write-Back
*   | 0 | 0 | 1 | 1 | [Rn],Rm{,<shift>}   | Post Add Register-Offset  Write-Back
*   | 0 | 1 | 0 | 0 | [Rn,-#]             | Pre  Sub Immediate-Offset
*   | 0 | 1 | 0 | 1 | [Rn,#]              | Pre  Add Immediate-Offset
*   | 0 | 1 | 1 | 0 | [Rn,-Rm{,<shift>}]  | Pre  Sub Register-Offset
*   | 0 | 1 | 1 | 1 | [Rn,Rm{,<shift>}]   | Pre  Add Register-Offset
*   | 1 | 0 | 0 | 0 | [Rn],-#             | Post Sub Immediate-Offset Write-Back User-Addr
*   | 1 | 0 | 0 | 1 | [Rn],#              | Post Add Immediate-Offset Write-Back User-Addr
*   | 1 | 0 | 1 | 0 | [Rn],-Rm{,<shift>}  | Post Sub Register-Offset  Write-Back User-Addr
*   | 1 | 0 | 1 | 1 | [Rn],Rm{,<shift>}   | Post Add Register-Offset  Write-Back User-Addr
*   | 1 | 1 | 0 | 0 | [Rn,-#]!            | Pre  Sub Immediate-Offset Write-Back
*   | 1 | 1 | 0 | 1 | [Rn,#]!             | Pre  Add Immediate-Offset Write-Back
*   | 1 | 1 | 1 | 0 | [Rn,-Rm{,<shift>}]! | Pre  Sub Register-Offset  Write-Back
*   | 1 | 1 | 1 | 1 | [Rn,Rm{,<shift>}]!  | Pre  Add Register-Offset  Write-Back
*
*   Computers are hard :p
*/
char * darm_ldr_address_str (struct _darm * darm, char * buf, size_t size)
{
    char address[64];

    char * u = "-";
    if (darm->U) u = "";
    
    if (darm->I) { // offset is a register
        char * type = "";
        switch ((darm->shift >> 1) & 0x3) {
        case 0 : type = "LSL"; break;
        case 1 : type = "LSR"; break;
        case 2 : type = "SAR"; break;
        case 3 : type = "ROR"; break;
        }

        if (darm->shift >> 3) { // shift is applied to Rm
            snprintf(address, 64, "%s%s,%s#%d",
                     u,
                     darm_reg_str[darm->rm],
                     type,
                     (darm->shift >> 3) & 0x1f);
        }
        else { // no shift applied
            snprintf(address, 64, "%s%s", u, darm_reg_str[darm->rm]);
        }
    }
    else { // offset is an immediate value
        snprintf(address, 64, "%s#%d", u, darm->offset);
    }
    if (darm->P) { // pre-indexed
        if ((darm->I == 0) && (darm->offset == 0))
            snprintf(buf, size, "[%s]",
                     darm_reg_str[darm->rn]);
        else
            snprintf(buf, size, "[%s,%s]!",
                     darm_reg_str[darm->rn], address);
    } // post-indexed
    else {
        snprintf(buf, size, "[%s],%s",
                 darm_reg_str[darm->rn], address);
    }

    return buf;
}


#define OPERAND2_BUF_SIZE 32
char * darm_str (struct _darm * darm, uint32_t pc)
{
    const char * mnemonic;
    const char * cond;

    char operand2_buf[OPERAND2_BUF_SIZE];

    switch (darm->op) {
    case ARM_AND     : mnemonic = "AND"; break;
    case ARM_EOR     : mnemonic = "EOR"; break;
    case ARM_SUB     : mnemonic = "SUB"; break;
    case ARM_RSB     : mnemonic = "RSB"; break;
    case ARM_ADD     : mnemonic = "ADD"; break;
    case ARM_ADC     : mnemonic = "ADC"; break;
    case ARM_SBC     : mnemonic = "SBC"; break;
    case ARM_RSC     : mnemonic = "RSC"; break;
    case ARM_TST     : mnemonic = "TST"; break;
    case ARM_TEQ     : mnemonic = "TEQ"; break;
    case ARM_CMP     : mnemonic = "CMP"; break;
    case ARM_CMN     : mnemonic = "CMN"; break;
    case ARM_ORR     : mnemonic = "ORR"; break;
    case ARM_MOV     : mnemonic = "MOV"; break;
    case ARM_BIC     : mnemonic = "BIC"; break;
    case ARM_MVN     : mnemonic = "MVN"; break;
    case ARM_MUL     : mnemonic = "MUL"; break;
    case ARM_MLA     : mnemonic = "MLA"; break;
    case ARM_MULL    : mnemonic = "MULL"; break;
    case ARM_MLAL    : mnemonic = "MLAL"; break;
    case ARM_BX      : mnemonic = "BX"; break;
    case ARM_B       : mnemonic = "B"; break;
    case ARM_BL      : mnemonic = "BL"; break;
    case ARM_STR     : mnemonic = "STR"; break;
    case ARM_LDR     : mnemonic = "LDR"; break;
    case ARM_SWP     : mnemonic = "SWP"; break;
    case ARM_SWPB    : mnemonic = "SWPB"; break;
    case ARM_SWI     : mnemonic = "SWI"; break;
    case ARM_CDP     : mnemonic = "CDP"; break;
    case ARM_LDC     : mnemonic = "LDC"; break;
    case ARM_STC     : mnemonic = "STC"; break;
    case ARM_MCR     : mnemonic = "MCR"; break;
    case ARM_MRC     : mnemonic = "MRC"; break;
    case ARM_LDMED   : mnemonic = "LDMED"; break;
    case ARM_LDMFD   : mnemonic = "LDMFD"; break;
    case ARM_LDMEA   : mnemonic = "LDMEA"; break;
    case ARM_LDMFA   : mnemonic = "LDMFA"; break;
    case ARM_STMFA   : mnemonic = "STMFA"; break;
    case ARM_STMEA   : mnemonic = "STMEA"; break;
    case ARM_STMFD   : mnemonic = "STMFD"; break;
    case ARM_STMED   : mnemonic = "STME"; break;
    case ARM_MRS     : mnemonic = "MRS"; break;
    case ARM_MSR     : mnemonic = "MSR"; break;
    case ARM_MSR_flg : mnemonic = "MSR_flg"; break;
    case ARM_UNDEF   : mnemonic = "UNDEF"; break;
    default          : mnemonic = "???"; break;
    }

    switch (darm->cond) {
    case ARM_COND_EQ : cond = "EQ"; break;
    case ARM_COND_NE : cond = "NE"; break;
    case ARM_COND_CS : cond = "CS"; break;
    case ARM_COND_CC : cond = "CC"; break;
    case ARM_COND_MI : cond = "MI"; break;
    case ARM_COND_PL : cond = "PL"; break;
    case ARM_COND_VS : cond = "VS"; break;
    case ARM_COND_VC : cond = "VC"; break;
    case ARM_COND_HI : cond = "HI"; break;
    case ARM_COND_LS : cond = "LS"; break;
    case ARM_COND_GE : cond = "GE"; break;
    case ARM_COND_LT : cond = "LT"; break;
    case ARM_COND_GT : cond = "GT"; break;
    case ARM_COND_LE : cond = "LE"; break;
    case ARM_COND_AL : cond = "";   break;
    case ARM_COND_NV : cond = "NV"; break;
    }

    switch (darm->op) {
    case ARM_MVN :
    case ARM_MOV : {
        char * s = "";
        if (darm->S) s = "S";
        snprintf(darm->text, DARM_TEXT_SIZE, "%s%s%s %s,%s",
                 mnemonic, cond, s,
                 darm_reg_str[darm->rd],
                 darm_operand2_str(darm, operand2_buf, OPERAND2_BUF_SIZE));
    }
    break;
    case ARM_CMP :
    case ARM_CMN :
    case ARM_TST :
    case ARM_TEQ :
        snprintf(darm->text, DARM_TEXT_SIZE, "%s%s %s,%s",
                 mnemonic, cond,
                 darm_reg_str[darm->rn],
                 darm_operand2_str(darm, operand2_buf, OPERAND2_BUF_SIZE));
    break;

    case ARM_AND :
    case ARM_EOR :
    case ARM_SUB :
    case ARM_RSB :
    case ARM_ADD :
    case ARM_ADC :
    case ARM_SBC :
    case ARM_RSC :
    case ARM_ORR :
    case ARM_BIC : {
        char * s = "";
        if (darm->S) s = "S";
        snprintf(darm->text, DARM_TEXT_SIZE, "%s%s%s %s,%s,%s",
                 mnemonic, cond, s,
                 darm_reg_str[darm->rd],
                 darm_reg_str[darm->rn],
                 darm_operand2_str(darm, operand2_buf, OPERAND2_BUF_SIZE));
        break;
    }

    case ARM_BX :
        snprintf(darm->text, DARM_TEXT_SIZE, "%s%s %s",
                 mnemonic, cond, darm_reg_str[darm->rn]);
        break;

    case ARM_B  :
    case ARM_BL :
        snprintf(darm->text, DARM_TEXT_SIZE, "%s%s %x",
                 mnemonic, cond, darm->offset);
        break;

    case ARM_LDR :
    case ARM_STR : {
        char buf[64];
        char * b = "";
        if (darm->B) b = "B";
        char * t = "";
        if (darm->W & darm->P) t = "T";
        snprintf(darm->text, DARM_TEXT_SIZE, "%s%s%s%s %s,%s",
                 mnemonic, cond, b, t, darm_reg_str[darm->rd],
                 darm_ldr_address_str(darm, buf, 64));
        break;
    }

    case ARM_MRS : {
        char * psr = "CPSR";
        if (darm->Ps) psr = "SPSR";
        snprintf(darm->text, DARM_TEXT_SIZE, "%s%s %s,%s",
                 mnemonic, cond, darm_reg_str[darm->rd], psr);
        break;
    }

    case ARM_MSR : {
        char * psr = "CPSR";
        if (darm->Pd) psr = "SPSR";
        snprintf(darm->text, DARM_TEXT_SIZE, "%s%s %s,%s",
                 mnemonic, cond, psr, darm_reg_str[darm->rm]);
        break;
    }

    case ARM_MSR_flg : {
        char * psrf = "CPSR";
        if (darm->Pd) psrf = "SPSR";
        snprintf(darm->text, DARM_TEXT_SIZE, "%s%s %s,%s",
                 mnemonic, cond, psrf,
                 darm_operand2_str(darm, operand2_buf, OPERAND2_BUF_SIZE));
        break;
    }

    case ARM_MUL : {
        char * s = "";
        if (darm->S) s = "S";
        snprintf(darm->text, DARM_TEXT_SIZE, "%s%s%s %s,%s,%s",
                 mnemonic, cond, s,
                 darm_reg_str[darm->rd],
                 darm_reg_str[darm->rm],
                 darm_reg_str[darm->rs]);
        break;
    }

    case ARM_MLA : {
        char * s = "";
        if (darm->S) s = "S";
        snprintf(darm->text, DARM_TEXT_SIZE, "%s%s%s %s,%s,%s,%s",
                 mnemonic, cond, s,
                 darm_reg_str[darm->rd],
                 darm_reg_str[darm->rm],
                 darm_reg_str[darm->rs],
                 darm_reg_str[darm->rn]);
        break;
    }

    case ARM_MULL :
    case ARM_MLAL : {
        char * sign = "U";
        if (darm->S) sign = "S";
        char * s = "";
        if (darm->S) s = "S";
        snprintf(darm->text, DARM_TEXT_SIZE, "%s%s%s%s %s,%s,%s,%s",
                 sign, mnemonic, cond, s,
                 darm_reg_str[darm->rdlo],
                 darm_reg_str[darm->rdhi],
                 darm_reg_str[darm->rm],
                 darm_reg_str[darm->rs]);
        break;
    }

    default :
        snprintf(darm->text, DARM_TEXT_SIZE, "%s%s", mnemonic, cond);
    }


    return darm->text;
}
