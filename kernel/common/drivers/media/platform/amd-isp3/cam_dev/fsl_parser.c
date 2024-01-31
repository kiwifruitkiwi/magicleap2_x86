/*
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#include "sensor_if.h"
#include "fsl_string.h"
#include "fsl_error.h"
#include "fsl_interinstr.h"
#include "fsl_linker.h"
#include "fsl_config.h"
#include "fsl_log.h"

int SvmInitParser(struct _SvmParserContext_t *context)
{
	if (InitInstrContext(context, MAX_INSTRS, MAX_LABELS, MAX_FUNCS) != 0)
		return -1;

	return 0;
}

static int DoSyntaxAnalyze(struct _SvmParserContext_t *context,
			const char *words, int numWords, int maxWordLen)
{
	const char *instrOp;
	const char *arg[MAX_WORDS];
	int i;
	int numArgs;

	if (numWords <= 0) {
		SetErrorInfo(context, "Instruction line word num error:%d\n",
			numWords);
		return -1;
	}
	numArgs = numWords - 1;
	if (numArgs > MAX_ARG_NUM) {
		SetErrorInfo(context, "Too many args:%d\n", numArgs);
		return -1;
	}

	instrOp = words;
	for (i = 0; i < numArgs; i++)
		arg[i] = words + (i + 1) * maxWordLen;

	if (!strcmp("ADD", instrOp)) {
		if (AddInstrArg3(context, INTER_INSTR_OP_ADD, arg, numArgs) !=
			0)
			return -1;
	} else if (!strcmp("SUB", instrOp)) {
		if (AddInstrArg3(context, INTER_INSTR_OP_SUB, arg, numArgs) !=
			0)
			return -1;
	} else if (!strcmp("MUL", instrOp)) {
		if (AddInstrArg3(context, INTER_INSTR_OP_MUL, arg, numArgs) !=
			0)
			return -1;
	} else if (!strcmp("DIV", instrOp)) {
		if (AddInstrArg3(context, INTER_INSTR_OP_DIV, arg, numArgs) !=
			0)
			return -1;
	} else if (!strcmp("SHL", instrOp)) {
		if (AddInstrArg3(context, INTER_INSTR_OP_SHL, arg, numArgs) !=
			0)
			return -1;
	} else if (!strcmp("SHR", instrOp)) {
		if (AddInstrArg3(context, INTER_INSTR_OP_SHR, arg, numArgs) !=
			0)
			return -1;
	} else if (!strcmp("AND", instrOp)) {
		if (AddInstrArg3(context, INTER_INSTR_OP_AND, arg, numArgs) !=
			0)
			return -1;
	} else if (!strcmp("OR", instrOp)) {
		if (AddInstrArg3(context, INTER_INSTR_OP_OR, arg, numArgs) !=
			0)
			return -1;
	} else if (!strcmp("XOR", instrOp)) {
		if (AddInstrArg3(context, INTER_INSTR_OP_XOR, arg, numArgs) !=
			0)
			return -1;
	} else if (!strcmp("WI2C", instrOp)) {
		if (AddInstrWi2c(context, arg, numArgs) != 0)
			return -1;
	} else if (!strcmp("RI2C", instrOp)) {
		if (AddInstrRi2c(context, arg, numArgs) != 0)
			return -1;
	} else if (!strcmp("MOVB", instrOp)) {
		if (AddInstrMov(context, INTER_INSTR_OP_MOVB, arg, numArgs) !=
			0)
			return -1;
	} else if (!strcmp("MOVW", instrOp)) {
		if (AddInstrMov(context, INTER_INSTR_OP_MOVW, arg, numArgs) !=
			0)
			return -1;
	} else if (!strcmp("MOVDW", instrOp)) {
		if (AddInstrMov(context, INTER_INSTR_OP_MOVDW, arg, numArgs) !=
			0)
			return -1;
	} else if (!strcmp("JNZ", instrOp)) {
		if (AddInstrJ(context, INTER_INSTR_OP_JNZ, arg, numArgs) != 0)
			return -1;
	} else if (!strcmp("JEZ", instrOp)) {
		if (AddInstrJ(context, INTER_INSTR_OP_JEZ, arg, numArgs) != 0)
			return -1;
	} else if (!strcmp("JLZ", instrOp)) {
		if (AddInstrJ(context, INTER_INSTR_OP_JLZ, arg, numArgs) != 0)
			return -1;
	} else if (!strcmp("JGZ", instrOp)) {
		if (AddInstrJ(context, INTER_INSTR_OP_JGZ, arg, numArgs) != 0)
			return -1;
	} else if (!strcmp("JGEZ", instrOp)) {
		if (AddInstrJ(context, INTER_INSTR_OP_JGEZ, arg, numArgs) != 0)
			return -1;
	} else if (!strcmp("JLEZ", instrOp)) {
		if (AddInstrJ(context, INTER_INSTR_OP_JLEZ, arg, numArgs) != 0)
			return -1;
	} else if (!strcmp("CALL", instrOp)) {
		if (AddCall(context, arg, numArgs) != 0)
			return -1;
	} else if (!strcmp(".function", instrOp)) {
		int funcNameLen;

		if (numArgs != 1) {
			SetErrorInfo(context,
				".function should with one function name\n");
			return -1;
		}

		funcNameLen = (int)strlen(arg[0]);
		if (funcNameLen <= 0) {
			SetErrorInfo(context, "Function name size error\n");
			return -1;
		}

		if (AddFunc(context, arg[0]) != 0)
			return -1;
	} else if (!strcmp(".endfunction", instrOp)) {
		if (numArgs != 0) {
			SetErrorInfo(context, ".endfunction syntax error\n");
			return -1;
		}

		if (EndFunc(context) != 0)
			return -1;
	} else {
		int labelLen;

		//It is not an instructionm, maybe it is an label.
		if (numArgs != 0) {
			SetErrorInfo(context,
				"Not an instruction nor a label\n");
			return -1;
		}

		labelLen = (int)strlen(instrOp);
		if (labelLen <= 0) {
			SetErrorInfo(context, "Label size error\n");
			return -1;
		}

		if (instrOp[labelLen - 1] != ':') {
			SetErrorInfo(context, "Label should end with \":\"");
			return -1;
		}

		if (AddLabel(context, instrOp) != 0)
			return -1;
	}
	return 0;
}

int SvmParseScript(struct _SvmParserContext_t *context,
	char *buf, int len)
{
	struct _SvmStringStream_t stringStream;
	int numWords;
	int lineLen;
	int lineNum;
	int ret;
	int tmp_i = 1;

	if (InitStringStream(context, &stringStream, buf, len) != 0)
		return -1;

	lineNum = 0;
	while (tmp_i > 0) {
		lineLen =
			GetLine(context, &stringStream, context->lineBuf,
			MAX_LINE_LEN);
		if (lineLen == END_OF_STREAM)
			break;
		else if (lineLen < 0)
			return -1;

		lineNum++;
		numWords =
			Split(context, context->lineBuf, lineLen,
				&context->wordsBuf[0][0], MAX_WORDS,
				MAX_WORD_LEN);
		if (numWords < 0) {
			SetErrLineNum(context, lineNum);
			return -1;
		} else if (numWords == 0)
			continue;
		else {
//			int i;

//			SVM_LOG_DEBUG("line:%s\n", context->lineBuf);
//			SVM_LOG_DEBUG("Words:%d\n", numWords);

//			for (i = 0; i < numWords; i++) {
//				SVM_LOG_DEBUG("Word[%d]=%s\n", i,
//					context->wordsBuf[i]);
//			}

			ret = DoSyntaxAnalyze(context,
					&context->wordsBuf[0][0],
					numWords, MAX_WORD_LEN);
			if (ret != 0)
				return -1;
		}
	}

	//Do internal instruction link
	ret = SvmInterInstrLink(context);
	if (ret != 0)
		return -1;
	return 0;
}

static int Internal2External_Op(enum _InterInstrOp_t op)
{
	int extOp = -1;

	switch (op) {
	case INTER_INSTR_OP_ADD:
		extOp = INSTR_OP_ADD;
		break;
	case INTER_INSTR_OP_SUB:
		extOp = INSTR_OP_SUB;
		break;
	case INTER_INSTR_OP_MUL:
		extOp = INSTR_OP_MUL;
		break;
	case INTER_INSTR_OP_DIV:
		extOp = INSTR_OP_DIV;
		break;
	case INTER_INSTR_OP_SHL:
		extOp = INSTR_OP_SHL;
		break;
	case INTER_INSTR_OP_SHR:
		extOp = INSTR_OP_SHR;
		break;
	case INTER_INSTR_OP_AND:
		extOp = INSTR_OP_AND;
		break;
	case INTER_INSTR_OP_OR:
		extOp = INSTR_OP_OR;
		break;
	case INTER_INSTR_OP_XOR:
		extOp = INSTR_OP_XOR;
		break;
	case INTER_INSTR_OP_MOVB:
		extOp = INSTR_OP_MOVB;
		break;
	case INTER_INSTR_OP_MOVW:
		extOp = INSTR_OP_MOVW;
		break;
	case INTER_INSTR_OP_MOVDW:
		extOp = INSTR_OP_MOVDW;
		break;
	case INTER_INSTR_OP_JNZ:
		extOp = INSTR_OP_JNZ;
		break;
	case INTER_INSTR_OP_JEZ:
		extOp = INSTR_OP_JEZ;
		break;
	case INTER_INSTR_OP_JLZ:
		extOp = INSTR_OP_JLZ;
		break;
	case INTER_INSTR_OP_JGZ:
		extOp = INSTR_OP_JGZ;
		break;
	case INTER_INSTR_OP_JGEZ:
		extOp = INSTR_OP_JGEZ;
		break;
	case INTER_INSTR_OP_JLEZ:
		extOp = INSTR_OP_JLEZ;
		break;
	case INTER_INSTR_OP_CALL:
		extOp = INSTR_OP_CALL;
		break;
	case INTER_INSTR_OP_WI2C:
		extOp = INSTR_OP_WI2C;
		break;
	case INTER_INSTR_OP_RI2C:
		extOp = INSTR_OP_RI2C;
		break;
	default:

//		SVM_LOG_ERROR("Invalid opcode\n");

		break;
	}

	return extOp;
}

int SvmGetInstrSeq(struct _SvmParserContext_t *context,
		struct _InstrSeq_t *instrSeq)
{
	int ret = 0;
	int extOp = -1;
	int i = 0;

	memset(instrSeq, 0, sizeof(struct _InstrSeq_t));

	for (i = 0; i < context->instrNum; i++) {
		// generate the device script binary according to SVM spec
		extOp = Internal2External_Op(context->instrs[i].op);
		if (extOp < 0) {
			ret = -1;
			goto EXIT;
		}

		switch (context->instrs[i].op) {
		case INTER_INSTR_OP_ADD:
		case INTER_INSTR_OP_SUB:
		case INTER_INSTR_OP_MUL:
		case INTER_INSTR_OP_DIV:
		case INTER_INSTR_OP_SHL:
		case INTER_INSTR_OP_SHR:
		case INTER_INSTR_OP_AND:
		case INTER_INSTR_OP_OR:
		case INTER_INSTR_OP_XOR:
		{
			//----------------
			//instrPart1:
			//-------------------
			//[bit31~bit27]: op
			//[bit26~bit22]: Rd register index
			//[bit21~bit17]: Rs register index
			//[bit16~bit15]: arg3 type(INSTR_ARG_TYPE_REG or
			//			INSTR_ARG_TYPE_IMM)
			//[bit14~bit0]: reserved(0)
			//
			//instrPart2:
			//-------------------
			//[bit31~bit0]: reg index(0~31) or immediate value
			//	according to the [bit16~bit15] of instrPart1

			int arg3Type = 0;

			if ((context->instrs[i].argNum != 3)
			|| (context->instrs[i].args[0].type !=
					INTER_INSTR_ARG_TYPE_REG)
			|| (context->instrs[i].args[1].type !=
					INTER_INSTR_ARG_TYPE_REG)
			|| ((context->instrs[i].args[2].type !=
						INTER_INSTR_ARG_TYPE_REG)
			&& (context->instrs[i].args[2].type !=
						INTER_INSTR_ARG_TYPE_IMM))) {

//				SVM_LOG_ERROR("Internal Instr Check Fail\n");

				ret = -1;
				goto EXIT;
			}

			arg3Type =
			(context->instrs[i].args[2].type ==
				INTER_INSTR_ARG_TYPE_REG) ?
				INSTR_ARG_TYPE_REG : INSTR_ARG_TYPE_IMM;
			instrSeq->instrs[i].instrPart1 =
				(extOp << 27) |
				(context->instrs[i].args[0].value.regIndex
									<< 22)
				| (context->instrs[i].args[1].value.regIndex
									<< 17)
				| (arg3Type << 15);
				instrSeq->instrs[i].instrPart2 =
					(arg3Type == INSTR_ARG_TYPE_REG) ?
				context->instrs[i].args[2].value.regIndex :
				context->instrs[i].args[2].value.immValue;
		}
		break;
		case INTER_INSTR_OP_MOVB:
		case INTER_INSTR_OP_MOVW:
		case INTER_INSTR_OP_MOVDW:
		{
			//=========================================
			//MOVB/MOVW/MOVDW Instruction
			//List:
			//INSTR_OP_MOVB
			//INSTR_OP_MOVW
			//INSTR_OP_MOVDW
			//Syntax:
			//MOVB dst, src
			//=========================================
			//-------------------
			//instrPart1:
			//-------------------
			//[bit31~bit27]: op
			//[bit26~bit25]:
			// dst arg type(INSTR_ARG_TYPE_REG, INSTR_ARG_TYPE_MEM)
			//[bit24~bit7]:
			//(1) if dst arg type is INISTR_ARG_TYPE_REG,
			//	 [bit24~bit20] defines the register index{0~31}
			//	 [bit19~bit7] reserved
			//(2) if dst arg type is INSTR_ARG_MEM,
			//	 [bit24] is mem{0~1} index,
			//	 [bit23] is dst mem offset type
			//	 (INSTR_MEM_OFFSET_TYPE_REG or
			//	INSTR_MEM_OFFSET_TYPE_IMM)
			//[bit22~bit7]:
			//(1) if dst mem offset type is
			//			INSTR_MEM_OFFSET_TYPE_REG
			//	 [bit22~bit18] is register index{0~31}
			//	 [bit17~bit7]reserved
			//(2) if dst mem offset type is
			//			INSTR_MEM_OFFSET_TYPE_IMM
			//	 [bit22~bit7] is the 16 bit unsigned imm value
			//[bit6~bit5]:
			//src arg type(INSTR_ARG_TYPE_REG, INSTR_ARG_TYPE_MEM
			//or INSTR_ARG_TYPE_IMM)
			//[bit4~bit0]:
			//(1) if src arg type is INSTR_ARG_TYPE_REG,
			//	[bit4~bit0] defines the register index{0~31}
			//(2) if src arg type is INSTR_ARG_MEM,
			//	[bit4] is mem{0~1} index
			//	[bit3] is src mem offset type
			//			(INSTR_MEM_OFFSET_TYPE_REG
			//			or INSTR_MEM_OFFSET_TYPE_IMM)
			//		 [bit2~bit0] reserved
			//(3) if src arg type is INSTR_ARG_TYPE_IMM
			//	[bit4~bit0] reserved
			//-------------------
			//instrPart2:
			//-------------------
			//[bit31~bit0]:
			//(1) if src arg type is INSTR_ARG_TYPE_IMM
			//	[bit31~bit0] defines
			//		 a unsigned 32 bit immedage value
			//(2) if src arg type is INSTR_ARG_TYPE_MEM and
			// the mem offset type is INSTR_MEM_OFFSET_TYPE_REG
			//	[bit31~bit27] is the register index{0~31}
			//	[bit26~bit0] reserved
			//(3) if src arg type is INSTR_ARG_TYPE_MEM and
			// the mem offset type is INSTR_MEM_OFFSET_TYPE_IMM
			//	[bit31~bit16] is the
			//			 16bit unsigned immediate value
			//	[bit15~bit0} reserved

			int dstArgType = 0, srcArgType = 0;
			int dstMemIdx = 0, srcMemIdx = 0;
			int dstRegIdx = 0, srcRegIdx = 0;
			int dstMemOffsetType = 0, srcMemOffsetType = 0;
			int dstMemOffsetRegIdx = 0;
			int srcMemOffsetRegIdx = 0;
			int dstMemOffsetImm = 0, srcMemOffsetImm = 0;
			int srcImm = 0;

			if ((context->instrs[i].argNum != 2)
			|| ((context->instrs[i].args[0].type !=
						INTER_INSTR_ARG_TYPE_REG)
			&& (context->instrs[i].args[0].type !=
						INTER_INSTR_ARG_TYPE_MEM))
			|| ((context->instrs[i].args[1].type !=
						INTER_INSTR_ARG_TYPE_REG)
			&& (context->instrs[i].args[1].type !=
						INTER_INSTR_ARG_TYPE_MEM)
			&& (context->instrs[i].args[1].type !=
						INTER_INSTR_ARG_TYPE_IMM))) {

//				SVM_LOG_ERROR
//				("Internal Instr Check Fail\n");

				ret = -1;
				goto EXIT;
			}

			if (context->instrs[i].args[0].type ==
				INTER_INSTR_ARG_TYPE_REG) {
				dstArgType = INSTR_ARG_TYPE_REG;
				dstRegIdx =
				    context->instrs[i].args[0].value.regIndex;
			} else {
				dstArgType = INSTR_ARG_TYPE_MEM;
				dstMemIdx =
			    context->instrs[i].args[0].value.memValue.memIdx;
				dstMemOffsetType =
			  (context->instrs[i].args[0].value.memValue.memIdxType
						==
						INTER_INSTR_ARG_TYPE_REG) ?
						INSTR_MEM_OFFSET_TYPE_REG :
						INSTR_MEM_OFFSET_TYPE_IMM;
				if (dstMemOffsetType ==
						INSTR_MEM_OFFSET_TYPE_REG) {
					dstMemOffsetRegIdx =
			context->instrs[i].args[0].value.memValue.idx.regIdx;
				} else {
					dstMemOffsetImm =
			context->instrs[i].args[0].value.memValue.idx.immValue;
				}
			}

			if (context->instrs[i].args[1].type ==
					INTER_INSTR_ARG_TYPE_REG) {
				srcArgType = INSTR_ARG_TYPE_REG;
				srcRegIdx =
				context->instrs[i].args[1].value.regIndex;
			} else if (context->instrs[i].args[1].type ==
					INTER_INSTR_ARG_TYPE_MEM) {
				srcArgType = INSTR_ARG_TYPE_MEM;
				srcMemIdx =
			context->instrs[i].args[1].value.memValue.memIdx;
				srcMemOffsetType =
			(context->instrs[i].args[1].value.memValue.memIdxType
					==
					INTER_INSTR_ARG_TYPE_REG) ?
					INSTR_MEM_OFFSET_TYPE_REG :
					INSTR_MEM_OFFSET_TYPE_IMM;
				if (srcMemOffsetType ==
						INSTR_MEM_OFFSET_TYPE_REG) {
					srcMemOffsetRegIdx =
			context->instrs[i].args[1].value.memValue.idx.regIdx;
				} else {
					srcMemOffsetImm =
			context->instrs[i].args[1].value.memValue.idx.immValue;
				}
			} else {
				srcArgType = INSTR_ARG_TYPE_IMM;
				srcImm =
				context->instrs[i].args[1].value.immValue;
			}

			instrSeq->instrs[i].instrPart1 = (extOp << 27)
					| (dstArgType << 25)
					| (srcArgType << 5);
			if (dstArgType == INSTR_ARG_TYPE_REG) {
				instrSeq->instrs[i].instrPart1 |=
						(dstRegIdx << 20);
			} else {// INSTR_ARG_TYPE_MEM
				instrSeq->instrs[i].instrPart1 |=
						(dstMemIdx << 24) |
						(dstMemOffsetType << 23);
				if (dstMemOffsetType ==
						INSTR_MEM_OFFSET_TYPE_REG) {
					instrSeq->instrs[i].instrPart1
					|= (dstMemOffsetRegIdx << 18);
				} else {
					instrSeq->instrs[i].instrPart1
					|= (dstMemOffsetImm << 7);
				}
			}

			if (srcArgType == INSTR_ARG_TYPE_REG) {
				instrSeq->instrs[i].instrPart1 |=
					srcRegIdx;
			} else if (srcArgType == INSTR_ARG_TYPE_MEM) {
				instrSeq->instrs[i].instrPart1 |=
					(srcMemIdx << 4) |
					(srcMemOffsetType << 3);
				if (srcMemOffsetType ==
					INSTR_MEM_OFFSET_TYPE_REG) {
					instrSeq->instrs[i].instrPart2
						|= (srcMemOffsetRegIdx << 27);
				} else {
					instrSeq->instrs[i].instrPart2
						|= (srcMemOffsetImm << 16);
				}
			} else	// INSTR_ARG_TYPE_IMM
				instrSeq->instrs[i].instrPart2 = srcImm;
		}
		break;
		case INTER_INSTR_OP_JNZ:
		case INTER_INSTR_OP_JEZ:
		case INTER_INSTR_OP_JLZ:
		case INTER_INSTR_OP_JGZ:
		case INTER_INSTR_OP_JGEZ:
		case INTER_INSTR_OP_JLEZ:
		{
		//-------------------
		//instrPart1:
		//-------------------
		//[bit31~bit27]: op
		//[bit26~bit25]:
		//arg1 type(INSTR_ARG_TYPE_REG or INSTR_ARG_TYPE_IMM)
		//[bit24~bit0]:instruction index
		//-------------------
		//instrPart2:
		//-------------------
		//[bit31~bit0]: reg index(0~31) or immediate value
		//	according to the [bit26~bit25] of instrPart1

			int arg1Type = 0;
			int instrIdx = 0;
			int part2value = 0;

			if ((context->instrs[i].argNum != 2)
			|| ((context->instrs[i].args[0].type !=
					INTER_INSTR_ARG_TYPE_REG)
			&& (context->instrs[i].args[0].type !=
					INTER_INSTR_ARG_TYPE_IMM))
			|| (context->instrs[i].args[1].type !=
						INTER_INSTR_ARG_TYPE_LABEL)) {

//				SVM_LOG_ERROR
//				("Internal Instr Check Fail\n");

				ret = -1;
				goto EXIT;
			}

			if (context->instrs[i].args[0].type ==
			    INTER_INSTR_ARG_TYPE_REG) {
				arg1Type = INSTR_ARG_TYPE_REG;
				part2value =
				    context->instrs[i].args[0].value.regIndex;
			} else {
				arg1Type = INSTR_ARG_TYPE_IMM;
				part2value =
				    context->instrs[i].args[0].value.immValue;
			}

			instrIdx =
				context->instrs[i].args[1].value.labelInstrIdx;

			instrSeq->instrs[i].instrPart1 = (extOp << 27)
					| (arg1Type << 25) | instrIdx;
			instrSeq->instrs[i].instrPart2 = part2value;
		}
		break;
		case INTER_INSTR_OP_CALL:
		{
			//-------------------
			//instrPart1:
			//-------------------
			//[bit31~bit27]: op
			//[bit26~bit25]: reserved(0)
			//[bit24~bit0]: instruction index
			//-------------------
			//instrPart2:
			//-------------------
			//[bit31~bit0]: reserved(0)

			int instrIdx = 0;

			if ((context->instrs[i].argNum != 1)
			|| (context->instrs[i].args[0].type !=
				INTER_INSTR_ARG_TYPE_FUNC)) {

//				SVM_LOG_ERROR("Internal Instr Check Fail\n");

				ret = -1;
				goto EXIT;
			}

			instrIdx = context->instrs[i].args[0].value.funcIdx;

			instrSeq->instrs[i].instrPart1 = (extOp << 27)
					| instrIdx;
			instrSeq->instrs[i].instrPart2 = 0;
		}
		break;
		case INTER_INSTR_OP_WI2C:
		{
		//----------------
		//instrPart1:
		//----------------
		//[bit31~bit27]: op
		//[bit26~bit25]:
		//i2cRegIndex arg type(INSTR_ARG_TYPE_REG or
		//				INSTR_ARG_TYPE_IMM)
		//[bit24~bit23]:
		//value arg type(INSTR_ARG_TYPE_REG or INSTR_ARG_TYPE_IMM)
		//[bit22~bit21]:
		//length arg type(INSTR_ARG_TYPE_REG or INSTR_ARG_TYPE_IMM)
		//[bit20~bit16]:
		//register index(0~31) for length
		//if its type is INSTR_ARG_TYPE_REG,
		//or an immediate value for length(range 0~4).
		//[bit15~bit0]:
		//register index(0~31) for i2cRegIndex
		//if its type is INSTR_ARG_TYPE_REG or
		//a 16bit immediate value for i2cRegIndex
		//if its type is INSTR_ARG_TYPE_IMM
		//according to [bit26~bit25] in instrPart1
		//---------------
		//instrPart2:
		//---------------
		//[bit31~bit0]:
		//immediate value for value arg if its type is
		//INSTR_ARG_TYPE_IMM or register
		//index(0~31) for value arg if its type is
		//INSTR_ARG_TYPE_REG
		//according to [bit24~bit23] in instrPart1

			int i2cRegIdxArgType = 0;
			int valArgType = 0;
			int lenArgType = 0;
			int i2cRegIdxArgVal = 0;
			int lenArgVal = 0;
			int valArgVal = 0;

			if ((context->instrs[i].argNum != 3)
			|| ((context->instrs[i].args[0].type !=
						INTER_INSTR_ARG_TYPE_REG)
			&& (context->instrs[i].args[0].type !=
						INTER_INSTR_ARG_TYPE_IMM))
			|| ((context->instrs[i].args[1].type !=
						INTER_INSTR_ARG_TYPE_REG)
			&& (context->instrs[i].args[1].type !=
						INTER_INSTR_ARG_TYPE_IMM))
			|| ((context->instrs[i].args[2].type !=
						INTER_INSTR_ARG_TYPE_REG)
			&& (context->instrs[i].args[2].type !=
						INTER_INSTR_ARG_TYPE_IMM))) {

//				SVM_LOG_ERROR
//				("Internal Instr Check Fail\n");

				ret = -1;
				goto EXIT;
			}

			if (context->instrs[i].args[0].type ==
				INTER_INSTR_ARG_TYPE_REG) {
				i2cRegIdxArgType = INSTR_ARG_TYPE_REG;
				i2cRegIdxArgVal =
				context->instrs[i].args[0].value.regIndex;
			} else {
				i2cRegIdxArgType = INSTR_ARG_TYPE_IMM;
				i2cRegIdxArgVal =
				context->instrs[i].args[0].value.immValue;
			}

			if (context->instrs[i].args[1].type ==
				INTER_INSTR_ARG_TYPE_REG) {
				valArgType = INSTR_ARG_TYPE_REG;
				valArgVal =
				context->instrs[i].args[1].value.regIndex;
			} else {
				valArgType = INSTR_ARG_TYPE_IMM;
				valArgVal =
				context->instrs[i].args[1].value.immValue;
			}

			if (context->instrs[i].args[2].type ==
				INTER_INSTR_ARG_TYPE_REG) {
				lenArgType = INSTR_ARG_TYPE_REG;
				lenArgVal =
				context->instrs[i].args[2].value.regIndex;
			} else {
				lenArgType = INSTR_ARG_TYPE_IMM;
				lenArgVal =
				context->instrs[i].args[2].value.immValue;
			}

			instrSeq->instrs[i].instrPart1 = (extOp << 27)
					| (i2cRegIdxArgType << 25)
					| (valArgType << 23)
					| (lenArgType << 21)
					| (lenArgVal << 16)
					| i2cRegIdxArgVal;
			instrSeq->instrs[i].instrPart2 = valArgVal;
		}
		break;
		case INTER_INSTR_OP_RI2C:
		{
		//----------------
		//instrPart1:
		//----------------
		//[bit31~bit27]: op
		//[bit26~bit25]:
		//i2cRegIndex arg type(INSTR_ARG_TYPE_REG
		//				or INSTR_ARG_TYPE_IMM)
		//[bit24~bit23]: reserved
		//[bit22~bit21]:
		//length arg type(INSTR_ARG_TYPE_REG or
		//INSTR_ARG_TYPE_IMM)
		//[bit20~bit16]:
		//register index(0~31) for length
		//if its type is INSTR_ARG_TYPE_REG,
		//or an immediate value for length(range 0~4).
		//[bit15~bit0]:
		//register index(0~31) for i2cRegIndex
		//if its type is INSTR_ARG_TYPE_REG or
		//a 16bit immediate value for i2cRegIndex
		//if its type is INSTR_ARG_TYPE_IMM
		//according to [bit26~bit25] in instrPart1
		//---------------
		//instrPart2:
		//---------------
		//[bit31~bit27]: Rd register index.
		//[bit26~bit0]: reserved

			int rdRegIdx = 0;
			int i2cRegIdxArgType = 0;
			int lenArgType = 0;
			int i2cRegIdxArgVal = 0;
			int lenArgVal = 0;

			if ((context->instrs[i].argNum != 3)
			|| (context->instrs[i].args[0].type !=
					INTER_INSTR_ARG_TYPE_REG)
			|| ((context->instrs[i].args[1].type !=
					INTER_INSTR_ARG_TYPE_REG)
			&& (context->instrs[i].args[1].type !=
					INTER_INSTR_ARG_TYPE_IMM))
			|| ((context->instrs[i].args[2].type !=
					INTER_INSTR_ARG_TYPE_REG)
			&& (context->instrs[i].args[2].type !=
					INTER_INSTR_ARG_TYPE_IMM))) {

//				SVM_LOG_ERROR
//				("Internal Instr Check Fail\n");

				ret = -1;
				goto EXIT;
			}

			rdRegIdx =
			context->instrs[i].args[0].value.regIndex;

			if (context->instrs[i].args[1].type ==
			INTER_INSTR_ARG_TYPE_REG) {
				i2cRegIdxArgType = INSTR_ARG_TYPE_REG;
				i2cRegIdxArgVal =
				context->instrs[i].args[1].value.regIndex;
			} else {
				i2cRegIdxArgType = INSTR_ARG_TYPE_IMM;
				i2cRegIdxArgVal =
				context->instrs[i].args[1].value.immValue;
			}

			if (context->instrs[i].args[2].type ==
			INTER_INSTR_ARG_TYPE_REG) {
				lenArgType = INSTR_ARG_TYPE_REG;
				lenArgVal =
				context->instrs[i].args[2].value.regIndex;
			} else {
				lenArgType = INSTR_ARG_TYPE_IMM;
				lenArgVal =
				context->instrs[i].args[2].value.immValue;
			}

			instrSeq->instrs[i].instrPart1 = (extOp << 27)
					| (i2cRegIdxArgType << 25)
					| (lenArgType << 21)
					| (lenArgVal << 16)
					| i2cRegIdxArgVal;
			instrSeq->instrs[i].instrPart2 =
					rdRegIdx << 27;
		}
		break;
		case INTER_INSTR_OP_INVALID:
		default:

//			SVM_LOG_ERROR("Invalid opcode\n");

			ret = -1;
			goto EXIT;
//			break;
		}
	}

	// the definition of struct _InterFuncMap_t is same with FuncMap_t
	memcpy(&instrSeq->funcs[0], &context->funcs[0],
		(context->funcNum * sizeof(struct _InterFuncMap_t)));
	instrSeq->instrNum = context->instrNum;
	instrSeq->funcNum = context->funcNum;

 EXIT:
	return ret;
}
