
.text
   addi	sp,sp,-32
   sd	s0,24(sp)
   addi	s0,sp,32
   csrr	zero,0x800
   sw	zero,-20(s0)
   sw	zero,-24(s0)
   lw	a5,-20(s0)
   addiw	a5,a5,1
   sw	a5,-20(s0)
   lw	a5,-24(s0)
   addiw	a5,a5,1
   sw	a5,-24(s0)
   lw	a4,-20(s0)
   lw	a5,-24(s0)
   addw	a5,a5,a4
   sw	a5,-28(s0)
   lw	a5,-28(s0)
   addiw	a5,a5,1
   sw	a5,-28(s0)
   lw	a4,-20(s0)
   lw	a5,-24(s0)
   mulw	a5,a4,a5
   sw	a5,-32(s0)
   lw	a4,-32(s0)
   lw	a5,-28(s0)
   divw	a5,a4,a5
   sw	a5,-24(s0)
   csrr	zero,0x801
   lw	a5,-28(s0)
   mv	a0,a5
   ld	s0,24(sp)
   addi	sp,sp,32
   ret

