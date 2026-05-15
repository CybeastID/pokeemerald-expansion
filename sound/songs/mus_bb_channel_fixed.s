	.include "sound/MPlayDef.s"

	.equ	mus_bb_channel_grp, voicegroup_dummy
	.equ	mus_bb_channel_pri, 0
	.equ	mus_bb_channel_rev, 0
	.equ	mus_bb_channel_mvl, 127
	.equ	mus_bb_channel_key, 0
	.equ	mus_bb_channel_tbs, 1
	.equ	mus_bb_channel_exg, 0
	.equ	mus_bb_channel_cmp, 1

	.section .rodata
	.global	mus_bb_channel_fixed
	.align	2

@**************** Track 1 (Midi-Chn.2) ****************@

mus_bb_channel_1:
	.byte	KEYSH , mus_bb_channel_key+0
@ 000   ----------------------------------------
	.byte	TEMPO , 160*mus_bb_channel_tbs/2
	.byte		VOICE , 1
	.byte		VOL   , 111*mus_bb_channel_mvl/mxv
	.byte		PAN   , c_v+0
	.byte	W96
@ 001   ----------------------------------------
	.byte	W96
@ 002   ----------------------------------------
	.byte	W96
@ 003   ----------------------------------------
	.byte	W96
@ 004   ----------------------------------------
	.byte	W96
@ 005   ----------------------------------------
	.byte	W96
@ 006   ----------------------------------------
	.byte	W96
@ 007   ----------------------------------------
	.byte	W96
mus_bb_loop_point1:
.byte	TEMPO , 160*mus_bb_channel_tbs/2
@ 008   ----------------------------------------
	.byte		N22   , Fs3 , v096
	.byte	W22
	.byte	W02
	.byte		        Bn3 
	.byte	W22
	.byte	W02
	.byte		        As3 
	.byte	W22
	.byte	W02
	.byte		        Bn3 
	.byte	W22
	.byte	W02
@ 009   ----------------------------------------
	.byte		        Dn4 
	.byte	W22
	.byte	W02
	.byte		        Cs4 
	.byte	W22
	.byte	W02
	.byte		        Bn3 
	.byte	W22
	.byte	W02
	.byte		        As3 
	.byte	W22
	.byte	W02
@ 010   ----------------------------------------
	.byte		        Bn3 
	.byte	W22
	.byte	W02
	.byte		        Fs4 
	.byte	W22
	.byte	W02
	.byte		        En4 
	.byte	W22
	.byte	W02
	.byte		        Gn4 
	.byte	W22
	.byte	W02
@ 011   ----------------------------------------
	.byte		N11   , Fs4 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
	.byte		        Dn4 
	.byte	W11
	.byte	W01
	.byte		        Cs4 
	.byte	W11
	.byte	W01
	.byte		N22   , Fs3 
	.byte	W22
	.byte	W02
	.byte		        An3 
	.byte	W22
	.byte	W02
@ 012   ----------------------------------------
	.byte		        Bn3 
	.byte	W22
	.byte	W02
	.byte		        Fs4 
	.byte	W22
	.byte	W02
	.byte		        En4 
	.byte	W22
	.byte	W02
	.byte		        Bn4 
	.byte	W22
	.byte	W02
@ 013   ----------------------------------------
	.byte		        An4 
	.byte	W22
	.byte	W02
	.byte		N11   , Gn4 
	.byte	W11
	.byte	W01
	.byte		        Fs4 
	.byte	W11
	.byte	W01
	.byte		N22   , Fn4 
	.byte	W22
	.byte	W02
	.byte		        Fs4 
	.byte	W22
	.byte	W02
@ 014   ----------------------------------------
	.byte		        Gn4 
	.byte	W22
	.byte	W02
	.byte		N11   , Fs4 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
	.byte		N22   , Dn4 
	.byte	W22
	.byte	W02
	.byte		N22   
	.byte	W22
	.byte	W02
@ 015   ----------------------------------------
mus_bb_channel_1_015:
	.byte		N32   , Dn4 , v096
	.byte	W32
	.byte	W02
	.byte	PEND
	.byte	W02
	.byte		N05   , En4 
	.byte	W05
	.byte	W01
	.byte		        Dn4 
	.byte	W05
	.byte	W01
	.byte		N44   , Cs4 
	.byte	W44
	.byte	W01
	.byte	W03
@ 016   ----------------------------------------
	.byte		N22   , Fs3 
	.byte	W22
	.byte	W02
	.byte		        Bn3 
	.byte	W22
	.byte	W02
	.byte		        As3 
	.byte	W22
	.byte	W02
	.byte		        Bn3 
	.byte	W22
	.byte	W02
@ 017   ----------------------------------------
	.byte		        Dn4 
	.byte	W22
	.byte	W02
	.byte		        Cs4 
	.byte	W22
	.byte	W02
	.byte		        Bn3 
	.byte	W22
	.byte	W02
	.byte		        As3 
	.byte	W22
	.byte	W02
@ 018   ----------------------------------------
	.byte		        Bn3 
	.byte	W22
	.byte	W02
	.byte		        Fs4 
	.byte	W22
	.byte	W02
	.byte		        En4 
	.byte	W22
	.byte	W02
	.byte		        Gn4 
	.byte	W22
	.byte	W02
@ 019   ----------------------------------------
	.byte		N11   , Fs4 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
	.byte		        Dn4 
	.byte	W11
	.byte	W01
	.byte		        Cs4 
	.byte	W11
	.byte	W01
	.byte		N22   , Fs3 
	.byte	W22
	.byte	W02
	.byte		        An3 
	.byte	W22
	.byte	W02
@ 020   ----------------------------------------
	.byte		        Bn3 
	.byte	W22
	.byte	W02
	.byte		        Fs4 
	.byte	W22
	.byte	W02
	.byte		        En4 
	.byte	W22
	.byte	W02
	.byte		        Bn4 
	.byte	W22
	.byte	W02
@ 021   ----------------------------------------
	.byte		        An4 
	.byte	W22
	.byte	W02
	.byte		N11   , Gn4 
	.byte	W11
	.byte	W01
	.byte		        Fs4 
	.byte	W11
	.byte	W01
	.byte		N22   , Fn4 
	.byte	W22
	.byte	W02
	.byte		        Fs4 
	.byte	W22
	.byte	W02
@ 022   ----------------------------------------
	.byte		        Gn4 
	.byte	W22
	.byte	W02
	.byte		N11   , Fs4 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
	.byte		N22   , Dn4 
	.byte	W22
	.byte	W02
	.byte		N22   
	.byte	W22
	.byte	W02
@ 023   ----------------------------------------
	.byte		N22   
	.byte	W22
	.byte	W02
	.byte		        As4 
	.byte	W22
	.byte	W02
	.byte		        Bn4 
	.byte	W22
	.byte	W02
	.byte		        As4 
	.byte	W22
	.byte	W02
@ 024   ----------------------------------------
mus_bb_channel_1_024:
	.byte		N90   , Bn4 , v096
	.byte	W90
	.byte	W01
	.byte	PEND
	.byte	W05
@ 025   ----------------------------------------
	.byte	W96
@ 026   ----------------------------------------
	.byte	W96
@ 027   ----------------------------------------
	.byte	W96
@ 028   ----------------------------------------
	.byte	W96
@ 029   ----------------------------------------
	.byte	W96
@ 030   ----------------------------------------
	.byte	W96
@ 031   ----------------------------------------
	.byte	W96
@ 032   ----------------------------------------
	.byte	W96
@ 033   ----------------------------------------
	.byte	W96
@ 034   ----------------------------------------
mus_bb_channel_1_034:
	.byte		N44   , Gn2 , v096
	.byte	W44
	.byte	W01
	.byte	PEND
	.byte	W03
	.byte		N11   
	.byte	W11
	.byte	W01
	.byte		        An2 
	.byte	W11
	.byte	W01
	.byte		N22   , As2 
	.byte	W22
	.byte	W02
@ 035   ----------------------------------------
mus_bb_channel_1_035:
	.byte		N44   , Bn2 , v096
	.byte	W44
	.byte	W01
	.byte	PEND
	.byte	W03
	.byte		N11   
	.byte	W11
	.byte	W01
	.byte		        Cs3 
	.byte	W11
	.byte	W01
	.byte		N22   , Dn3 
	.byte	W22
	.byte	W02
@ 036   ----------------------------------------
mus_bb_channel_1_036:
	.byte		N44   , En3 , v096
	.byte	W44
	.byte	W01
	.byte	PEND
	.byte	W03
	.byte		N11   
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Gn3 
	.byte	W11
	.byte	W01
	.byte		        As3 
	.byte	W11
	.byte	W01
@ 037   ----------------------------------------
	.byte		N22   , Bn3 
	.byte	W22
	.byte	W02
	.byte		        An3 
	.byte	W22
	.byte	W02
	.byte		        Gn3 
	.byte	W22
	.byte	W02
	.byte		        Fs3 
	.byte	W22
	.byte	W02
@ 038   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_1_036
	.byte	W03
	.byte		N11   , En3 , v096
	.byte	W11
	.byte	W01
	.byte		        Dn3 
	.byte	W11
	.byte	W01
	.byte		        Cs3 
	.byte	W11
	.byte	W01
	.byte		        Bn2 
	.byte	W11
	.byte	W01
@ 039   ----------------------------------------
mus_bb_channel_1_039:
	.byte		N44   , Cs3 , v096
	.byte	W44
	.byte	W01
	.byte	PEND
	.byte	W03
	.byte		N11   
	.byte	W11
	.byte	W01
	.byte		        Dn3 
	.byte	W11
	.byte	W01
	.byte		        Ds3 
	.byte	W11
	.byte	W01
	.byte		        En3 
	.byte	W11
	.byte	W01
@ 040   ----------------------------------------
	.byte		N22   , Fs3 
	.byte	W22
	.byte	W02
	.byte		        Fs2 
	.byte	W22
	.byte	W02
	.byte		        Fs3 
	.byte	W22
	.byte	W02
	.byte		        Fs2 
	.byte	W22
	.byte	W02
@ 041   ----------------------------------------
	.byte		        Fs3 
	.byte	W22
	.byte	W02
	.byte		        Fs2 
	.byte	W22
	.byte	W02
	.byte		N44   , Fs3 
	.byte	W44
	.byte	W01
	.byte	W03
@ 042   ----------------------------------------
	.byte	W96
@ 043   ----------------------------------------
	.byte	W96
@ 044   ----------------------------------------
	.byte	W96
@ 045   ----------------------------------------
	.byte	W96
@ 046   ----------------------------------------
	.byte	W96
@ 047   ----------------------------------------
	.byte	W96
@ 048   ----------------------------------------
	.byte	W96
@ 049   ----------------------------------------
	.byte	W96
@ 050   ----------------------------------------
	.byte		N22   
	.byte	W22
	.byte	W02
	.byte		        Bn3 
	.byte	W22
	.byte	W02
	.byte		        As3 
	.byte	W22
	.byte	W02
	.byte		        Bn3 
	.byte	W22
	.byte	W02
@ 051   ----------------------------------------
	.byte		        Dn4 
	.byte	W22
	.byte	W02
	.byte		        Cs4 
	.byte	W22
	.byte	W02
	.byte		        Bn3 
	.byte	W22
	.byte	W02
	.byte		        As3 
	.byte	W22
	.byte	W02
@ 052   ----------------------------------------
	.byte		        Bn3 
	.byte	W22
	.byte	W02
	.byte		        Fs4 
	.byte	W22
	.byte	W02
	.byte		        En4 
	.byte	W22
	.byte	W02
	.byte		        Gn4 
	.byte	W22
	.byte	W02
@ 053   ----------------------------------------
	.byte		N11   , Fs4 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
	.byte		        Dn4 
	.byte	W11
	.byte	W01
	.byte		        Cs4 
	.byte	W11
	.byte	W01
	.byte		N22   , Fs3 
	.byte	W22
	.byte	W02
	.byte		        An3 
	.byte	W22
	.byte	W02
@ 054   ----------------------------------------
	.byte		        Bn3 
	.byte	W22
	.byte	W02
	.byte		        Fs4 
	.byte	W22
	.byte	W02
	.byte		        En4 
	.byte	W22
	.byte	W02
	.byte		        Bn4 
	.byte	W22
	.byte	W02
@ 055   ----------------------------------------
	.byte		        An4 
	.byte	W22
	.byte	W02
	.byte		N11   , Gn4 
	.byte	W11
	.byte	W01
	.byte		        Fs4 
	.byte	W11
	.byte	W01
	.byte		N22   , Fn4 
	.byte	W22
	.byte	W02
	.byte		        Fs4 
	.byte	W22
	.byte	W02
@ 056   ----------------------------------------
	.byte		        Gn4 
	.byte	W22
	.byte	W02
	.byte		N11   , Fs4 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
	.byte		N22   , Dn4 
	.byte	W22
	.byte	W02
	.byte		N22   
	.byte	W22
	.byte	W02
@ 057   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_1_015
	.byte	W02
	.byte		N05   , En4 , v096
	.byte	W05
	.byte	W01
	.byte		        Dn4 
	.byte	W05
	.byte	W01
	.byte		N44   , Cs4 
	.byte	W44
	.byte	W01
	.byte	W03
@ 058   ----------------------------------------
	.byte		N22   , Fs3 
	.byte	W22
	.byte	W02
	.byte		        Bn3 
	.byte	W22
	.byte	W02
	.byte		        As3 
	.byte	W22
	.byte	W02
	.byte		        Bn3 
	.byte	W22
	.byte	W02
@ 059   ----------------------------------------
	.byte		        Dn4 
	.byte	W22
	.byte	W02
	.byte		        Cs4 
	.byte	W22
	.byte	W02
	.byte		        Bn3 
	.byte	W22
	.byte	W02
	.byte		        As3 
	.byte	W22
	.byte	W02
@ 060   ----------------------------------------
	.byte		        Bn3 
	.byte	W22
	.byte	W02
	.byte		        Fs4 
	.byte	W22
	.byte	W02
	.byte		        En4 
	.byte	W22
	.byte	W02
	.byte		        Gn4 
	.byte	W22
	.byte	W02
@ 061   ----------------------------------------
	.byte		N11   , Fs4 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
	.byte		        Dn4 
	.byte	W11
	.byte	W01
	.byte		        Cs4 
	.byte	W11
	.byte	W01
	.byte		N22   , Fs3 
	.byte	W22
	.byte	W02
	.byte		        An3 
	.byte	W22
	.byte	W02
@ 062   ----------------------------------------
	.byte		        Bn3 
	.byte	W22
	.byte	W02
	.byte		        Fs4 
	.byte	W22
	.byte	W02
	.byte		        En4 
	.byte	W22
	.byte	W02
	.byte		        Bn4 
	.byte	W22
	.byte	W02
@ 063   ----------------------------------------
	.byte		        An4 
	.byte	W22
	.byte	W02
	.byte		N11   , Gn4 
	.byte	W11
	.byte	W01
	.byte		        Fs4 
	.byte	W11
	.byte	W01
	.byte		N22   , Fn4 
	.byte	W22
	.byte	W02
	.byte		        Fs4 
	.byte	W22
	.byte	W02
@ 064   ----------------------------------------
	.byte		        Gn4 
	.byte	W22
	.byte	W02
	.byte		N11   , Fs4 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
	.byte		N22   , Dn4 
	.byte	W22
	.byte	W02
	.byte		N22   
	.byte	W22
	.byte	W02
@ 065   ----------------------------------------
	.byte		N22   
	.byte	W22
	.byte	W02
	.byte		        As4 
	.byte	W22
	.byte	W02
	.byte		        Bn4 
	.byte	W22
	.byte	W02
	.byte		        As4 
	.byte	W22
	.byte	W02
@ 066   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_1_024
	.byte	W05
@ 067   ----------------------------------------
	.byte	W96
@ 068   ----------------------------------------
	.byte	W96
@ 069   ----------------------------------------
	.byte	W96
@ 070   ----------------------------------------
	.byte	W96
@ 071   ----------------------------------------
	.byte	W96
@ 072   ----------------------------------------
	.byte	W96
@ 073   ----------------------------------------
	.byte	W96
@ 074   ----------------------------------------
	.byte	W96
@ 075   ----------------------------------------
	.byte	W96
@ 076   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_1_034
	.byte	W03
	.byte		N11   , Gn2 , v096
	.byte	W11
	.byte	W01
	.byte		        An2 
	.byte	W11
	.byte	W01
	.byte		N22   , As2 
	.byte	W22
	.byte	W02
@ 077   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_1_035
	.byte	W03
	.byte		N11   , Bn2 , v096
	.byte	W11
	.byte	W01
	.byte		        Cs3 
	.byte	W11
	.byte	W01
	.byte		N22   , Dn3 
	.byte	W22
	.byte	W02
@ 078   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_1_036
	.byte	W03
	.byte		N11   , En3 , v096
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Gn3 
	.byte	W11
	.byte	W01
	.byte		        As3 
	.byte	W11
	.byte	W01
@ 079   ----------------------------------------
	.byte		N22   , Bn3 
	.byte	W22
	.byte	W02
	.byte		        An3 
	.byte	W22
	.byte	W02
	.byte		        Gn3 
	.byte	W22
	.byte	W02
	.byte		        Fs3 
	.byte	W22
	.byte	W02
@ 080   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_1_036
	.byte	W03
	.byte		N11   , En3 , v096
	.byte	W11
	.byte	W01
	.byte		        Dn3 
	.byte	W11
	.byte	W01
	.byte		        Cs3 
	.byte	W11
	.byte	W01
	.byte		        Bn2 
	.byte	W11
	.byte	W01
@ 081   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_1_039
	.byte	W03
	.byte		N11   , Cs3 , v096
	.byte	W11
	.byte	W01
	.byte		        Dn3 
	.byte	W11
	.byte	W01
	.byte		        Ds3 
	.byte	W11
	.byte	W01
	.byte		        En3 
	.byte	W11
	.byte	W01
@ 082   ----------------------------------------
	.byte		N22   , Fs3 
	.byte	W22
	.byte	W02
	.byte		        Fs2 
	.byte	W22
	.byte	W02
	.byte		        Fs3 
	.byte	W22
	.byte	W02
	.byte		        Fs2 
	.byte	W22
	.byte	W02
@ 083   ----------------------------------------
	.byte		        Fs3 
	.byte	W22
	.byte	W02
	.byte		        Fs2 
	.byte	W22
	.byte	W02
	.byte		N44   , Fs3 
	.byte	W44
	.byte	W01
	.byte	W03
@ 084   ----------------------------------------
	.byte	W96
@ 085   ----------------------------------------
	.byte	W96
@ 086   ----------------------------------------
	.byte	W96
@ 087   ----------------------------------------
	.byte	W96
@ 088   ----------------------------------------
	.byte	W96
@ 089   ----------------------------------------
	.byte	W96
@ 090   ----------------------------------------
	.byte	W96
@ 091   ----------------------------------------
	.byte	W96
	.byte	0xB2                 @ Direct Jump command
	.word   mus_bb_loop_point1

@**************** Track 2 (Midi-Chn.1) ****************@

mus_bb_channel_2:
	.byte	KEYSH , mus_bb_channel_key+0
@ 000   ----------------------------------------
	.byte		VOICE , 48
	.byte		VOL   , 100*mus_bb_channel_mvl/mxv
	.byte		PAN   , c_v+0
	.byte	W96
@ 001   ----------------------------------------
	.byte	W96
@ 002   ----------------------------------------
	.byte	W96
@ 003   ----------------------------------------
	.byte	W96
@ 004   ----------------------------------------
	.byte	W96
@ 005   ----------------------------------------
	.byte	W96
@ 006   ----------------------------------------
	.byte	W96
@ 007   ----------------------------------------
	.byte	W96
mus_bb_loop_point2:
	.byte	TEMPO , 160*mus_bb_channel_tbs/2
@ 008   ----------------------------------------
mus_bb_channel_2_008:
	.byte		N44   , Fs4 , v080
	.byte	W44
	.byte	W03
	.byte	PEND
	.byte	W01
	.byte		N23   , Fn4 
	.byte	W23
	.byte	W01
	.byte		        Fs4 
	.byte	W23
	.byte	W01
@ 009   ----------------------------------------
mus_bb_channel_2_009:
	.byte		N44   , Gn4 , v080
	.byte	W44
	.byte	W03
	.byte	PEND
	.byte	W01
	.byte		        Fs4 
	.byte	W44
	.byte	W03
	.byte	W01
@ 010   ----------------------------------------
mus_bb_channel_2_010:
	.byte		N44   , Dn4 , v080
	.byte	W44
	.byte	W03
	.byte	PEND
	.byte	W01
	.byte		N23   , Cs4 
	.byte	W23
	.byte	W01
	.byte		        En4 
	.byte	W23
	.byte	W01
@ 011   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_2_010
	.byte	W01
	.byte		N44   , Fs3 , v080
	.byte	W44
	.byte	W03
	.byte	W01
@ 012   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_2_008
	.byte	W01
	.byte		N23   , Fn4 , v080
	.byte	W23
	.byte	W01
	.byte		        Fs4 
	.byte	W23
	.byte	W01
@ 013   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_2_009
	.byte	W01
	.byte		N44   , Fs4 , v080
	.byte	W44
	.byte	W03
	.byte	W01
@ 014   ----------------------------------------
	.byte		N23   
	.byte	W23
	.byte	W01
	.byte		        En4 
	.byte	W23
	.byte	W01
	.byte		        Gn4 
	.byte	W23
	.byte	W01
	.byte		        Fs4 
	.byte	W23
	.byte	W01
@ 015   ----------------------------------------
	.byte		        An4 
	.byte	W23
	.byte	W01
	.byte		        Gn4 
	.byte	W23
	.byte	W01
	.byte		        Bn4 
	.byte	W23
	.byte	W01
	.byte		        As4 
	.byte	W23
	.byte	W01
@ 016   ----------------------------------------
mus_bb_channel_2_016:
	.byte		N44   , Bn4 , v080
	.byte	W44
	.byte	W03
	.byte	PEND
	.byte	W01
	.byte		N23   , Fn4 
	.byte	W23
	.byte	W01
	.byte		        Fs4 
	.byte	W23
	.byte	W01
@ 017   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_2_009
	.byte	W01
	.byte		N44   , Fs4 , v080
	.byte	W44
	.byte	W03
	.byte	W01
@ 018   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_2_010
	.byte	W01
	.byte		N23   , Cs4 , v080
	.byte	W23
	.byte	W01
	.byte		        En4 
	.byte	W23
	.byte	W01
@ 019   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_2_010
	.byte	W01
	.byte		N44   , Fs3 , v080
	.byte	W44
	.byte	W03
	.byte	W01
@ 020   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_2_008
	.byte	W01
	.byte		N23   , Fn4 , v080
	.byte	W23
	.byte	W01
	.byte		        Fs4 
	.byte	W23
	.byte	W01
@ 021   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_2_009
	.byte	W01
	.byte		N44   , Fs4 , v080
	.byte	W44
	.byte	W03
	.byte	W01
@ 022   ----------------------------------------
	.byte		N23   
	.byte	W23
	.byte	W01
	.byte		        En4 
	.byte	W23
	.byte	W01
	.byte		        Gn4 
	.byte	W23
	.byte	W01
	.byte		        Fs4 
	.byte	W23
	.byte	W01
@ 023   ----------------------------------------
	.byte		        An4 
	.byte	W23
	.byte	W01
	.byte		        Gn4 
	.byte	W23
	.byte	W01
	.byte		        Bn4 
	.byte	W23
	.byte	W01
	.byte		        As4 
	.byte	W23
	.byte	W01
@ 024   ----------------------------------------
mus_bb_channel_2_024:
	.byte		N92   , Bn4 , v080
	.byte	W92
	.byte	W03
	.byte	PEND
	.byte	W01
@ 025   ----------------------------------------
	.byte	W96
@ 026   ----------------------------------------
mus_bb_channel_2_026:
	.byte		N44   , Dn5 , v112
	.byte	W44
	.byte	W03
	.byte	PEND
	.byte	W01
	.byte		        Cs5 
	.byte	W44
	.byte	W03
	.byte	W01
@ 027   ----------------------------------------
	.byte		N23   , Bn4 
	.byte	W23
	.byte	W01
	.byte		N11   , Fs4 
	.byte	W11
	.byte	W01
	.byte		        Gn4 
	.byte	W11
	.byte	W01
	.byte		N23   , As4 
	.byte	W23
	.byte	W01
	.byte		        Cs5 
	.byte	W23
	.byte	W01
@ 028   ----------------------------------------
	.byte		N11   , Bn4 
	.byte	W11
	.byte	W01
	.byte		        Cs5 
	.byte	W11
	.byte	W01
	.byte		N23   , Dn5 
	.byte	W23
	.byte	W01
	.byte		        En5 
	.byte	W23
	.byte	W01
	.byte		        Fs5 
	.byte	W23
	.byte	W01
@ 029   ----------------------------------------
	.byte		        Gn5 
	.byte	W23
	.byte	W01
	.byte		        Fs5 
	.byte	W23
	.byte	W01
	.byte		        Dn5 
	.byte	W23
	.byte	W01
	.byte		        Cs5 
	.byte	W23
	.byte	W01
@ 030   ----------------------------------------
	.byte		        Bn4 
	.byte	W23
	.byte	W01
	.byte		        Fs5 
	.byte	W23
	.byte	W01
	.byte		        Fn5 
	.byte	W23
	.byte	W01
	.byte		        Fs5 
	.byte	W23
	.byte	W01
@ 031   ----------------------------------------
	.byte		        Dn5 
	.byte	W23
	.byte	W01
	.byte		        En5 
	.byte	W23
	.byte	W01
	.byte		        Cs5 
	.byte	W23
	.byte	W01
	.byte		        Fs4 
	.byte	W23
	.byte	W01
@ 032   ----------------------------------------
	.byte		N11   , Gn4 
	.byte	W11
	.byte	W01
	.byte		        An4 
	.byte	W11
	.byte	W01
	.byte		        Bn4 
	.byte	W11
	.byte	W01
	.byte		        Cs5 
	.byte	W11
	.byte	W01
	.byte		N23   , Dn5 
	.byte	W23
	.byte	W01
	.byte		        Cs5 
	.byte	W23
	.byte	W01
@ 033   ----------------------------------------
	.byte		        Bn4 
	.byte	W23
	.byte	W01
	.byte		N11   , As4 
	.byte	W11
	.byte	W01
	.byte		        Cs5 
	.byte	W11
	.byte	W01
	.byte		N44   , Fs4 
	.byte	W44
	.byte	W03
	.byte	W01
@ 034   ----------------------------------------
	.byte	W96
@ 035   ----------------------------------------
	.byte	W96
@ 036   ----------------------------------------
	.byte	W96
@ 037   ----------------------------------------
	.byte	W96
@ 038   ----------------------------------------
	.byte	W96
@ 039   ----------------------------------------
	.byte	W96
@ 040   ----------------------------------------
	.byte	W96
@ 041   ----------------------------------------
	.byte	W96
@ 042   ----------------------------------------
	.byte		N23   , Bn4 , v096
	.byte	W23
	.byte	W01
	.byte		        As4 
	.byte	W23
	.byte	W01
	.byte		        An4 
	.byte	W23
	.byte	W01
	.byte		        As4 
	.byte	W23
	.byte	W01
@ 043   ----------------------------------------
	.byte		        Bn4 
	.byte	W23
	.byte	W01
	.byte		        As4 
	.byte	W23
	.byte	W01
	.byte		        An4 
	.byte	W23
	.byte	W01
	.byte		        As4 
	.byte	W23
	.byte	W01
@ 044   ----------------------------------------
	.byte		        Bn4 
	.byte	W23
	.byte	W01
	.byte		        As4 
	.byte	W23
	.byte	W01
	.byte		        An4 
	.byte	W23
	.byte	W01
	.byte		        As4 
	.byte	W23
	.byte	W01
@ 045   ----------------------------------------
	.byte		        Bn4 
	.byte	W23
	.byte	W01
	.byte		        As4 
	.byte	W23
	.byte	W01
	.byte		        An4 
	.byte	W23
	.byte	W01
	.byte		        As4 
	.byte	W23
	.byte	W01
@ 046   ----------------------------------------
	.byte		        Dn5 
	.byte	W23
	.byte	W01
	.byte		        Cs5 
	.byte	W23
	.byte	W01
	.byte		        Bn4 
	.byte	W23
	.byte	W01
	.byte		        Cs5 
	.byte	W23
	.byte	W01
@ 047   ----------------------------------------
	.byte		        En5 
	.byte	W23
	.byte	W01
	.byte		        Dn5 
	.byte	W23
	.byte	W01
	.byte		        Cs5 
	.byte	W23
	.byte	W01
	.byte		        Dn5 
	.byte	W23
	.byte	W01
@ 048   ----------------------------------------
	.byte		        Cs5 
	.byte	W23
	.byte	W01
	.byte		        Bn4 
	.byte	W23
	.byte	W01
	.byte		        As4 
	.byte	W23
	.byte	W01
	.byte		        Cs5 
	.byte	W23
	.byte	W01
@ 049   ----------------------------------------
	.byte		        Fs5 
	.byte	W23
	.byte	W01
	.byte		        Fs4 
	.byte	W23
	.byte	W01
	.byte		N44   , Cs5 
	.byte	W44
	.byte	W03
	.byte	W01
@ 050   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_2_008
	.byte	W01
	.byte		N23   , Fn4 , v080
	.byte	W23
	.byte	W01
	.byte		        Fs4 
	.byte	W23
	.byte	W01
@ 051   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_2_009
	.byte	W01
	.byte		N44   , Fs4 , v080
	.byte	W44
	.byte	W03
	.byte	W01
@ 052   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_2_010
	.byte	W01
	.byte		N23   , Cs4 , v080
	.byte	W23
	.byte	W01
	.byte		        En4 
	.byte	W23
	.byte	W01
@ 053   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_2_010
	.byte	W01
	.byte		N44   , Fs3 , v080
	.byte	W44
	.byte	W03
	.byte	W01
@ 054   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_2_008
	.byte	W01
	.byte		N23   , Fn4 , v080
	.byte	W23
	.byte	W01
	.byte		        Fs4 
	.byte	W23
	.byte	W01
@ 055   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_2_009
	.byte	W01
	.byte		N44   , Fs4 , v080
	.byte	W44
	.byte	W03
	.byte	W01
@ 056   ----------------------------------------
	.byte		N23   
	.byte	W23
	.byte	W01
	.byte		        En4 
	.byte	W23
	.byte	W01
	.byte		        Gn4 
	.byte	W23
	.byte	W01
	.byte		        Fs4 
	.byte	W23
	.byte	W01
@ 057   ----------------------------------------
	.byte		        An4 
	.byte	W23
	.byte	W01
	.byte		        Gn4 
	.byte	W23
	.byte	W01
	.byte		        Bn4 
	.byte	W23
	.byte	W01
	.byte		        As4 
	.byte	W23
	.byte	W01
@ 058   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_2_016
	.byte	W01
	.byte		N23   , Fn4 , v080
	.byte	W23
	.byte	W01
	.byte		        Fs4 
	.byte	W23
	.byte	W01
@ 059   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_2_009
	.byte	W01
	.byte		N44   , Fs4 , v080
	.byte	W44
	.byte	W03
	.byte	W01
@ 060   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_2_010
	.byte	W01
	.byte		N23   , Cs4 , v080
	.byte	W23
	.byte	W01
	.byte		        En4 
	.byte	W23
	.byte	W01
@ 061   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_2_010
	.byte	W01
	.byte		N44   , Fs3 , v080
	.byte	W44
	.byte	W03
	.byte	W01
@ 062   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_2_008
	.byte	W01
	.byte		N23   , Fn4 , v080
	.byte	W23
	.byte	W01
	.byte		        Fs4 
	.byte	W23
	.byte	W01
@ 063   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_2_009
	.byte	W01
	.byte		N44   , Fs4 , v080
	.byte	W44
	.byte	W03
	.byte	W01
@ 064   ----------------------------------------
	.byte		N23   
	.byte	W23
	.byte	W01
	.byte		        En4 
	.byte	W23
	.byte	W01
	.byte		        Gn4 
	.byte	W23
	.byte	W01
	.byte		        Fs4 
	.byte	W23
	.byte	W01
@ 065   ----------------------------------------
	.byte		        An4 
	.byte	W23
	.byte	W01
	.byte		        Gn4 
	.byte	W23
	.byte	W01
	.byte		        Bn4 
	.byte	W23
	.byte	W01
	.byte		        As4 
	.byte	W23
	.byte	W01
@ 066   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_2_024
	.byte	W01
@ 067   ----------------------------------------
	.byte	W96
@ 068   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_2_026
	.byte	W01
	.byte		N44   , Cs5 , v112
	.byte	W44
	.byte	W03
	.byte	W01
@ 069   ----------------------------------------
	.byte		N23   , Bn4 
	.byte	W23
	.byte	W01
	.byte		N11   , Fs4 
	.byte	W11
	.byte	W01
	.byte		        Gn4 
	.byte	W11
	.byte	W01
	.byte		N23   , As4 
	.byte	W23
	.byte	W01
	.byte		        Cs5 
	.byte	W23
	.byte	W01
@ 070   ----------------------------------------
	.byte		N11   , Bn4 
	.byte	W11
	.byte	W01
	.byte		        Cs5 
	.byte	W11
	.byte	W01
	.byte		N23   , Dn5 
	.byte	W23
	.byte	W01
	.byte		        En5 
	.byte	W23
	.byte	W01
	.byte		        Fs5 
	.byte	W23
	.byte	W01
@ 071   ----------------------------------------
	.byte		        Gn5 
	.byte	W23
	.byte	W01
	.byte		        Fs5 
	.byte	W23
	.byte	W01
	.byte		        Dn5 
	.byte	W23
	.byte	W01
	.byte		        Cs5 
	.byte	W23
	.byte	W01
@ 072   ----------------------------------------
	.byte		        Bn4 
	.byte	W23
	.byte	W01
	.byte		        Fs5 
	.byte	W23
	.byte	W01
	.byte		        Fn5 
	.byte	W23
	.byte	W01
	.byte		        Fs5 
	.byte	W23
	.byte	W01
@ 073   ----------------------------------------
	.byte		        Dn5 
	.byte	W23
	.byte	W01
	.byte		        En5 
	.byte	W23
	.byte	W01
	.byte		        Cs5 
	.byte	W23
	.byte	W01
	.byte		        Fs4 
	.byte	W23
	.byte	W01
@ 074   ----------------------------------------
	.byte		N11   , Gn4 
	.byte	W11
	.byte	W01
	.byte		        An4 
	.byte	W11
	.byte	W01
	.byte		        Bn4 
	.byte	W11
	.byte	W01
	.byte		        Cs5 
	.byte	W11
	.byte	W01
	.byte		N23   , Dn5 
	.byte	W23
	.byte	W01
	.byte		        Cs5 
	.byte	W23
	.byte	W01
@ 075   ----------------------------------------
	.byte		        Bn4 
	.byte	W23
	.byte	W01
	.byte		N11   , As4 
	.byte	W11
	.byte	W01
	.byte		        Cs5 
	.byte	W11
	.byte	W01
	.byte		N44   , Fs4 
	.byte	W44
	.byte	W03
	.byte	W01
@ 076   ----------------------------------------
	.byte	W96
@ 077   ----------------------------------------
	.byte	W96
@ 078   ----------------------------------------
	.byte	W96
@ 079   ----------------------------------------
	.byte	W96
@ 080   ----------------------------------------
	.byte	W96
@ 081   ----------------------------------------
	.byte	W96
@ 082   ----------------------------------------
	.byte	W96
@ 083   ----------------------------------------
	.byte	W96
@ 084   ----------------------------------------
	.byte		N23   , Bn4 , v096
	.byte	W23
	.byte	W01
	.byte		        As4 
	.byte	W23
	.byte	W01
	.byte		        An4 
	.byte	W23
	.byte	W01
	.byte		        As4 
	.byte	W23
	.byte	W01
@ 085   ----------------------------------------
	.byte		        Bn4 
	.byte	W23
	.byte	W01
	.byte		        As4 
	.byte	W23
	.byte	W01
	.byte		        An4 
	.byte	W23
	.byte	W01
	.byte		        As4 
	.byte	W23
	.byte	W01
@ 086   ----------------------------------------
	.byte		        Bn4 
	.byte	W23
	.byte	W01
	.byte		        As4 
	.byte	W23
	.byte	W01
	.byte		        An4 
	.byte	W23
	.byte	W01
	.byte		        As4 
	.byte	W23
	.byte	W01
@ 087   ----------------------------------------
	.byte		        Bn4 
	.byte	W23
	.byte	W01
	.byte		        As4 
	.byte	W23
	.byte	W01
	.byte		        An4 
	.byte	W23
	.byte	W01
	.byte		        As4 
	.byte	W23
	.byte	W01
@ 088   ----------------------------------------
	.byte		        Dn5 
	.byte	W23
	.byte	W01
	.byte		        Cs5 
	.byte	W23
	.byte	W01
	.byte		        Bn4 
	.byte	W23
	.byte	W01
	.byte		        Cs5 
	.byte	W23
	.byte	W01
@ 089   ----------------------------------------
	.byte		        En5 
	.byte	W23
	.byte	W01
	.byte		        Dn5 
	.byte	W23
	.byte	W01
	.byte		        Cs5 
	.byte	W23
	.byte	W01
	.byte		        Dn5 
	.byte	W23
	.byte	W01
@ 090   ----------------------------------------
	.byte		        Cs5 
	.byte	W23
	.byte	W01
	.byte		        Bn4 
	.byte	W23
	.byte	W01
	.byte		        As4 
	.byte	W23
	.byte	W01
	.byte		        Cs5 
	.byte	W23
	.byte	W01
@ 091   ----------------------------------------
	.byte		        Fs5 
	.byte	W23
	.byte	W01
	.byte		        Fs4 
	.byte	W23
	.byte	W01
	.byte		N44   , Cs5 
	.byte	W44
	.byte	W03
	.byte	W01
	.byte	0xB2                 @ Direct Jump command
	.word   mus_bb_loop_point2
@**************** Track 3 (Midi-Chn.3) ****************@

mus_bb_channel_3:
	.byte	KEYSH , mus_bb_channel_key+0
@ 000   ----------------------------------------
	.byte		VOICE , 38
	.byte		VOL   , 105*mus_bb_channel_mvl/mxv
	.byte		PAN   , c_v+0
	.byte	W96
@ 001   ----------------------------------------
	.byte	W96
@ 002   ----------------------------------------
	.byte	W96
@ 003   ----------------------------------------
	.byte	W96
@ 004   ----------------------------------------
	.byte	W96
@ 005   ----------------------------------------
	.byte	W96
@ 006   ----------------------------------------
	.byte	W96
@ 007   ----------------------------------------
	.byte	W96
mus_bb_loop_point3:
@ 008   ----------------------------------------
mus_bb_channel_3_008:
	.byte		N11   , BnM1, v080
	.byte	W11
	.byte	PEND
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 009   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 010   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 011   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 012   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 013   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 014   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 015   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 016   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 017   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 018   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 019   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 020   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 021   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 022   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 023   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 024   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 025   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 026   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 027   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 028   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 029   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 030   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 031   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 032   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 033   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 034   ----------------------------------------
	.byte	W96
@ 035   ----------------------------------------
	.byte	W96
@ 036   ----------------------------------------
	.byte	W96
@ 037   ----------------------------------------
	.byte	W96
@ 038   ----------------------------------------
	.byte	W96
@ 039   ----------------------------------------
	.byte	W96
@ 040   ----------------------------------------
	.byte	W96
@ 041   ----------------------------------------
	.byte	W96
@ 042   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 043   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 044   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 045   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 046   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 047   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 048   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 049   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 050   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 051   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 052   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 053   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 054   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 055   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 056   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 057   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 058   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 059   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 060   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 061   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 062   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 063   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 064   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 065   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 066   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 067   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 068   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 069   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 070   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 071   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 072   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 073   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 074   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 075   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 076   ----------------------------------------
	.byte	W96
@ 077   ----------------------------------------
	.byte	W96
@ 078   ----------------------------------------
	.byte	W96
@ 079   ----------------------------------------
	.byte	W96
@ 080   ----------------------------------------
	.byte	W96
@ 081   ----------------------------------------
	.byte	W96
@ 082   ----------------------------------------
	.byte	W96
@ 083   ----------------------------------------
	.byte	W96
@ 084   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 085   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 086   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 087   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 088   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 089   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 090   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
@ 091   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_3_008
	.byte	W01
	.byte		N11   , Bn0 , v080
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte		        BnM1
	.byte	W11
	.byte	W01
	.byte		        Bn0 
	.byte	W11
	.byte	W01
	.byte	0xB2                 @ Direct Jump command
	.word   mus_bb_loop_point3

@**************** Track 4 (Midi-Chn.5) ****************@

mus_bb_channel_4:
	.byte	KEYSH , mus_bb_channel_key+0
@ 000   ----------------------------------------
	.byte		VOICE , 48
	.byte		VOL   , 100*mus_bb_channel_mvl/mxv
	.byte		PAN   , c_v+0
	.byte		N11   , Fs3 , v080
	.byte	W11
	.byte	W01
	.byte		        Bn3 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Bn3 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Cs4 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Cs4 
	.byte	W11
	.byte	W01
@ 001   ----------------------------------------
mus_bb_channel_4_001:
	.byte		N11   , Fs3 , v080
	.byte	W11
	.byte	PEND
	.byte	W01
	.byte		        Dn4 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Dn4 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Cs4 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Cs4 
	.byte	W11
	.byte	W01
@ 002   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_001
	.byte	W01
	.byte		N11   , Bn3 , v080
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Bn3 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Cs4 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Cs4 
	.byte	W11
	.byte	W01
@ 003   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_001
	.byte	W01
	.byte		N11   , Dn4 , v080
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Dn4 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
@ 004   ----------------------------------------
mus_bb_channel_4_004:
	.byte		N11   , Fs4 , v080
	.byte	W11
	.byte	PEND
	.byte	W01
	.byte		        Bn3 
	.byte	W11
	.byte	W01
	.byte		        Fs4 
	.byte	W11
	.byte	W01
	.byte		        Bn3 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
	.byte		        Bn3 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
	.byte		        Bn3 
	.byte	W11
	.byte	W01
@ 005   ----------------------------------------
mus_bb_channel_4_005:
	.byte		N11   , Dn4 , v080
	.byte	W11
	.byte	PEND
	.byte	W01
	.byte		        Gn3 
	.byte	W11
	.byte	W01
	.byte		        Dn4 
	.byte	W11
	.byte	W01
	.byte		        Gn3 
	.byte	W11
	.byte	W01
	.byte		        Cs4 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Cs4 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
@ 006   ----------------------------------------
mus_bb_channel_4_006:
	.byte		N11   , Bn3 , v080
	.byte	W11
	.byte	PEND
	.byte	W01
	.byte		        En3 
	.byte	W11
	.byte	W01
	.byte		        Bn3 
	.byte	W11
	.byte	W01
	.byte		        En3 
	.byte	W11
	.byte	W01
	.byte		        Cs4 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Cs4 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
@ 007   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_005
	.byte	W01
	.byte		N11   , Bn3 , v080
	.byte	W11
	.byte	W01
	.byte		        Dn4 
	.byte	W11
	.byte	W01
	.byte		        Bn3 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
	.byte		        Bn3 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
	.byte		        Bn3 
	.byte	W11
	.byte	W01
mus_bb_loop_point4:
	.byte	TEMPO , 160*mus_bb_channel_tbs/2
@ 008   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_001
	.byte	W01
	.byte		N11   , Bn3 , v080
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Bn3 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Cs4 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Cs4 
	.byte	W11
	.byte	W01
@ 009   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_001
	.byte	W01
	.byte		N11   , Dn4 , v080
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Dn4 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Cs4 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Cs4 
	.byte	W11
	.byte	W01
@ 010   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_001
	.byte	W01
	.byte		N11   , Bn3 , v080
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Bn3 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Cs4 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Cs4 
	.byte	W11
	.byte	W01
@ 011   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_001
	.byte	W01
	.byte		N11   , Dn4 , v080
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Dn4 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
@ 012   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_004
	.byte	W01
	.byte		N11   , Bn3 , v080
	.byte	W11
	.byte	W01
	.byte		        Fs4 
	.byte	W11
	.byte	W01
	.byte		        Bn3 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
	.byte		        Bn3 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
	.byte		        Bn3 
	.byte	W11
	.byte	W01
@ 013   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_005
	.byte	W01
	.byte		N11   , Gn3 , v080
	.byte	W11
	.byte	W01
	.byte		        Dn4 
	.byte	W11
	.byte	W01
	.byte		        Gn3 
	.byte	W11
	.byte	W01
	.byte		        Cs4 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Cs4 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
@ 014   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_006
	.byte	W01
	.byte		N11   , En3 , v080
	.byte	W11
	.byte	W01
	.byte		        Bn3 
	.byte	W11
	.byte	W01
	.byte		        En3 
	.byte	W11
	.byte	W01
	.byte		        Cs4 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Cs4 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
@ 015   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_005
	.byte	W01
	.byte		N11   , Bn3 , v080
	.byte	W11
	.byte	W01
	.byte		        Dn4 
	.byte	W11
	.byte	W01
	.byte		        Bn3 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
	.byte		        Bn3 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
	.byte		        Bn3 
	.byte	W11
	.byte	W01
@ 016   ----------------------------------------
	.byte		        Fs3 
	.byte		N92   , Bn4 
	.byte	W11
	.byte	W01
	.byte		N11   , Bn3 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Bn3 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Cs4 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Cs4 
	.byte	W11
	.byte	W01
@ 017   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_001
	.byte	W01
	.byte		N11   , Dn4 , v080
	.byte		N11   , Cs5 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Dn4 
	.byte		N11   , Cs5 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Cs4 
	.byte		N11   , Bn4 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte		N23   , Cs5 
	.byte	W11
	.byte	W01
	.byte		N11   , Cs4 
	.byte	W11
	.byte	W01
@ 018   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_001
	.byte	W01
	.byte		N11   , Bn3 , v080
	.byte		N11   , Dn5 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Bn3 
	.byte		N11   , Dn5 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Cs4 
	.byte		N11   , Cs5 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte		N23   , Dn5 
	.byte	W11
	.byte	W01
	.byte		N11   , Cs4 
	.byte	W11
	.byte	W01
@ 019   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_001
	.byte	W01
	.byte		N11   , Dn4 , v080
	.byte		N11   , En5 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Dn4 
	.byte		N11   , En5 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte		N44   , An4 
	.byte	W11
	.byte	W01
	.byte		N11   , En4 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
@ 020   ----------------------------------------
	.byte		        Fs4 
	.byte		N92   , Fs5 
	.byte	W11
	.byte	W01
	.byte		N11   , Bn3 
	.byte	W11
	.byte	W01
	.byte		        Fs4 
	.byte	W11
	.byte	W01
	.byte		        Bn3 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
	.byte		        Bn3 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
	.byte		        Bn3 
	.byte	W11
	.byte	W01
@ 021   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_005
	.byte	W01
	.byte		N11   , Gn3 , v080
	.byte		N11   , Gn5 
	.byte	W11
	.byte	W01
	.byte		        Dn4 
	.byte	W11
	.byte	W01
	.byte		        Gn3 
	.byte		N11   , Gn5 
	.byte	W11
	.byte	W01
	.byte		        Cs4 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte		N11   , Fs5 
	.byte	W11
	.byte	W01
	.byte		        Cs4 
	.byte		N23   , En5 
	.byte	W11
	.byte	W01
	.byte		N11   , Fs3 
	.byte	W11
	.byte	W01
@ 022   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_006
	.byte	W01
	.byte		N11   , En3 , v080
	.byte		N11   , Dn5 
	.byte	W11
	.byte	W01
	.byte		        Bn3 
	.byte	W11
	.byte	W01
	.byte		        En3 
	.byte		N11   , Dn5 
	.byte	W11
	.byte	W01
	.byte		        Cs4 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte		N11   , En5 
	.byte	W11
	.byte	W01
	.byte		        Cs4 
	.byte		N23   , Dn5 
	.byte	W11
	.byte	W01
	.byte		N11   , Fs3 
	.byte	W11
	.byte	W01
@ 023   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_005
	.byte	W01
	.byte		N11   , Bn3 , v080
	.byte		N11   , Cs5 
	.byte	W11
	.byte	W01
	.byte		        Dn4 
	.byte	W11
	.byte	W01
	.byte		        Bn3 
	.byte		N11   , Cs5 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte		N44   , As4 
	.byte	W11
	.byte	W01
	.byte		N11   , Bn3 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
	.byte		        Bn3 
	.byte	W11
	.byte	W01
@ 024   ----------------------------------------
	.byte		TIE   , Fs4 
	.byte		TIE   , Bn4 
	.byte	W96
@ 025   ----------------------------------------
	.byte	W92
	.byte	W03
	.byte		EOT   , Fs4 
	.byte		        Bn4 
	.byte	W01
@ 026   ----------------------------------------
mus_bb_channel_4_026:
	.byte		N11   , Fs4 , v096
	.byte	W11
	.byte	PEND
	.byte	W13
	.byte		N11   
	.byte	W11
	.byte	W13
	.byte		N44   , Bn2 , v080
	.byte		N11   , Fs4 , v096
	.byte	W11
	.byte	W13
	.byte		N11   
	.byte	W11
	.byte	W12
	.byte	W01
@ 027   ----------------------------------------
mus_bb_channel_4_027:
	.byte		N44   , Gn3 , v080
	.byte	W44
	.byte	W03
	.byte	PEND
	.byte	W01
	.byte		        Fs3 
	.byte		N11   , Fs4 , v096
	.byte	W11
	.byte	W13
	.byte		N11   
	.byte	W11
	.byte	W12
	.byte	W01
@ 028   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_026
	.byte	W13
	.byte		N11   , Fs4 , v096
	.byte	W11
	.byte	W13
	.byte		N44   , Bn2 , v080
	.byte		N11   , Fs4 , v096
	.byte	W11
	.byte	W13
	.byte		N11   
	.byte	W11
	.byte	W12
	.byte	W01
@ 029   ----------------------------------------
mus_bb_channel_4_029:
	.byte		N44   , En3 , v080
	.byte	W44
	.byte	W03
	.byte	PEND
	.byte	W01
	.byte		        Dn3 
	.byte		N11   , Fs4 , v096
	.byte	W11
	.byte	W13
	.byte		N11   
	.byte	W11
	.byte	W12
	.byte	W01
@ 030   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_026
	.byte	W13
	.byte		N11   , Fs4 , v096
	.byte	W11
	.byte	W13
	.byte		N44   , Bn2 , v080
	.byte		N11   , Fs4 , v096
	.byte	W11
	.byte	W13
	.byte		N11   
	.byte	W11
	.byte	W12
	.byte	W01
@ 031   ----------------------------------------
mus_bb_channel_4_031:
	.byte		N44   , An3 , v080
	.byte	W44
	.byte	W03
	.byte	PEND
	.byte	W01
	.byte		        Gn3 
	.byte		N11   , Fs4 , v096
	.byte	W11
	.byte	W13
	.byte		N11   
	.byte	W11
	.byte	W12
	.byte	W01
@ 032   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_026
	.byte	W13
	.byte		N11   , Fs4 , v096
	.byte	W11
	.byte	W13
	.byte		N44   , Bn2 , v080
	.byte		N11   , Fs4 , v096
	.byte	W11
	.byte	W13
	.byte		N11   
	.byte	W11
	.byte	W12
	.byte	W01
@ 033   ----------------------------------------
mus_bb_channel_4_033:
	.byte		N44   , Bn3 , v080
	.byte	W44
	.byte	W03
	.byte	PEND
	.byte	W01
	.byte		        Cs4 
	.byte		N11   , Fs4 , v096
	.byte	W11
	.byte	W13
	.byte		N11   
	.byte	W11
	.byte	W12
	.byte	W01
@ 034   ----------------------------------------
mus_bb_channel_4_034:
	.byte		N92   , Gn3 , v080
	.byte		N92   , Bn3 
	.byte	W92
	.byte	W03
	.byte	PEND
	.byte	W01
@ 035   ----------------------------------------
mus_bb_channel_4_035:
	.byte		N92   , Bn3 , v080
	.byte		N92   , En4 
	.byte	W92
	.byte	W03
	.byte	PEND
	.byte	W01
@ 036   ----------------------------------------
mus_bb_channel_4_036:
	.byte		N92   , An3 , v080
	.byte		N92   , Cs4 
	.byte	W92
	.byte	W03
	.byte	PEND
	.byte	W01
@ 037   ----------------------------------------
mus_bb_channel_4_037:
	.byte		N23   , Dn4 , v080
	.byte		N23   , Bn4 
	.byte	W23
	.byte	PEND
	.byte	W01
	.byte		        Cs4 
	.byte		N23   , An4 
	.byte	W23
	.byte	W01
	.byte		        Bn3 
	.byte		N23   , Gn4 
	.byte	W23
	.byte	W01
	.byte		        An3 
	.byte		N23   , Fs4 
	.byte	W23
	.byte	W01
@ 038   ----------------------------------------
mus_bb_channel_4_038:
	.byte		N92   , Bn3 , v080
	.byte		N92   , Gn4 
	.byte	W92
	.byte	W03
	.byte	PEND
	.byte	W01
@ 039   ----------------------------------------
mus_bb_channel_4_039:
	.byte		N92   , Gn3 , v080
	.byte		N92   , En4 
	.byte	W92
	.byte	W03
	.byte	PEND
	.byte	W01
@ 040   ----------------------------------------
mus_bb_channel_4_040:
	.byte		N44   , An3 , v080
	.byte		N44   , Fs4 
	.byte	W44
	.byte	W03
	.byte	PEND
	.byte	W01
	.byte		        Cs4 
	.byte		N44   , As4 
	.byte	W44
	.byte	W03
	.byte	W01
@ 041   ----------------------------------------
mus_bb_channel_4_041:
	.byte		N92   , En4 , v080
	.byte		N92   , Cs5 
	.byte	W92
	.byte	W03
	.byte	PEND
	.byte	W01
@ 042   ----------------------------------------
mus_bb_channel_4_042:
	.byte		N92   , Dn4 , v080
	.byte		N44   , Dn5 , v096
	.byte	W44
	.byte	W03
	.byte	PEND
	.byte	W01
	.byte		N11   , En5 
	.byte	W11
	.byte	W01
	.byte		        Fs5 
	.byte	W11
	.byte	W01
	.byte		N23   , Gn5 
	.byte	W23
	.byte	W01
@ 043   ----------------------------------------
mus_bb_channel_4_043:
	.byte		N92   , En4 , v080
	.byte		N23   , Fs5 , v096
	.byte	W23
	.byte	PEND
	.byte	W01
	.byte		        Fn5 
	.byte	W23
	.byte	W01
	.byte		        Fs5 
	.byte	W23
	.byte	W01
	.byte		        Dn5 
	.byte	W23
	.byte	W01
@ 044   ----------------------------------------
mus_bb_channel_4_044:
	.byte		N92   , Fs4 , v080
	.byte		N23   , En5 , v096
	.byte	W23
	.byte	PEND
	.byte	W01
	.byte		N07   , Dn5 
	.byte	W07
	.byte	W01
	.byte		        En5 
	.byte	W07
	.byte	W01
	.byte		        Dn5 
	.byte	W07
	.byte	W01
	.byte		N44   , Cs5 
	.byte	W44
	.byte	W03
	.byte	W01
@ 045   ----------------------------------------
mus_bb_channel_4_045:
	.byte		N44   , Gn4 , v080
	.byte	W44
	.byte	W03
	.byte	PEND
	.byte	W01
	.byte		        En4 
	.byte		N23   , As4 , v096
	.byte	W23
	.byte	W01
	.byte		N11   , As5 
	.byte	W11
	.byte	W12
	.byte	W01
@ 046   ----------------------------------------
mus_bb_channel_4_046:
	.byte		N92   , En4 , v080
	.byte		N11   , En5 , v096
	.byte	W11
	.byte	PEND
	.byte	W01
	.byte		        Fs5 
	.byte	W11
	.byte	W01
	.byte		N23   , Gn5 
	.byte	W23
	.byte	W01
	.byte		N11   , Cs5 
	.byte	W11
	.byte	W01
	.byte		        Dn5 
	.byte	W11
	.byte	W01
	.byte		N23   , En5 
	.byte	W23
	.byte	W01
@ 047   ----------------------------------------
mus_bb_channel_4_047:
	.byte		N92   , En4 , v080
	.byte		N11   , As4 , v096
	.byte	W11
	.byte	PEND
	.byte	W01
	.byte		        Bn4 
	.byte	W11
	.byte	W01
	.byte		N23   , Cs5 
	.byte	W23
	.byte	W01
	.byte		N11   , Fs4 
	.byte	W11
	.byte	W01
	.byte		        Gn4 
	.byte	W11
	.byte	W01
	.byte		        An4 
	.byte	W11
	.byte	W01
	.byte		        Bn4 
	.byte	W11
	.byte	W01
@ 048   ----------------------------------------
mus_bb_channel_4_048:
	.byte		N44   , An4 , v080
	.byte	W44
	.byte	W03
	.byte	PEND
	.byte	W01
	.byte		        Gn4 
	.byte		N23   , Cs5 , v096
	.byte	W23
	.byte	W01
	.byte		N11   , Fs4 
	.byte	W11
	.byte	W12
	.byte	W01
@ 049   ----------------------------------------
mus_bb_channel_4_049:
	.byte		N24   , Fs4 , v080
	.byte		N23   , Fs5 , v096
	.byte	W23
	.byte	PEND
	.byte	W01
	.byte		N68   , Fs4 
	.byte	W24
	.byte		N44   , Cs5 
	.byte	W44
	.byte	W03
	.byte	W01
@ 050   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_001
	.byte	W01
	.byte		N11   , Bn3 , v080
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Bn3 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Cs4 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Cs4 
	.byte	W11
	.byte	W01
@ 051   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_001
	.byte	W01
	.byte		N11   , Dn4 , v080
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Dn4 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Cs4 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Cs4 
	.byte	W11
	.byte	W01
@ 052   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_001
	.byte	W01
	.byte		N11   , Bn3 , v080
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Bn3 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Cs4 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Cs4 
	.byte	W11
	.byte	W01
@ 053   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_001
	.byte	W01
	.byte		N11   , Dn4 , v080
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Dn4 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
@ 054   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_004
	.byte	W01
	.byte		N11   , Bn3 , v080
	.byte	W11
	.byte	W01
	.byte		        Fs4 
	.byte	W11
	.byte	W01
	.byte		        Bn3 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
	.byte		        Bn3 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
	.byte		        Bn3 
	.byte	W11
	.byte	W01
@ 055   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_005
	.byte	W01
	.byte		N11   , Gn3 , v080
	.byte	W11
	.byte	W01
	.byte		        Dn4 
	.byte	W11
	.byte	W01
	.byte		        Gn3 
	.byte	W11
	.byte	W01
	.byte		        Cs4 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Cs4 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
@ 056   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_006
	.byte	W01
	.byte		N11   , En3 , v080
	.byte	W11
	.byte	W01
	.byte		        Bn3 
	.byte	W11
	.byte	W01
	.byte		        En3 
	.byte	W11
	.byte	W01
	.byte		        Cs4 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Cs4 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
@ 057   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_005
	.byte	W01
	.byte		N11   , Bn3 , v080
	.byte	W11
	.byte	W01
	.byte		        Dn4 
	.byte	W11
	.byte	W01
	.byte		        Bn3 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
	.byte		        Bn3 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
	.byte		        Bn3 
	.byte	W11
	.byte	W01
@ 058   ----------------------------------------
	.byte		        Fs3 
	.byte		N92   , Bn4 , v096
	.byte	W11
	.byte	W01
	.byte		N11   , Bn3 , v080
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Bn3 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Cs4 
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        Cs4 
	.byte	W11
	.byte	W01
@ 059   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_001
	.byte	W01
	.byte		N11   , Dn4 , v080
	.byte		N11   , Cs5 , v096
	.byte	W11
	.byte	W01
	.byte		        Fs3 , v080
	.byte	W11
	.byte	W01
	.byte		        Dn4 
	.byte		N11   , Cs5 , v096
	.byte	W11
	.byte	W01
	.byte		        Fs3 , v080
	.byte	W11
	.byte	W01
	.byte		        Cs4 
	.byte		N11   , Bn4 , v096
	.byte	W11
	.byte	W01
	.byte		        Fs3 , v080
	.byte		N23   , Cs5 , v096
	.byte	W11
	.byte	W01
	.byte		N11   , Cs4 , v080
	.byte	W11
	.byte	W01
@ 060   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_001
	.byte	W01
	.byte		N11   , Bn3 , v080
	.byte		N11   , Dn5 , v096
	.byte	W11
	.byte	W01
	.byte		        Fs3 , v080
	.byte	W11
	.byte	W01
	.byte		        Bn3 
	.byte		N11   , Dn5 , v096
	.byte	W11
	.byte	W01
	.byte		        Fs3 , v080
	.byte	W11
	.byte	W01
	.byte		        Cs4 
	.byte		N11   , Cs5 , v096
	.byte	W11
	.byte	W01
	.byte		        Fs3 , v080
	.byte		N23   , Dn5 , v096
	.byte	W11
	.byte	W01
	.byte		N11   , Cs4 , v080
	.byte	W11
	.byte	W01
@ 061   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_001
	.byte	W01
	.byte		N11   , Dn4 , v080
	.byte		N11   , En5 , v096
	.byte	W11
	.byte	W01
	.byte		        Fs3 , v080
	.byte	W11
	.byte	W01
	.byte		        Dn4 
	.byte		N11   , En5 , v096
	.byte	W11
	.byte	W01
	.byte		        Fs3 , v080
	.byte		N44   , An4 , v096
	.byte	W11
	.byte	W01
	.byte		N11   , En4 , v080
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
@ 062   ----------------------------------------
	.byte		        Fs4 
	.byte		N92   , Fs5 , v096
	.byte	W11
	.byte	W01
	.byte		N11   , Bn3 , v080
	.byte	W11
	.byte	W01
	.byte		        Fs4 
	.byte	W11
	.byte	W01
	.byte		        Bn3 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
	.byte		        Bn3 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
	.byte		        Bn3 
	.byte	W11
	.byte	W01
@ 063   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_005
	.byte	W01
	.byte		N11   , Gn3 , v080
	.byte		N11   , Gn5 , v096
	.byte	W11
	.byte	W01
	.byte		        Dn4 , v080
	.byte	W11
	.byte	W01
	.byte		        Gn3 
	.byte		N11   , Gn5 , v096
	.byte	W11
	.byte	W01
	.byte		        Cs4 , v080
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte		N11   , Fs5 , v096
	.byte	W11
	.byte	W01
	.byte		        Cs4 , v080
	.byte		N23   , En5 , v096
	.byte	W11
	.byte	W01
	.byte		N11   , Fs3 , v080
	.byte	W11
	.byte	W01
@ 064   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_006
	.byte	W01
	.byte		N11   , En3 , v080
	.byte		N11   , Dn5 , v096
	.byte	W11
	.byte	W01
	.byte		        Bn3 , v080
	.byte	W11
	.byte	W01
	.byte		        En3 
	.byte		N11   , Dn5 , v096
	.byte	W11
	.byte	W01
	.byte		        Cs4 , v080
	.byte	W11
	.byte	W01
	.byte		        Fs3 
	.byte		N11   , En5 , v096
	.byte	W11
	.byte	W01
	.byte		        Cs4 , v080
	.byte		N23   , Dn5 , v096
	.byte	W11
	.byte	W01
	.byte		N11   , Fs3 , v080
	.byte	W11
	.byte	W01
@ 065   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_005
	.byte	W01
	.byte		N11   , Bn3 , v080
	.byte		N11   , Cs5 , v096
	.byte	W11
	.byte	W01
	.byte		        Dn4 , v080
	.byte	W11
	.byte	W01
	.byte		        Bn3 
	.byte		N11   , Cs5 , v096
	.byte	W11
	.byte	W01
	.byte		        En4 , v080
	.byte		N44   , As4 , v096
	.byte	W11
	.byte	W01
	.byte		N11   , Bn3 , v080
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
	.byte		        Bn3 
	.byte	W11
	.byte	W01
@ 066   ----------------------------------------
	.byte		TIE   , Fs4 
	.byte		TIE   , Bn4 , v096
	.byte	W96
@ 067   ----------------------------------------
	.byte	W92
	.byte	W03
	.byte		EOT   , Fs4 
	.byte		        Bn4 
	.byte	W01
@ 068   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_026
	.byte	W13
	.byte		N11   , Fs4 , v096
	.byte	W11
	.byte	W13
	.byte		N44   , Bn2 , v080
	.byte		N11   , Fs4 , v096
	.byte	W11
	.byte	W13
	.byte		N11   
	.byte	W11
	.byte	W12
	.byte	W01
@ 069   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_027
	.byte	W01
	.byte		N44   , Fs3 , v080
	.byte		N11   , Fs4 , v096
	.byte	W11
	.byte	W13
	.byte		N11   
	.byte	W11
	.byte	W12
	.byte	W01
@ 070   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_026
	.byte	W13
	.byte		N11   , Fs4 , v096
	.byte	W11
	.byte	W13
	.byte		N44   , Bn2 , v080
	.byte		N11   , Fs4 , v096
	.byte	W11
	.byte	W13
	.byte		N11   
	.byte	W11
	.byte	W12
	.byte	W01
@ 071   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_029
	.byte	W01
	.byte		N44   , Dn3 , v080
	.byte		N11   , Fs4 , v096
	.byte	W11
	.byte	W13
	.byte		N11   
	.byte	W11
	.byte	W12
	.byte	W01
@ 072   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_026
	.byte	W13
	.byte		N11   , Fs4 , v096
	.byte	W11
	.byte	W13
	.byte		N44   , Bn2 , v080
	.byte		N11   , Fs4 , v096
	.byte	W11
	.byte	W13
	.byte		N11   
	.byte	W11
	.byte	W12
	.byte	W01
@ 073   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_031
	.byte	W01
	.byte		N44   , Gn3 , v080
	.byte		N11   , Fs4 , v096
	.byte	W11
	.byte	W13
	.byte		N11   
	.byte	W11
	.byte	W12
	.byte	W01
@ 074   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_026
	.byte	W13
	.byte		N11   , Fs4 , v096
	.byte	W11
	.byte	W13
	.byte		N44   , Bn2 , v080
	.byte		N11   , Fs4 , v096
	.byte	W11
	.byte	W13
	.byte		N11   
	.byte	W11
	.byte	W12
	.byte	W01
@ 075   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_033
	.byte	W01
	.byte		N44   , Cs4 , v080
	.byte		N11   , Fs4 , v096
	.byte	W11
	.byte	W13
	.byte		N11   
	.byte	W11
	.byte	W12
	.byte	W01
@ 076   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_034
	.byte	W01
@ 077   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_035
	.byte	W01
@ 078   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_036
	.byte	W01
@ 079   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_037
	.byte	W01
	.byte		N23   , Cs4 , v080
	.byte		N23   , An4 
	.byte	W23
	.byte	W01
	.byte		        Bn3 
	.byte		N23   , Gn4 
	.byte	W23
	.byte	W01
	.byte		        An3 
	.byte		N23   , Fs4 
	.byte	W23
	.byte	W01
@ 080   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_038
	.byte	W01
@ 081   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_039
	.byte	W01
@ 082   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_040
	.byte	W01
	.byte		N44   , Cs4 , v080
	.byte		N44   , As4 
	.byte	W44
	.byte	W03
	.byte	W01
@ 083   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_041
	.byte	W01
@ 084   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_042
	.byte	W01
	.byte		N11   , En5 , v096
	.byte	W11
	.byte	W01
	.byte		        Fs5 
	.byte	W11
	.byte	W01
	.byte		N23   , Gn5 
	.byte	W23
	.byte	W01
@ 085   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_043
	.byte	W01
	.byte		N23   , Fn5 , v096
	.byte	W23
	.byte	W01
	.byte		        Fs5 
	.byte	W23
	.byte	W01
	.byte		        Dn5 
	.byte	W23
	.byte	W01
@ 086   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_044
	.byte	W01
	.byte		N07   , Dn5 , v096
	.byte	W07
	.byte	W01
	.byte		        En5 
	.byte	W07
	.byte	W01
	.byte		        Dn5 
	.byte	W07
	.byte	W01
	.byte		N44   , Cs5 
	.byte	W44
	.byte	W03
	.byte	W01
@ 087   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_045
	.byte	W01
	.byte		N44   , En4 , v080
	.byte		N23   , As4 , v096
	.byte	W23
	.byte	W01
	.byte		N11   , As5 
	.byte	W11
	.byte	W12
	.byte	W01
@ 088   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_046
	.byte	W01
	.byte		N11   , Fs5 , v096
	.byte	W11
	.byte	W01
	.byte		N23   , Gn5 
	.byte	W23
	.byte	W01
	.byte		N11   , Cs5 
	.byte	W11
	.byte	W01
	.byte		        Dn5 
	.byte	W11
	.byte	W01
	.byte		N23   , En5 
	.byte	W23
	.byte	W01
@ 089   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_047
	.byte	W01
	.byte		N11   , Bn4 , v096
	.byte	W11
	.byte	W01
	.byte		N23   , Cs5 
	.byte	W23
	.byte	W01
	.byte		N11   , Fs4 
	.byte	W11
	.byte	W01
	.byte		        Gn4 
	.byte	W11
	.byte	W01
	.byte		        An4 
	.byte	W11
	.byte	W01
	.byte		        Bn4 
	.byte	W11
	.byte	W01
@ 090   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_048
	.byte	W01
	.byte		N44   , Gn4 , v080
	.byte		N23   , Cs5 , v096
	.byte	W23
	.byte	W01
	.byte		N11   , Fs4 
	.byte	W11
	.byte	W12
	.byte	W01
@ 091   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_4_049
	.byte	W01
	.byte		N68   , Fs4 , v096
	.byte	W24
	.byte		N44   , Cs5 
	.byte	W44
	.byte	W03
	.byte	W01
	.byte	0xB2                 @ Direct Jump command
	.word   mus_bb_loop_point4

@**************** Track 5 (Midi-Chn.2) ****************@

mus_bb_channel_5:
	.byte	KEYSH , mus_bb_channel_key+0
@ 000   ----------------------------------------
	.byte		VOICE , 14
	.byte		VOL   , 20*mus_bb_channel_mvl/mxv
	.byte		PAN   , c_v+0
	.byte		N92   , Bn3 , v080
	.byte	W92
	.byte	W03
	.byte	W01
@ 001   ----------------------------------------
	.byte	W96
@ 002   ----------------------------------------
mus_bb_channel_5_002:
	.byte		N92   , Bn3 , v080
	.byte	W92
	.byte	W03
	.byte	PEND
	.byte	W01
@ 003   ----------------------------------------
	.byte	W96
@ 004   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_5_002
	.byte	W01
@ 005   ----------------------------------------
	.byte	W96
@ 006   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_5_002
	.byte	W01
@ 007   ----------------------------------------
	.byte	W96
mus_bb_loop_point5:
	.byte	TEMPO , 160*mus_bb_channel_tbs/2
@ 008   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_5_002
	.byte	W01
@ 009   ----------------------------------------
	.byte	W96
@ 010   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_5_002
	.byte	W01
@ 011   ----------------------------------------
	.byte	W96
@ 012   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_5_002
	.byte	W01
@ 013   ----------------------------------------
	.byte	W96
@ 014   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_5_002
	.byte	W01
@ 015   ----------------------------------------
	.byte	W96
@ 016   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_5_002
	.byte	W01
@ 017   ----------------------------------------
	.byte	W96
@ 018   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_5_002
	.byte	W01
@ 019   ----------------------------------------
	.byte	W96
@ 020   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_5_002
	.byte	W01
@ 021   ----------------------------------------
	.byte	W96
@ 022   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_5_002
	.byte	W01
@ 023   ----------------------------------------
	.byte	W96
@ 024   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_5_002
	.byte	W01
@ 025   ----------------------------------------
	.byte	W96
@ 026   ----------------------------------------
	.byte	W96
@ 027   ----------------------------------------
	.byte	W96
@ 028   ----------------------------------------
	.byte	W96
@ 029   ----------------------------------------
	.byte	W96
@ 030   ----------------------------------------
	.byte	W96
@ 031   ----------------------------------------
	.byte	W96
@ 032   ----------------------------------------
	.byte	W96
@ 033   ----------------------------------------
	.byte	W96
@ 034   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_5_002
	.byte	W01
@ 035   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_5_002
	.byte	W01
@ 036   ----------------------------------------
mus_bb_channel_5_036:
	.byte		N92   , En4 , v080
	.byte	W92
	.byte	W03
	.byte	PEND
	.byte	W01
@ 037   ----------------------------------------
	.byte		N23   , Bn3 
	.byte	W23
	.byte	W01
	.byte		        An3 
	.byte	W23
	.byte	W01
	.byte		        Gn3 
	.byte	W23
	.byte	W01
	.byte		        Fs3 
	.byte	W23
	.byte	W01
@ 038   ----------------------------------------
mus_bb_channel_5_038:
	.byte		N92   , Gn3 , v080
	.byte	W92
	.byte	W03
	.byte	PEND
	.byte	W01
@ 039   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_5_038
	.byte	W01
@ 040   ----------------------------------------
mus_bb_channel_5_040:
	.byte		N44   , Fs3 , v080
	.byte	W44
	.byte	W03
	.byte	PEND
	.byte	W01
	.byte		N44   
	.byte	W44
	.byte	W03
	.byte	W01
@ 041   ----------------------------------------
mus_bb_channel_5_041:
	.byte		N92   , Fs3 , v080
	.byte	W92
	.byte	W03
	.byte	PEND
	.byte	W01
@ 042   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_5_002
	.byte	W01
@ 043   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_5_002
	.byte	W01
@ 044   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_5_002
	.byte	W01
@ 045   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_5_002
	.byte	W01
@ 046   ----------------------------------------
	.byte		N23   , Dn4 , v080
	.byte	W23
	.byte	W01
	.byte		        Cs4 
	.byte	W23
	.byte	W01
	.byte		        Bn3 
	.byte	W23
	.byte	W01
	.byte		        Cs4 
	.byte	W23
	.byte	W01
@ 047   ----------------------------------------
	.byte		        En4 
	.byte	W23
	.byte	W01
	.byte		        Dn4 
	.byte	W23
	.byte	W01
	.byte		        Cs4 
	.byte	W23
	.byte	W01
	.byte		        Dn4 
	.byte	W23
	.byte	W01
@ 048   ----------------------------------------
mus_bb_channel_5_048:
	.byte		N92   , Cs4 , v080
	.byte	W92
	.byte	W03
	.byte	PEND
	.byte	W01
@ 049   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_5_048
	.byte	W01
@ 050   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_5_002
	.byte	W01
@ 051   ----------------------------------------
	.byte	W96
@ 052   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_5_002
	.byte	W01
@ 053   ----------------------------------------
	.byte	W96
@ 054   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_5_002
	.byte	W01
@ 055   ----------------------------------------
	.byte	W96
@ 056   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_5_002
	.byte	W01
@ 057   ----------------------------------------
	.byte	W96
@ 058   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_5_002
	.byte	W01
@ 059   ----------------------------------------
	.byte	W96
@ 060   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_5_002
	.byte	W01
@ 061   ----------------------------------------
	.byte	W96
@ 062   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_5_002
	.byte	W01
@ 063   ----------------------------------------
	.byte	W96
@ 064   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_5_002
	.byte	W01
@ 065   ----------------------------------------
	.byte	W96
@ 066   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_5_002
	.byte	W01
@ 067   ----------------------------------------
	.byte	W96
@ 068   ----------------------------------------
	.byte	W96
@ 069   ----------------------------------------
	.byte	W96
@ 070   ----------------------------------------
	.byte	W96
@ 071   ----------------------------------------
	.byte	W96
@ 072   ----------------------------------------
	.byte	W96
@ 073   ----------------------------------------
	.byte	W96
@ 074   ----------------------------------------
	.byte	W96
@ 075   ----------------------------------------
	.byte	W96
@ 076   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_5_002
	.byte	W01
@ 077   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_5_002
	.byte	W01
@ 078   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_5_036
	.byte	W01
@ 079   ----------------------------------------
	.byte		N23   , Bn3 , v080
	.byte	W23
	.byte	W01
	.byte		        An3 
	.byte	W23
	.byte	W01
	.byte		        Gn3 
	.byte	W23
	.byte	W01
	.byte		        Fs3 
	.byte	W23
	.byte	W01
@ 080   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_5_038
	.byte	W01
@ 081   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_5_038
	.byte	W01
@ 082   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_5_040
	.byte	W01
	.byte		N44   , Fs3 , v080
	.byte	W44
	.byte	W03
	.byte	W01
@ 083   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_5_041
	.byte	W01
@ 084   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_5_002
	.byte	W01
@ 085   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_5_002
	.byte	W01
@ 086   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_5_002
	.byte	W01
@ 087   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_5_002
	.byte	W01
@ 088   ----------------------------------------
	.byte		N23   , Dn4 , v080
	.byte	W23
	.byte	W01
	.byte		        Cs4 
	.byte	W23
	.byte	W01
	.byte		        Bn3 
	.byte	W23
	.byte	W01
	.byte		        Cs4 
	.byte	W23
	.byte	W01
@ 089   ----------------------------------------
	.byte		        En4 
	.byte	W23
	.byte	W01
	.byte		        Dn4 
	.byte	W23
	.byte	W01
	.byte		        Cs4 
	.byte	W23
	.byte	W01
	.byte		        Dn4 
	.byte	W23
	.byte	W01
@ 090   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_5_048
	.byte	W01
@ 091   ----------------------------------------
	.byte	PATT
	 .word	mus_bb_channel_5_048
	.byte	W01
	.byte 	0xB2                 @ Direct Jump command
	.word mus_bb_loop_point5

@******************************************************@
	.align	2

mus_bb_channel_fixed:
	.byte	5	@ NumTrks
	.byte	0	@ NumBlks
	.byte	mus_bb_channel_pri	@ Priority
	.byte	mus_bb_channel_rev	@ Reverb.

	.word	mus_bb_channel_grp

	.word	mus_bb_channel_1
	.word	mus_bb_channel_2
	.word	mus_bb_channel_3
	.word	mus_bb_channel_4
	.word	mus_bb_channel_5

	.end
