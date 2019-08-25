/* Header only used inside opcodes library for disassemble.

   Copyright (C) 2017-2018 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor,
   Boston, MA 02110-1301, USA.  */

#ifndef DISASSEMBLE_H
#define DISASSEMBLE_H
#include "dis-asm.h"

extern int print_insn_big_arm		(bfd_vma, disassemble_info *);
extern int print_insn_little_arm	(bfd_vma, disassemble_info *);

extern disassembler_ftype rl78_get_disassembler (bfd *);
#endif /* DISASSEMBLE_H */
