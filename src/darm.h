#ifndef darm_HEADER
#define darm_HEADER

#include <inttypes.h>

enum {
    ARM_COND_EQ,
    ARM_COND_NE,
    ARM_COND_CS,
    ARM_COND_CC,
    ARM_COND_MI,
    ARM_COND_PL,
    ARM_COND_VS,
    ARM_COND_VC,
    ARM_COND_HI,
    ARM_COND_LS,
    ARM_COND_GE,
    ARM_COND_LT,
    ARM_COND_GT,
    ARM_COND_LE,
    ARM_COND_AL,
    ARM_COND_NV
};

enum {
    ARM_AND = 0,
    ARM_EOR,
    ARM_SUB,
    ARM_RSB,
    ARM_ADD,
    ARM_ADC,
    ARM_SBC,
    ARM_RSC,
    ARM_TST,
    ARM_TEQ,
    ARM_CMP,
    ARM_CMN,
    ARM_ORR,
    ARM_MOV,
    ARM_BIC,
    ARM_MVN,

    ARM_MUL,
    ARM_MLA,
    ARM_MULL,
    ARM_MLAL,

    ARM_BX,
    ARM_B,
    ARM_BL,

    ARM_LDR,
    ARM_STR,

    ARM_LDRH,
    ARM_LDRSH,
    ARM_LDRSB,
    ARM_STRH,

    ARM_SWP,
    ARM_SWPB,

    ARM_SWI,

    ARM_CDP,

    ARM_LDC,
    ARM_STC,

    ARM_MCR,
    ARM_MRC,

    ARM_LDMED,
    ARM_LDMFD,
    ARM_LDMEA,
    ARM_LDMFA,
    ARM_STMFA,
    ARM_STMEA,
    ARM_STMFD,
    ARM_STMED,

    ARM_MRS,
    ARM_MSR,
    ARM_MSR_flg,

    ARM_UNDEF
};

#define DARM_TEXT_SIZE 64

struct _darm {
    char text[DARM_TEXT_SIZE];
    int op;

    uint8_t cond;

    uint8_t A; // accumulate
    uint8_t B; // byte/word bit
    uint8_t H;
    uint8_t I; // immediate operand
    uint8_t L; // link OR load/store bit
    uint8_t N; // transfer length
    uint8_t P; // pre/post indexing bit
    uint8_t S; // set condition codes OR PSR & Force User Bit
    uint8_t U; // unsigned OR up/down bit
    uint8_t W; // write-back bit

    uint8_t cpopc;  // coprocessor op code
    uint8_t cpnum; // coprocessor number
    uint8_t cp;    // coprocessor information

    union {
        uint8_t Ps; // source PSR
        uint8_t Pd; // destination PSR
    };
    union {
        uint8_t rd;   // destination register
        uint8_t crd;
        uint8_t rdhi;
    };
    union {
        uint8_t rn;   // first operand register
        uint8_t crn;
        uint8_t rdlo;
    };
    
    uint8_t rs;

    union {
        uint16_t shift;
        uint8_t  rotate;
    };
    union {
        uint8_t rm; // second operand register
        uint8_t crm;
        uint16_t imm;
    };

    union {
        uint32_t comment;
        int32_t  offset;
    };
};


int    darm_dis (struct _darm * darm, uint32_t ins);
char * darm_str (struct _darm * darm, uint32_t pc);

#endif