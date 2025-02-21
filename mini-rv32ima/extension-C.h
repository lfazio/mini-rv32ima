// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2025 Laurent Fazio <laurent.fazio@gmail.com>

#ifndef _EXTENSION_C_H
#define _EXTENSION_C_H

#define IALIGN (2)

#define MINIRV32IMA_EXTENSION_C_OPCODES                                                                                          \
	do {                                                                                                                     \
		switch (ir & 0x3) {                                                                                              \
		case 0x0: { /* C0 */                                                                                             \
			pc_step = 2;                                                                                             \
			ir = (uint16_t)ir;                                                                                       \
			uint32_t funct3 = (ir >> 13) & 0x7;                                                                      \
			uint32_t nzuimm;                                                                                         \
			uint32_t uimm;                                                                                           \
                                                                                                                                 \
			switch (funct3) {                                                                                        \
			case 0x0: { /* C.ADDI4SPN */                                                                             \
				nzuimm = ((ir & 0x1800) >> 7) | ((ir & 0x0780) >> 1) | ((ir & 0x0040) >> 4)                      \
					 | ((ir & 0x0020) >> 2);                                                                 \
                                                                                                                                 \
				if (!ir && !nzuimm) {                                                                            \
					trap = (2 + 1);                                                                          \
					break;                                                                                   \
				}                                                                                                \
                                                                                                                                 \
				rdid = 8 + ((ir >> 2) & 0x7);                                                                    \
				rval = REG(2) + nzuimm;                                                                          \
				break;                                                                                           \
			}                                                                                                        \
			case 0x2: { /* C.LW */                                                                                   \
				uint32_t rs1id = 8 + ((ir >> 7) & 0x7);                                                          \
				uint32_t rs1 = REG(rs1id);                                                                       \
				uint32_t rsval;                                                                                  \
                                                                                                                                 \
				rdid = 8 + ((ir >> 2) & 0x7);                                                                    \
				uimm = ((ir & 0x1c00) >> 7) | ((ir & 0x0040) >> 4) | ((ir & 0x0020) << 1);                       \
				rsval = rs1 + uimm;                                                                              \
                                                                                                                                 \
				rsval -= MINIRV32_RAM_IMAGE_OFFSET;                                                              \
				if (rsval >= MINI_RV32_RAM_SIZE - 3) {                                                           \
					rsval += MINIRV32_RAM_IMAGE_OFFSET;                                                      \
					if (MINIRV32_MMIO_RANGE(rsval)) { /* UART, CLNT */                                       \
						MINIRV32_HANDLE_MEM_LOAD_CONTROL(rsval, rval);                                   \
					} else {                                                                                 \
						trap = (5 + 1);                                                                  \
						rval = rsval;                                                                    \
					}                                                                                        \
				} else {                                                                                         \
					rval = MINIRV32_LOAD4(rsval);                                                            \
				}                                                                                                \
				break;                                                                                           \
			}                                                                                                        \
			case 0x6: { /* C.SW */                                                                                   \
				uint32_t rs1id = 8 + ((ir >> 7) & 0x7);                                                          \
				uint32_t rs2id = 8 + ((ir >> 2) & 0x7);                                                          \
				uint32_t rs1 = REG(rs1id);                                                                       \
				uint32_t rs2 = REG(rs2id);                                                                       \
				uimm = ((ir & 0x1c00) >> 7) | ((ir & 0x0040) >> 4) | ((ir & 0x0020) << 1);                       \
				uint32_t addy = uimm + rs1 - MINIRV32_RAM_IMAGE_OFFSET;                                          \
                                                                                                                                 \
				if (addy >= MINI_RV32_RAM_SIZE - 3) {                                                            \
					addy += MINIRV32_RAM_IMAGE_OFFSET;                                                       \
					if (MINIRV32_MMIO_RANGE(addy)) {                                                         \
						MINIRV32_HANDLE_MEM_STORE_CONTROL(addy, rs2);                                    \
					} else { /* Store access fault. */                                                       \
						trap = (7 + 1);                                                                  \
						rval = addy;                                                                     \
					}                                                                                        \
				} else {                                                                                         \
					MINIRV32_STORE4(addy, rs2);                                                              \
				}                                                                                                \
				break;                                                                                           \
			}                                                                                                        \
			default:                                                                                                 \
				trap = (2 + 1);                                                                                  \
			}                                                                                                        \
			break;                                                                                                   \
		}                                                                                                                \
		case 0x1: { /* C1 */                                                                                             \
			pc_step = 2;                                                                                             \
			ir = (uint16_t)ir;                                                                                       \
			uint32_t funct3 = (ir >> 13) & 0x7;                                                                      \
                                                                                                                                 \
			switch (funct3) {                                                                                        \
			case 0x0: { /* C.NOP, C.ADDI */                                                                          \
				int32_t nzimm = ((int32_t)(((ir & 0x1000) >> 7) | ((ir >> 2) & 0x1f)) << 26) >> 26;              \
                                                                                                                                 \
				if (!rdid && !nzimm) /* C.NOP */                                                                 \
					break;                                                                                   \
                                                                                                                                 \
				if (rdid && nzimm) {                                                                             \
					rval = REG(rdid) + nzimm;                                                                \
					break;                                                                                   \
				}                                                                                                \
                                                                                                                                 \
				trap = (1 + 2);                                                                                  \
				break;                                                                                           \
			}                                                                                                        \
			case 0x1: /* C.JAL */                                                                                    \
			case 0x5: { /* C.J */                                                                                    \
				int32_t offset = ((int32_t)(((ir & 0x1000) >> 1) | ((ir & 0x0800) >> 7) | ((ir & 0x0600) >> 1)   \
							    | ((ir & 0x0100) << 2) | ((ir & 0x0080) >> 1) | ((ir & 0x0040) << 1) \
							    | ((ir & 0x0038) >> 2) | ((ir & 0x0004) << 3))                       \
						  << 20)                                                                         \
						 >> 20;                                                                          \
                                                                                                                                 \
				if (funct3 == 0x1) { /* C.JAL */                                                                 \
					rdid = 1;                                                                                \
					rval = pc + 2;                                                                           \
				} else { /* C.J */                                                                               \
					rdid = 0;                                                                                \
				}                                                                                                \
				pc += offset - 2;                                                                                \
				break;                                                                                           \
			}                                                                                                        \
			case 0x2: { /* C.LI */                                                                                   \
				int32_t imm = ((int32_t)(((ir & 0x1000) >> 7) | ((ir >> 2) & 0x1f)) << 26) >> 26;                \
                                                                                                                                 \
				if (!rdid) {                                                                                     \
					trap = (2 + 1);                                                                          \
					break;                                                                                   \
				}                                                                                                \
                                                                                                                                 \
				rval = imm;                                                                                      \
				break;                                                                                           \
			}                                                                                                        \
			case 0x3: { /* C.ADDI16SP, C.LUI */                                                                      \
				int32_t imm;                                                                                     \
                                                                                                                                 \
				if (!rdid) {                                                                                     \
					trap = (2 + 1);                                                                          \
					break;                                                                                   \
				}                                                                                                \
                                                                                                                                 \
				if (rdid == 2) { /* C.ADD16SP */                                                                 \
					imm = ((int32_t)((((ir & 0x1000) >> 3) | ((ir & 0x0040) >> 2) | ((ir & 0x0020) << 1)     \
							  | ((ir & 0x0018) << 4) | ((ir & 0x0004) << 3))                         \
							 << 22)                                                                  \
					       >> 22);                                                                           \
                                                                                                                                 \
					if (!imm) {                                                                              \
						trap = (2 + 1);                                                                  \
						break;                                                                           \
					}                                                                                        \
                                                                                                                                 \
					rval = REG(rdid) + imm;                                                                  \
					break;                                                                                   \
				}                                                                                                \
                                                                                                                                 \
				/* C.LUI */                                                                                      \
				imm = ((int32_t)(((ir & 0x1000) >> 7) | ((ir >> 2) & 0x1f)) << 26) >> 14;                        \
                                                                                                                                 \
				rval = imm;                                                                                      \
				break;                                                                                           \
			}                                                                                                        \
			case 0x4: { /* C.SRLI, C.SRAI, C.ANDI, C.SUB, C.XOR, C.OR, C.AND */                                      \
				uint32_t rs1id = 8 + ((ir >> 7) & 0x7);                                                          \
				uint32_t rs2id = 8 + ((ir >> 2) & 0x7);                                                          \
				uint32_t funct6 = (ir >> 10) & 0x3;                                                              \
				uint32_t funct2 = (ir >> 5) & 0x3;                                                               \
				uint32_t shamt = ((ir >> 2) & 0x1f) | ((ir & 0x1000) >> 7);                                      \
                                                                                                                                 \
				rdid = 8 + ((ir >> 7) & 0x7);                                                                    \
                                                                                                                                 \
				switch (funct6) {                                                                                \
				case 0x0: /* C.SRLI */                                                                           \
					rval = REG(rdid) >> shamt;                                                               \
					break;                                                                                   \
				case 0x1: /* C.SRLI */                                                                           \
					rval = ((int32_t)REG(rdid)) >> shamt;                                                    \
					break;                                                                                   \
				case 0x2: { /* C.ANDI */                                                                         \
					int32_t imm = (((int32_t)shamt) << 26) >> 26;                                            \
					rval = REG(rdid) & imm;                                                                  \
					break;                                                                                   \
				}                                                                                                \
				case 0x3:                                                                                        \
					switch (funct2) {                                                                        \
					case 0x00: /* C.SUB */                                                                   \
						rval = REG(rs1id) - REG(rs2id);                                                  \
						break;                                                                           \
					case 0x01: /* C.XOR */                                                                   \
						rval = REG(rs1id) ^ REG(rs2id);                                                  \
						break;                                                                           \
					case 0x02: /* C.OR */                                                                    \
						rval = REG(rs1id) | REG(rs2id);                                                  \
						break;                                                                           \
					case 0x03: /* C.AND */                                                                   \
						rval = REG(rs1id) & REG(rs2id);                                                  \
						break;                                                                           \
					}                                                                                        \
					break;                                                                                   \
				}                                                                                                \
				break;                                                                                           \
			}                                                                                                        \
			case 0x6: /* C.BEQZ */                                                                                   \
			case 0x7: { /* C.BNEZ */                                                                                 \
				uint32_t rs1id = 8 + ((ir >> 7) & 0x7);                                                          \
				int32_t imm = (((int32_t)(((ir & 0x1000) >> 5) | ((ir & 0x0c00) >> 8) | ((ir & 0x0060))          \
							  | ((ir & 0x0018) >> 3) | ((ir & 0x0004) << 2)))                        \
					       << 24)                                                                            \
					      >> 23;                                                                             \
                                                                                                                                 \
				if (funct3 == 0x6) { /* C.BEQZ */                                                                \
					if (REG(rs1id) == 0)                                                                     \
						pc += imm - 2;                                                                   \
				} else { /* C.BNEZ */                                                                            \
					if (REG(rs1id) != 0)                                                                     \
						pc += imm - 2;                                                                   \
				}                                                                                                \
				break;                                                                                           \
			}                                                                                                        \
			default:                                                                                                 \
				trap = (2 + 1);                                                                                  \
			}                                                                                                        \
			break;                                                                                                   \
		}                                                                                                                \
		case 0x2: { /* C2 */                                                                                             \
			pc_step = 2;                                                                                             \
			ir = (uint16_t)ir;                                                                                       \
			uint32_t funct3 = (ir >> 13) & 0x7;                                                                      \
                                                                                                                                 \
			switch (funct3) {                                                                                        \
			case 0x0: { /* C.SLLI */                                                                                 \
				uint32_t shamt = (ir >> 2) & 0x1f;                                                               \
                                                                                                                                 \
				if (!rdid || (ir & 0x1000)) {                                                                    \
					trap = (2 + 1);                                                                          \
					break;                                                                                   \
				}                                                                                                \
                                                                                                                                 \
				rval = REG(rdid) << shamt;                                                                       \
				break;                                                                                           \
			}                                                                                                        \
			case 0x2: { /* C.LWSP */                                                                                 \
				uint32_t rsval;                                                                                  \
				uint32_t uimm = ((ir & 0x1000) >> 7) | ((ir & 0x0070) >> 2) | ((ir & 0x000c) << 4);              \
                                                                                                                                 \
				if (!rdid) {                                                                                     \
					trap = (2 + 1);                                                                          \
					break;                                                                                   \
				}                                                                                                \
                                                                                                                                 \
				rsval = REG(2) + uimm;                                                                           \
                                                                                                                                 \
				rsval -= MINIRV32_RAM_IMAGE_OFFSET;                                                              \
				if (rsval >= MINI_RV32_RAM_SIZE - 3) {                                                           \
					rsval += MINIRV32_RAM_IMAGE_OFFSET;                                                      \
					if (MINIRV32_MMIO_RANGE(rsval)) { /* UART, CLNT */                                       \
						MINIRV32_HANDLE_MEM_LOAD_CONTROL(rsval, rval);                                   \
					} else {                                                                                 \
						trap = (5 + 1);                                                                  \
						rval = rsval;                                                                    \
					}                                                                                        \
				} else {                                                                                         \
					rval = MINIRV32_LOAD4(rsval);                                                            \
				}                                                                                                \
				break;                                                                                           \
			}                                                                                                        \
			case 0x4: { /* C.JR, C.MV, C.EBREAK, C.JALR, C.ADD */                                                    \
				uint32_t rs1id = (ir >> 7) & 0x1f;                                                               \
				uint32_t rs2id = (ir >> 2) & 0x1f;                                                               \
				rdid = 0;                                                                                        \
                                                                                                                                 \
				if (ir & 0x1000) {                                                                               \
					if (!rs1id && !rs2id) { /* C.EBREAK */                                                   \
						trap = (3 + 1);                                                                  \
						break; /* EBREAK 3 = "Breakpoint" */                                             \
					}                                                                                        \
                                                                                                                                 \
					if (rs1id && !rs2id) { /* C.JALR */                                                      \
						rdid = 1;                                                                        \
						rval = pc + 2;                                                                   \
						pc = REG(rs1id) - 2;                                                             \
						break;                                                                           \
					}                                                                                        \
                                                                                                                                 \
					if (rs1id && rs2id) { /* C.ADD */                                                        \
						rdid = rs1id;                                                                    \
						rval = REG(rs1id) + REG(rs2id);                                                  \
						break;                                                                           \
					}                                                                                        \
				} else {                                                                                         \
					if (rs1id && !rs2id) { /* C.JR */                                                        \
						pc = REG(rs1id) - 2;                                                             \
						break;                                                                           \
					}                                                                                        \
                                                                                                                                 \
					if (rs1id && rs2id) { /* C.MV */                                                         \
						rdid = rs1id;                                                                    \
						rval = REG(rs2id);                                                               \
						break;                                                                           \
					}                                                                                        \
				}                                                                                                \
                                                                                                                                 \
				trap = (2 + 1);                                                                                  \
				break;                                                                                           \
			}                                                                                                        \
			case 0x6: { /* C.SWSP */                                                                                 \
				uint32_t uimm = ((ir & 0x1e00) >> 7) | ((ir & 0x0180) >> 1);                                     \
				uint32_t addy = REG(2) + uimm - MINIRV32_RAM_IMAGE_OFFSET;                                       \
				uint32_t rs2id = (ir >> 2) & 0x1f;                                                               \
                                                                                                                                 \
				if (addy >= MINI_RV32_RAM_SIZE - 3) {                                                            \
					addy += MINIRV32_RAM_IMAGE_OFFSET;                                                       \
					if (MINIRV32_MMIO_RANGE(addy)) {                                                         \
						MINIRV32_HANDLE_MEM_STORE_CONTROL(addy, REG(rs2id));                             \
					} else {                                                                                 \
						trap = (7 + 1); /* Store access fault. */                                        \
						rval = addy;                                                                     \
					}                                                                                        \
				} else {                                                                                         \
					MINIRV32_STORE4(addy, REG(rs2id));                                                       \
				}                                                                                                \
				break;                                                                                           \
			}                                                                                                        \
			default:                                                                                                 \
				trap = (2 + 1);                                                                                  \
			}                                                                                                        \
			break;                                                                                                   \
		}                                                                                                                \
		default:                                                                                                         \
			trap = (2 + 1);                                                                                          \
		}                                                                                                                \
	} while (0)

#endif
