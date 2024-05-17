#ifndef _EXTENSION_F_H
#define _EXTENSION_F_H

#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <fenv.h>

typedef union {
	float f32;
	uint32_t u32;
} freg_t;

#define MINIRV32IMA_ADDITIONAL_F_STATE struct { \
	freg_t f[32]; \
	uint32_t fcsr; \
}

#define REGFSETF( x, val ) do { state->f[(x)].f32 = (val); } while (0)
#define REGFSET( x, val ) do { state->f[(x)].u32 = (val); } while (0)
#define F( x ) state->f[(x)]
#define AS_F32( x ) (x.f32)
#define AS_U32( x ) (x.u32)
#define F32(v) ((float)(v))
#define U32(v) ((uint32_t)(v))
#define I32(v) ((int32_t)(v))
#define FSQRT( x ) sqrtf( (x) )

#define MINIRV32_FCSR_WRITE( csrno, writeval, rval ) \
	case 0x002: /*frm*/ \
		rval = (state->fcsr & 0xe0u) >> 5; \
		state->fcsr = (state->fcsr & ~0xe0u) | (((writeval) & 0x7u) << 5u); \
		break; \
	case 0x001: /*fflags*/ \
		rval = state->fcsr & 0x1fu; \
		state->fcsr = (state->fcsr & ~0x1fu) | ((writeval) & 0x1fu); \
		break; \
	case 0x003: /*fcsr*/ \
		rval = state->fcsr; \
		state->fcsr = (state->fcsr & ~0xffu) | ((writeval) & 0xffu); \
		break

#define MINIRV32_FCSR_READ( csrno, rval ) \
	case 0x001: /*fflags*/ \
		rval = state->fcsr & 0x1fu; \
		break; \
	case 0x002: /*frm*/ \
		rval = (state->fcsr>>5u) & 0x7u; \
		break; \
	case 0x003: /*fcsr*/ \
		rval = state->fcsr & 0xffu; \
		break

#define SETRM(rm) do { \
	if (rm == 0x7u) rm = (state->fcsr >> 5u) & 0x7u; /* Dynamic rounding mode */ \
		switch (rm) { \
			case 0: fesetround(FE_TONEAREST); break; \
			case 1: fesetround(FE_DOWNWARD); break; \
			case 2: fesetround(FE_UPWARD); break; \
			case 3: fesetround(FE_TOWARDZERO); break; \
			default: trap = (2+1); /* Illegal instruction */ break; \
		} \
		feclearexcept(FE_ALL_EXCEPT); \
	} while(0)

#define SETFFLAGS() do { \
		if(fetestexcept(FE_INEXACT))    state->fcsr |= 1u << 0u; \
		if(fetestexcept(FE_UNDERFLOW))  state->fcsr |= 1u << 1u; \
		if(fetestexcept(FE_OVERFLOW))   state->fcsr |= 1u << 2u; \
		if(fetestexcept(FE_DIVBYZERO))  state->fcsr |= 1u << 3u; \
		if(fetestexcept(FE_INVALID))    state->fcsr |= 1u << 4u; \
		feclearexcept(FE_ALL_EXCEPT); \
	} while(0)

#define CLRFFLAGS() do { \
		feclearexcept(FE_ALL_EXCEPT); \
	} while(0)

#define MINIRV32IMA_F_OPCODES \
	case 0x07: /*FLW*/ \
		if( ( ( ir >> 12u ) & 0x7u ) == 2u ) \
		{ \
			uint32_t rsval = REG( (ir >> 15u) & 0x1f ); \
			uint32_t ofs = (((int32_t)ir) >> 20 ); \
			rsval += ofs - MINIRV32_RAM_IMAGE_OFFSET; \
			REGFSET( rdid , MINIRV32_LOAD4( rsval ) ); \
			rdid = 0; /* Don't use regular register saving */ \
		} else { \
			trap = (2+1); \
		} \
		break; \
	case 0x27: /*FSW*/ \
		if( ( ( ir >> 12u ) & 0x7u ) == 2u ) \
		{ \
			uint32_t rs1 = REG( (ir >> 15) & 0x1f ); \
			uint32_t rs2 = AS_U32(F((ir >> 20) & 0x1f)); \
			uint32_t addy = ( ( ir >> 7 ) & 0x1f ) | ( ( ir & 0xfe000000 ) >> 20 ); \
			addy = rs1 + (((int32_t)(addy<<20u))>>20u); \
			addy -= MINIRV32_RAM_IMAGE_OFFSET; \
			MINIRV32_STORE4( addy, rs2 ); \
			rdid = 0; /* Don't register save */ \
		} \
		else \
			trap = (2+1); \
		break; \
	case 0x43: { /*1000011, FMADD.S*/ \
		float rs1 = AS_F32(F((ir >> 15) & 0x1f)); \
		float rs2 = AS_F32(F((ir >> 20) & 0x1f)); \
		float rs3 = AS_F32(F((ir >> 27) & 0x1f)); \
		uint32_t rm = (ir >> 12) & 0x7u; \
		SETRM(rm); \
		REGFSETF( rdid, rs1*rs2+rs3 ); \
		SETFFLAGS(); \
		rdid = 0; /* Don't register save */ \
		break; \
	} \
	case 0x47: { /*1000111, FMSUB.S*/ \
		float rs1 = AS_F32(F((ir >> 15) & 0x1f)); \
		float rs2 = AS_F32(F((ir >> 20) & 0x1f)); \
		float rs3 = AS_F32(F((ir >> 27) & 0x1f)); \
		uint32_t rm = (ir >> 12) & 0x7u; \
		SETRM(rm); \
		REGFSETF( rdid, rs1*rs2-rs3 ); \
		SETFFLAGS(); \
		rdid = 0; /* Don't register save */ \
		break; \
	} \
	case 0x4b: { /*1001011, FNMSUB.S*/ \
		float rs1 = AS_F32(F((ir >> 15) & 0x1f)); \
		float rs2 = AS_F32(F((ir >> 20) & 0x1f)); \
		float rs3 = AS_F32(F((ir >> 27) & 0x1f)); \
		uint32_t rm = (ir >> 12) & 0x7u; \
		SETRM(rm); \
		REGFSETF( rdid, -rs1*rs2+rs3 ); \
		SETFFLAGS(); \
		rdid = 0; /* Don't register save */ \
		break; \
	} \
	case 0x4f: { /*1001111, FNMADD.S*/ \
		float rs1 = AS_F32(F((ir >> 15) & 0x1f)); \
		float rs2 = AS_F32(F((ir >> 20) & 0x1f)); \
		float rs3 = AS_F32(F((ir >> 27) & 0x1f)); \
		uint32_t rm = (ir >> 12) & 0x7u; \
		SETRM(rm); \
		REGFSETF( rdid, -rs1*rs2-rs3 ); \
		SETFFLAGS(); \
		rdid = 0; /* Don't register save */ \
		break; \
	} \
	case 0x53: { /*1010011, Regular float operation*/ \
		uint32_t x = (ir>>15u)&0x1fu; \
		uint32_t rs1 = AS_U32( F( x ) ); \
		uint32_t rs2 = AS_U32( F( (ir >> 20) & 0x1f) ); \
		float frs1 = AS_F32( F( x ) ); \
		float frs2 = AS_F32( F( (ir >> 20) & 0x1f) ); \
		uint32_t rm = (ir>>12)&0x7u; \
		uint32_t subop = ir>>25; \
		SETRM(rm); \
		switch( subop ) \
		{ \
		case 0x00: REGFSETF( rdid, frs1 + frs2 ); break; /*0000000 FADD.S*/ \
		case 0x04: /*0000100 FSUB.S*/ \
			REGFSETF( rdid, frs1 - frs2 ); \
			if (frs1 == INFINITY && frs2 == INFINITY) REGFSET( rdid, 0x7fc00000 ); \
			break; \
		case 0x08: REGFSETF( rdid, frs1 * frs2 ); break; /*0001000 FMUL.S*/ \
		case 0x0c: REGFSETF( rdid, frs1 / frs2 ); break; /*0001100 FDIV.S*/ \
		case 0x2c: /*0101100 FSQRT.S*/ \
			if (frs1 < 0.0f) { \
				REGFSET( rdid, 0x7fc00000u); \
				fesetexcept( FE_INVALID ); \
			} else { \
				REGFSETF( rdid, FSQRT( frs1 ) ); \
			} \
			break; \
		case 0x10: \
			switch( rm ) { \
			case 0x0: REGFSET( rdid, (rs1&~0x80000000) | (rs2&0x80000000) ); break; /*0010000 FSGNJ.S*/ \
			case 0x1: REGFSET( rdid, (rs1&~0x80000000) | ((rs2&0x80000000)^0x80000000) ); break; /*0010000 FSGNJN.S*/ \
			case 0x2: REGFSET( rdid, (rs1&~0x80000000) | ((rs1&0x80000000)^(rs2&0x80000000)) ); break; /*0010000 FSGNJX.S*/ \
			default: trap = (2+1); \
			} \
			CLRFFLAGS(); \
			break; \
		case 0x14: \
			switch( rm ) { \
			case 0x0: /*0010100 FMIN.S*/ \
				if( rs1 == 0x00000000u && rs2 == 0x80000000u ) { \
					REGFSET( rdid, rs2 ); \
				} else if( rs1 == 0x80000000u && rs2 == 0x00000000u ) { \
					REGFSET( rdid, rs1 ); \
				} else { \
					REGFSETF(rdid, fminf( frs1, frs2 ) ); \
				} \
				break; \
			case 0x1: /*0010100 FMAX.S*/ \
				if( isnan(frs1) && !isnan(frs2) ) { \
					REGFSETF( rdid, frs2 ); \
				} else if( !isnan(frs1) && isnan(frs2) ) { \
					REGFSETF( rdid, frs1 ); \
				} else if( isnan(frs1) && isnan(frs2) ) { \
					REGFSET( rdid, 0x7fc00000u ); \
				} else if( rs1 == 0x00000000u && rs2 == 0x80000000u ) { \
					REGFSET( rdid, rs1 ); \
				} else if( rs1 == 0x80000000u && rs2 == 0x00000000u ) { \
					REGFSET( rdid, rs2 ); \
				} else { \
					REGFSETF(rdid, fmaxf( frs1, frs2 ) ); \
				} \
				break; \
			default: trap = (2+1); \
			} \
			break; \
		case 0x60: \
			uint32_t rs2 = (ir>>20u)&0x1fu; \
			switch( rs2 ) { \
			case 0x0: /*1100000 FCVT.W.S*/ \
				REGSET( rdid, U32( I32( frs1 ) ) ); \
				if( fetestexcept( FE_INVALID ) ) { \
					if( frs1 > F32( 0x7fffffffu ) ) { \
						REGSET( rdid, 0x7fffffffu ); \
					} else if( isnan(frs1) ){ \
						REGSET( rdid, 0x7fffffffu ); \
					} else { \
						REGSET( rdid, I32( 0x80000000u ) ); \
					} \
				} \
				break; \
			case 0x1: /*1100000 FCVT.WU.S*/ \
				if( isinf(frs1) == -1 ) { \
					REGSET( rdid, 0x0u ); \
				} else if( isinf(frs1) == 1 ) { \
					REGSET( rdid, 0xffffffffu ); \
				} else if( frs1 <= -1.0f ) { \
					REGSET( rdid, 0x0u ); \
					fesetexcept( FE_INVALID ); \
				} else if ( frs1 < 0.0f ) { \
					REGSET( rdid, 0u ); \
					fesetexcept( FE_INEXACT ); \
				} else if( isnan(frs1) ) { \
					REGSET( rdid, 0xffffffffu ); \
				} else { \
					REGSET( rdid, U32( frs1 ) ); \
				} \
				break; \
			default: trap = (2+1); \
			} \
			break; \
		case 0x68: { \
			uint32_t rs2 = (ir>>20u)&0x1fu; \
			switch( rs2 ) { \
			case 0x0: REGFSETF( rdid, F32( (int32_t)REG( x ) ) ); break; /*1101000 FCVT.S.W*/ \
			case 0x1: REGFSETF( rdid, F32( REG( x ) ) ); break; /*1101000 FCVT.S.WU*/ \
			default: trap = (2+1); \
			} \
			break; \
		 } \
		case 0x50: { \
			switch( rm ) { \
			case 0x0: REGSET( rdid, frs1 <= frs2 ? 1 : 0 ); break; /*1010000 FLE.S*/ \
			case 0x1: REGSET( rdid, frs1 <  frs2 ? 1 : 0 ); break; /*1010000 FLT.S*/ \
			case 0x2: REGSET( rdid, frs1 == frs2 ? 1 : 0 ); break; /*1010000 FEQ.S*/ \
			default: trap = (2+1); \
			} \
			break; \
		} \
		case 0x78: { /*1111000 FMV.W.S*/ \
			REGFSET( rdid, REG( (ir >> 15) & 0x1f ) ); \
			CLRFFLAGS(); \
			break; \
		} \
		case 0x70: { \
			switch( rm ){ \
			case 0x0: REGSET( rdid, rs1 ); CLRFFLAGS(); break; /*1110000 FMV.S.W*/ \
			case 0x1: /*1110000 FCLASS.S*/ \
			{ \
				uint32_t sign = rs1 >> 31u; \
				uint32_t exponent = (rs1 >> 23) & 0xffu; \
				uint32_t mantissa = rs1 & 0x7fffffu; \
				if( exponent == 0x00u && mantissa == 0x00u ) \
				{ \
					if( sign ) \
						REGSET( rdid, 1<<3 ); /* -Zero */ \
					else \
						REGSET( rdid, 1<<4 ); /* +Zero */ \
				} \
				else if( exponent == 0xffu && mantissa == 0x00u ) \
				{ \
					if( sign ) \
						REGSET( rdid, 1<<0 ); /* -Infinity */ \
					else \
						REGSET( rdid, 1<<7 ); /* +Infinity */ \
				} \
				else if( exponent == 0xffu ) \
				{ \
					if( mantissa == 0x1u) \
						REGSET( rdid, 1<<8 ); /* Signaling NaN */ \
					else \
						REGSET( rdid, 1<<9 ); /* Quiet NaN */ \
				} \
				else if( exponent == 0x00u ) \
				{ \
					if( sign ) \
						REGSET( rdid, 1<<2 ); /* -Subnormalised */ \
					else \
						REGSET( rdid, 1<<5 ); /* +Subnormalised */ \
				} \
				else \
				{ \
					if( sign ) \
						REGSET( rdid, 1<<1 ); /* -Normalised */ \
					else \
						REGSET( rdid, 1<<6 ); /* +Normalised */ \
				} \
				break; \
			} \
			default: trap = (2+1); \
			} \
			break; \
		} \
		default: trap = (2+1); \
		} \
		SETFFLAGS(); \
		\
		rdid = 0; \
		break; \
	}
#endif

