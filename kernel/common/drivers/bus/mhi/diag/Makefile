# SPDX-License-Identifier: BSD-3-Clause-Clear

obj-$(CONFIG_HSP_DIAG) += hsp_diag.o
hsp_diag-y += diag_mhi.o \
		 diag_hdlc.o \
		 diag_msg.o \
		 diag_msg_ana.o \
		 diag.o \
		 trace.o

# for tracing framework to find trace.h
CFLAGS_trace.o := -I$(src)
