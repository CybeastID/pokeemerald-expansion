	.include "MPlayDef.s"

	.equ	mus_bb_channelGO_grp, voicegroup_dummy
	.equ	mus_bb_channelGO_pri, 10
	.equ	mus_bb_channelGO_rev, reverb_set+50
	.equ	mus_bb_channelGO_mvl, 127
	.equ	mus_bb_channelGO_key, 0
	.equ	mus_bb_channelGO_tbs, 1
	.equ	mus_bb_channelGO_exg, 1
	.equ	mus_bb_channelGO_cmp, 1

	.section .rodata
	.global	mus_bb_channelGO
	.align	2

@**************** Track 1 (Midi-Chn.1) ****************@

mus_bb_channelGO_1:
	.byte	KEYSH , mus_bb_channelGO_key+0
@ 000   ----------------------------------------
	.byte	TEMPO , 45*mus_bb_channelGO_tbs/2
	.byte		VOICE , 17
	.byte		VOL   , 100*mus_bb_channelGO_mvl/mxv
	.byte		PAN   , c_v+0
	.byte		N11   , En3 , v080
	.byte	W11
	.byte	W01
	.byte		        An3 
	.byte	W11
	.byte	W01
	.byte		        Gs3 
	.byte	W11
	.byte	W01
	.byte		        An3 
	.byte	W11
	.byte	W01
	.byte		N01   , As3 
	.byte	W01
	.byte	W01
	.byte		N02   , Bn3 
	.byte	W02
	.byte		N07   , Cn4 
	.byte	W07
	.byte	W01
	.byte		N11   , Bn3 
	.byte	W11
	.byte	W01
	.byte		        An3 
	.byte	W11
	.byte	W01
	.byte		        Gs3 
	.byte	W11
	.byte	W01
@ 001   ----------------------------------------
	.byte		        An3 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
	.byte		        Ds4 
	.byte	W11
	.byte	W01
	.byte		        Fn4 
	.byte	W11
	.byte	W01
	.byte		N05   , En4 
	.byte	W05
	.byte	W01
	.byte		        Dn4 
	.byte	W05
	.byte	W01
	.byte		        Cn4 
	.byte	W05
	.byte	W01
	.byte		        Bn3 
	.byte	W05
	.byte	W01
	.byte		N11   , En3 
	.byte	W11
	.byte	W01
	.byte		        Gn3 
	.byte	W11
	.byte	W01
@ 002   ----------------------------------------
	.byte		        An3 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
	.byte		        Ds4 
	.byte	W11
	.byte	W01
	.byte		N01   , Gn4 
	.byte	W01
	.byte	W01
	.byte		N02   , Gs4 
	.byte	W02
	.byte		N07   , An4 
	.byte	W07
	.byte	W01
	.byte		N11   , Gn4 
	.byte	W11
	.byte	W01
	.byte		N05   , Fn4 
	.byte	W05
	.byte	W01
	.byte		        En4 
	.byte	W05
	.byte	W01
	.byte		N11   , Ds4 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
@ 003   ----------------------------------------
	.byte		        Fn4 
	.byte	W11
	.byte	W01
	.byte		N05   , En4 
	.byte	W05
	.byte	W01
	.byte		        Dn4 
	.byte	W05
	.byte	W01
	.byte		N11   , Cn4 
	.byte	W11
	.byte	W01
	.byte		N11   
	.byte	W11
	.byte	W01
	.byte		N17   
	.byte	W17
	.byte	W01
	.byte		N02   , Dn4 
	.byte	W02
	.byte	W01
	.byte		        Cn4 
	.byte	W02
	.byte	W01
	.byte		N22   , Bn3 
	.byte	W22
	.byte	W02
@ 004   ----------------------------------------
	.byte		        Cn4 
	.byte	W22
	.byte	W02
	.byte		N05   , Dn4 
	.byte	W05
	.byte	W01
	.byte		        En4 
	.byte	W05
	.byte	W01
	.byte		N11   , Fn4 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
	.byte		        Ds4 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
	.byte		        Cn4 
	.byte	W11
	.byte	W01
@ 005   ----------------------------------------
	.byte		        Dn4 
	.byte	W11
	.byte	W04
	.byte		N02   , Cn4 
	.byte	W02
	.byte	W01
	.byte		        Dn4 
	.byte	W02
	.byte	W01
	.byte		        Cn4 
	.byte	W02
	.byte	W01
	.byte		N22   , Bn3 
	.byte	W22
	.byte	W02
	.byte		N11   , An3 
	.byte	W11
	.byte	W01
	.byte		        An4 
	.byte	W11
	.byte	W01
	.byte		        Dn4 
	.byte	W11
	.byte	W01
	.byte		        Gs4 
	.byte	W11
	.byte	W01
@ 006   ----------------------------------------
	.byte		N05   , Dn4 
	.byte	W05
	.byte	W01
	.byte		        En4 
	.byte	W05
	.byte	W01
	.byte		        Fn4 
	.byte	W05
	.byte	W07
	.byte		        Bn3 
	.byte	W05
	.byte	W01
	.byte		        Cn4 
	.byte	W05
	.byte	W01
	.byte		        Dn4 
	.byte	W05
	.byte	W07
	.byte		        Gs3 
	.byte	W05
	.byte	W01
	.byte		        An3 
	.byte	W05
	.byte	W01
	.byte		        Bn3 
	.byte	W05
	.byte	W07
	.byte		        En3 
	.byte	W05
	.byte	W01
	.byte		        Fn3 
	.byte	W05
	.byte	W01
	.byte		        Gn3 
	.byte	W05
	.byte	W01
	.byte		        An3 
	.byte	W05
	.byte	W01
@ 007   ----------------------------------------
	.byte		N11   , Bn3 
	.byte	W11
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
	.byte		        En4 
	.byte	W11
	.byte	W01
	.byte		        Gs3 
	.byte	W11
	.byte	W01
	.byte		N22   , Bn3 
	.byte	W22
	.byte	W02
@ 008   ----------------------------------------
	.byte		N11   , En3 
	.byte	W11
	.byte	W01
	.byte		        An3 
	.byte	W11
	.byte	W01
	.byte		        Gs3 
	.byte	W11
	.byte	W01
	.byte		        An3 
	.byte	W11
	.byte	W01
	.byte		        Cn4 
	.byte	W11
	.byte	W01
	.byte		        Bn3 
	.byte	W11
	.byte	W01
	.byte		        An3 
	.byte	W11
	.byte	W01
	.byte		        Gs3 
	.byte	W11
	.byte	W01
@ 009   ----------------------------------------
	.byte		        An3 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
	.byte		        Ds4 
	.byte	W11
	.byte	W01
	.byte		        Fn4 
	.byte	W11
	.byte	W01
	.byte		N05   , En4 
	.byte	W05
	.byte	W01
	.byte		        Dn4 
	.byte	W05
	.byte	W01
	.byte		        Cn4 
	.byte	W05
	.byte	W01
	.byte		        Bn3 
	.byte	W05
	.byte	W01
	.byte		N11   , En3 
	.byte	W11
	.byte	W01
	.byte		        Gn3 
	.byte	W11
	.byte	W01
@ 010   ----------------------------------------
	.byte		        An3 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
	.byte		        Ds4 
	.byte	W11
	.byte	W01
	.byte		        An4 
	.byte	W11
	.byte	W01
	.byte		        Gn4 
	.byte	W11
	.byte	W01
	.byte		N05   , Fn4 
	.byte	W05
	.byte	W01
	.byte		        En4 
	.byte	W05
	.byte	W01
	.byte		N11   , Ds4 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
@ 011   ----------------------------------------
	.byte		        Fn4 
	.byte	W11
	.byte	W01
	.byte		N05   , En4 
	.byte	W05
	.byte	W01
	.byte		        Dn4 
	.byte	W05
	.byte	W01
	.byte		N11   , Cn4 
	.byte	W11
	.byte	W01
	.byte		N11   
	.byte	W11
	.byte	W01
	.byte		N17   
	.byte	W17
	.byte	W01
	.byte		N02   , Dn4 
	.byte	W02
	.byte	W01
	.byte		        Cn4 
	.byte	W02
	.byte	W01
	.byte		N22   , Bn3 
	.byte	W22
	.byte	W02
@ 012   ----------------------------------------
	.byte		        Cn4 
	.byte	W22
	.byte	W02
	.byte		N05   , Dn4 
	.byte	W05
	.byte	W01
	.byte		        En4 
	.byte	W05
	.byte	W01
	.byte		N11   , Fn4 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
	.byte		        Ds4 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
	.byte		        Cn4 
	.byte	W11
	.byte	W01
@ 013   ----------------------------------------
	.byte		        Dn4 
	.byte	W11
	.byte	W04
	.byte		N02   , Cn4 
	.byte	W02
	.byte	W01
	.byte		        Dn4 
	.byte	W02
	.byte	W01
	.byte		        Cn4 
	.byte	W02
	.byte	W01
	.byte		N22   , Bn3 
	.byte	W22
	.byte	W02
	.byte		N01   , An3 
	.byte	W12
	.byte		N11   , An4 
	.byte	W11
	.byte	W01
	.byte		        Dn4 
	.byte	W11
	.byte	W01
	.byte		        Gs4 
	.byte	W11
	.byte	W01
@ 014   ----------------------------------------
	.byte		N05   , Dn4 
	.byte	W05
	.byte	W01
	.byte		        En4 
	.byte	W05
	.byte	W01
	.byte		        Fn4 
	.byte	W05
	.byte	W07
	.byte		        Bn3 
	.byte	W05
	.byte	W01
	.byte		        Cn4 
	.byte	W05
	.byte	W01
	.byte		        Dn4 
	.byte	W05
	.byte	W07
	.byte		        Gs3 
	.byte	W05
	.byte	W01
	.byte		        An3 
	.byte	W05
	.byte	W01
	.byte		        Bn3 
	.byte	W05
	.byte	W07
	.byte		        En3 
	.byte	W05
	.byte	W01
	.byte		        Fn3 
	.byte	W05
	.byte	W01
	.byte		        Gn3 
	.byte	W05
	.byte	W01
	.byte		        An3 
	.byte	W05
	.byte	W01
@ 015   ----------------------------------------
	.byte		N11   , Bn3 
	.byte	W11
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
	.byte		        En4 
	.byte	W11
	.byte	W01
	.byte		        Gs3 
	.byte	W11
	.byte	W01
	.byte		N22   , Bn3 
	.byte	W22
	.byte	W02
@ 016   ----------------------------------------
mus_bb_GO_RETRY_1:
	.byte	W96
@ 017   ----------------------------------------
	.byte	W96
@ 018   ----------------------------------------
	.byte	W96
@ 019   ----------------------------------------
	.byte	W96
@ 020   ----------------------------------------
	.byte	W96
@ 021   ----------------------------------------
	.byte	W96
@ 022   ----------------------------------------
	.byte	W96
@ 023   ----------------------------------------
	.byte	W96
@ 024   ----------------------------------------
	.byte	W96
@ 025   ----------------------------------------
	.byte	W96
@ 026   ----------------------------------------
	.byte		        Cn4 
	.byte	W22
	.byte	W02
	.byte		N05   , Dn4 
	.byte	W05
	.byte	W01
	.byte		        En4 
	.byte	W05
	.byte	W01
	.byte		N11   , Fn4 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
	.byte		        Ds4 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
	.byte		        Cn4 
	.byte	W11
	.byte	W01
@ 027   ----------------------------------------
	.byte		        Dn4 
	.byte	W11
	.byte	W04
	.byte		N02   , Cn4 
	.byte	W02
	.byte	W01
	.byte		        Dn4 
	.byte	W02
	.byte	W01
	.byte		        Cn4 
	.byte	W02
	.byte	W01
	.byte		N22   , Bn3 
	.byte	W22
	.byte	W02
	.byte		N11   , An3 
	.byte	W11
	.byte	W01
	.byte		        An4 
	.byte	W11
	.byte	W01
	.byte		        Dn4 
	.byte	W11
	.byte	W01
	.byte		        Gs4 
	.byte	W11
	.byte	W01
@ 028   ----------------------------------------
	.byte		N05   , Dn4 
	.byte	W05
	.byte	W01
	.byte		        En4 
	.byte	W05
	.byte	W01
	.byte		        Fn4 
	.byte	W05
	.byte	W07
	.byte		        Bn3 
	.byte	W05
	.byte	W01
	.byte		        Cn4 
	.byte	W05
	.byte	W01
	.byte		        Dn4 
	.byte	W05
	.byte	W07
	.byte		        Gs3 
	.byte	W05
	.byte	W01
	.byte		        An3 
	.byte	W05
	.byte	W01
	.byte		        Bn3 
	.byte	W05
	.byte	W07
	.byte		        En3 
	.byte	W05
	.byte	W01
	.byte		        Fn3 
	.byte	W05
	.byte	W01
	.byte		        Gn3 
	.byte	W05
	.byte	W01
	.byte		        An3 
	.byte	W05
	.byte	W01
@ 029   ----------------------------------------
	.byte		N11   , Bn3 
	.byte	W11
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
	.byte		        En4 
	.byte	W11
	.byte	W01
	.byte		        Gs3 
	.byte	W11
	.byte	W01
	.byte		N22   , Bn3 
	.byte	W22
	.byte	W02
@ 030   ----------------------------------------
	.byte		N11   , En3 
	.byte	W11
	.byte	W01
	.byte		        An3 
	.byte	W11
	.byte	W01
	.byte		        Gs3 
	.byte	W11
	.byte	W01
	.byte		        An3 
	.byte	W11
	.byte	W01
	.byte		        Cn4 
	.byte	W11
	.byte	W01
	.byte		        Bn3 
	.byte	W11
	.byte	W01
	.byte		        An3 
	.byte	W11
	.byte	W01
	.byte		        Gs3 
	.byte	W11
	.byte	W01
@ 031   ----------------------------------------
	.byte		        An3 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
	.byte		        Ds4 
	.byte	W11
	.byte	W01
	.byte		        Fn4 
	.byte	W11
	.byte	W01
	.byte		N05   , En4 
	.byte	W05
	.byte	W01
	.byte		        Dn4 
	.byte	W05
	.byte	W01
	.byte		        Cn4 
	.byte	W05
	.byte	W01
	.byte		        Bn3 
	.byte	W05
	.byte	W01
	.byte		N11   , En3 
	.byte	W11
	.byte	W01
	.byte		        Gn3 
	.byte	W11
	.byte	W01
@ 032   ----------------------------------------
	.byte		        An3 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
	.byte		        Ds4 
	.byte	W11
	.byte	W01
	.byte		        An4 
	.byte	W11
	.byte	W01
	.byte		        Gn4 
	.byte	W11
	.byte	W01
	.byte		N05   , Fn4 
	.byte	W05
	.byte	W01
	.byte		        En4 
	.byte	W05
	.byte	W01
	.byte		N11   , Ds4 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
@ 033   ----------------------------------------
	.byte		        Fn4 
	.byte	W11
	.byte	W01
	.byte		N05   , En4 
	.byte	W05
	.byte	W01
	.byte		        Dn4 
	.byte	W05
	.byte	W01
	.byte		N11   , Cn4 
	.byte	W11
	.byte	W01
	.byte		N11   
	.byte	W11
	.byte	W01
	.byte		N17   
	.byte	W17
	.byte	W01
	.byte		N02   , Dn4 
	.byte	W02
	.byte	W01
	.byte		        Cn4 
	.byte	W02
	.byte	W01
	.byte		N22   , Bn3 
	.byte	W22
	.byte	W02
@ 034   ----------------------------------------
	.byte		        Cn4 
	.byte	W22
	.byte	W02
	.byte		N05   , Dn4 
	.byte	W05
	.byte	W01
	.byte		        En4 
	.byte	W05
	.byte	W01
	.byte		N11   , Fn4 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
	.byte		        Ds4 
	.byte	W11
	.byte	W01
	.byte		        En4 
	.byte	W11
	.byte	W01
	.byte		        Cn4 
	.byte	W11
	.byte	W01
@ 035   ----------------------------------------
	.byte		        Dn4 
	.byte	W11
	.byte	W04
	.byte		N02   , Cn4 
	.byte	W02
	.byte	W01
	.byte		        Dn4 
	.byte	W02
	.byte	W01
	.byte		        Cn4 
	.byte	W02
	.byte	W01
	.byte		N22   , Bn3 
	.byte	W22
	.byte	W02
	.byte		N01   , An3 
	.byte	W12
	.byte		N11   , An4 
	.byte	W11
	.byte	W01
	.byte		        Dn4 
	.byte	W11
	.byte	W01
	.byte		        Gs4 
	.byte	W11
	.byte	W01
@ 036   ----------------------------------------
	.byte		N05   , Dn4 
	.byte	W05
	.byte	W01
	.byte		        En4 
	.byte	W05
	.byte	W01
	.byte		        Fn4 
	.byte	W05
	.byte	W07
	.byte		        Bn3 
	.byte	W05
	.byte	W01
	.byte		        Cn4 
	.byte	W05
	.byte	W01
	.byte		        Dn4 
	.byte	W05
	.byte	W07
	.byte		        Gs3 
	.byte	W05
	.byte	W01
	.byte		        An3 
	.byte	W05
	.byte	W01
	.byte		        Bn3 
	.byte	W05
	.byte	W07
	.byte		        En3 
	.byte	W05
	.byte	W01
	.byte		        Fn3 
	.byte	W05
	.byte	W01
	.byte		        Gn3 
	.byte	W05
	.byte	W01
	.byte		        An3 
	.byte	W05
	.byte	W01
@ 037   ----------------------------------------
	.byte		N11   , Bn3 
	.byte	W11
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
	.byte		N22   , En4 
	.byte	W12
	.byte		N11   , En3 
	.byte	W10
	.byte	W01
	.byte	W07
	.byte		N05   , Fn4 
	.byte	W05
	.byte	W01
	.byte		        Gn4 
	.byte	W05
	.byte	W01
	.byte		        Gs4 
	.byte	W05
	.byte	W01
@ 038   ----------------------------------------
	.byte		N44   , An4 , v080, gtp1
	.byte	W48
	.byte	0xB2                 @ Direct Jump command
	.word	mus_bb_GO_RETRY_1

@**************** Track 2 (Midi-Chn.1) ****************@

mus_bb_channelGO_2:
	.byte		VOL   , 70*mus_bb_channelGO_mvl/mxv
	.byte	KEYSH , mus_bb_channelGO_key+0
	.byte	VOICE, 32
	.byte   PAN , c_v-15                       @ <--- Slight nudge to the left
@ 000   ----------------------------------------
	.byte	W48
	.byte		N22   , An2 , v080
	.byte	W22
	.byte	W02
	.byte		        Gs2 
	.byte	W22
	.byte	W02
@ 001   ----------------------------------------
	.byte		        An2 
	.byte	W22
	.byte	W72
	.byte	W02
@ 002   ----------------------------------------
	.byte	W48
	.byte		N22   
	.byte	W22
	.byte	W02
	.byte		        Gs2 
	.byte	W22
	.byte	W02
@ 003   ----------------------------------------
	.byte		        An2 
	.byte	W22
	.byte	W72
	.byte	W02
@ 004   ----------------------------------------
	.byte		N28   , Fn3 
	.byte	W28
	.byte	W02
	.byte		N05   , En3 
	.byte	W05
	.byte	W01
	.byte		        Ds3 
	.byte	W05
	.byte	W01
	.byte		        En3 
	.byte	W05
	.byte	W01
	.byte		N22   , Bn2 
	.byte	W22
	.byte	W02
	.byte		        Cn3 
	.byte	W22
	.byte	W02
@ 005   ----------------------------------------
	.byte		N28   , Dn3 
	.byte	W28
	.byte	W02
	.byte		N05   , Cn3 
	.byte	W05
	.byte	W01
	.byte		        Bn2 
	.byte	W05
	.byte	W01
	.byte		        As2 
	.byte	W05
	.byte	W01
	.byte		N11   , An2 
	.byte	W11
	.byte	W01
	.byte		        An3 
	.byte	W11
	.byte	W01
	.byte		        Dn3 
	.byte	W11
	.byte	W01
	.byte		        Gs3 
	.byte	W11
	.byte	W01
@ 006   ----------------------------------------
	.byte		N05   , Fn3 
	.byte	W05
	.byte	W01
	.byte		        En3 
	.byte	W05
	.byte	W01
	.byte		        Dn3 
	.byte	W05
	.byte	W07
	.byte		N05   
	.byte	W05
	.byte	W01
	.byte		        Cn3 
	.byte	W05
	.byte	W01
	.byte		        Bn2 
	.byte	W05
	.byte	W07
	.byte		        Gs2 
	.byte	W05
	.byte	W01
	.byte		        An2 
	.byte	W05
	.byte	W01
	.byte		        Bn2 
	.byte	W05
	.byte	W07
	.byte		        En2 
	.byte	W05
	.byte	W01
	.byte		        Fn2 
	.byte	W05
	.byte	W01
	.byte		        Gn2 
	.byte	W05
	.byte	W01
	.byte		        An2 
	.byte	W05
	.byte	W01
@ 007   ----------------------------------------
	.byte		N11   , Bn2 
	.byte	W11
	.byte	W01
	.byte		        En2 
	.byte	W11
	.byte	W01
	.byte		        Bn2 
	.byte	W11
	.byte	W01
	.byte		        En2 
	.byte	W11
	.byte	W48
	.byte	W01
@ 008   ----------------------------------------
	.byte		N22   , Cn3 
	.byte	W22
	.byte	W02
	.byte		        Bn2 
	.byte	W22
	.byte	W02
	.byte		N11   , An2 
	.byte	W11
	.byte	W01
	.byte		N05   , En2 
	.byte	W05
	.byte	W01
	.byte		        Fn2 
	.byte	W05
	.byte	W01
	.byte		N11   , Gs2 
	.byte	W11
	.byte	W01
	.byte		        Bn2 
	.byte	W11
	.byte	W01
@ 009   ----------------------------------------
	.byte		N05   , An2 
	.byte	W05
	.byte	W01
	.byte		        Bn2 
	.byte	W05
	.byte	W01
	.byte		N11   , Cn3 
	.byte	W11
	.byte	W01
	.byte		        Dn3 
	.byte	W11
	.byte	W01
	.byte		        En3 
	.byte	W11
	.byte	W01
	.byte		        Fn3 
	.byte	W11
	.byte	W01
	.byte		        En3 
	.byte	W11
	.byte	W01
	.byte		        Cn3 
	.byte	W11
	.byte	W01
	.byte		        Bn2 
	.byte	W11
	.byte	W01
@ 010   ----------------------------------------
	.byte		        An2 
	.byte	W11
	.byte	W01
	.byte		        En3 
	.byte	W11
	.byte	W01
	.byte		        Ds3 
	.byte	W11
	.byte	W01
	.byte		        En3 
	.byte	W11
	.byte	W01
	.byte		        Cn3 
	.byte	W11
	.byte	W01
	.byte		        Dn3 
	.byte	W11
	.byte	W01
	.byte		        Bn2 
	.byte	W11
	.byte	W01
	.byte		        En2 
	.byte	W11
	.byte	W01
@ 011   ----------------------------------------
	.byte		N05   , Fn2 
	.byte	W05
	.byte	W01
	.byte		        Gn2 
	.byte	W05
	.byte	W01
	.byte		        An2 
	.byte	W05
	.byte	W01
	.byte		        Bn2 
	.byte	W05
	.byte	W01
	.byte		N11   , Cn3 
	.byte	W11
	.byte	W01
	.byte		        Bn2 
	.byte	W11
	.byte	W01
	.byte		        An2 
	.byte	W11
	.byte	W01
	.byte		N05   , Gs2 
	.byte	W05
	.byte	W01
	.byte		        Bn2 
	.byte	W05
	.byte	W01
	.byte		N22   , En2 
	.byte	W22
	.byte	W02
@ 012   ----------------------------------------
	.byte		        Fn2 
	.byte	W22
	.byte	W02
	.byte		N05   
	.byte	W05
	.byte	W01
	.byte		        Gn2 
	.byte	W05
	.byte	W01
	.byte		N11   , Gs2 
	.byte	W11
	.byte	W01
	.byte		N22   , An2 
	.byte	W22
	.byte	W02
	.byte		N05   
	.byte	W05
	.byte	W01
	.byte		        Bn2 
	.byte	W05
	.byte	W01
	.byte		N11   , Cn3 
	.byte	W11
	.byte	W01
@ 013   ----------------------------------------
	.byte		N22   , Dn3 
	.byte	W22
	.byte	W02
	.byte		N05   
	.byte	W05
	.byte	W01
	.byte		        En3 
	.byte	W05
	.byte	W01
	.byte		        Fn3 
	.byte	W05
	.byte	W01
	.byte		        Gn3 
	.byte	W05
	.byte	W01
	.byte		N11   , An3 
	.byte	W11
	.byte	W01
	.byte		        Gn3 
	.byte	W11
	.byte	W01
	.byte		        Fn3 
	.byte	W11
	.byte	W01
	.byte		        En3 
	.byte	W11
	.byte	W01
@ 014   ----------------------------------------
	.byte		N05   , Fn3 
	.byte	W05
	.byte	W01
	.byte		        En3 
	.byte	W05
	.byte	W01
	.byte		        Dn3 
	.byte	W05
	.byte	W07
	.byte		N05   
	.byte	W05
	.byte	W01
	.byte		        Cn3 
	.byte	W05
	.byte	W01
	.byte		        Bn2 
	.byte	W05
	.byte	W07
	.byte		        Gs2 
	.byte	W05
	.byte	W01
	.byte		        An2 
	.byte	W05
	.byte	W01
	.byte		        Bn2 
	.byte	W05
	.byte	W07
	.byte		        En2 
	.byte	W05
	.byte	W01
	.byte		        Fn2 
	.byte	W05
	.byte	W01
	.byte		        Gn2 
	.byte	W05
	.byte	W01
	.byte		        An2 
	.byte	W05
	.byte	W01
@ 015   ----------------------------------------
	.byte		N11   , Bn2 
	.byte	W11
	.byte	W01
	.byte		        En2 
	.byte	W11
	.byte	W01
	.byte		        Bn2 
	.byte	W11
	.byte	W01
	.byte		        En2 
	.byte	W11
	.byte	W01
	.byte		N22   , Bn2 
	.byte	W22
	.byte	W02
	.byte		N05   , En2 
	.byte	W05
	.byte	W01
	.byte		        Fn2 
	.byte	W05
	.byte	W01
	.byte		        Gn2 
	.byte	W05
	.byte	W01
	.byte		        Gs2 
	.byte	W05
	.byte	W01
@ 016   ----------------------------------------
mus_bb_GO_RETRY_2:
	.byte		N11   , An2 
	.byte	W11
	.byte	W84
	.byte	W01
@ 017   ----------------------------------------
	.byte	W96
@ 018   ----------------------------------------
	.byte		N22   , Cn3 
	.byte	W22
	.byte	W02
	.byte		        Bn2 
	.byte	W22
	.byte	W02
	.byte		N11   , An2 
	.byte	W11
	.byte	W01
	.byte		N05   , En2 
	.byte	W05
	.byte	W01
	.byte		        Fn2 
	.byte	W05
	.byte	W01
	.byte		N11   , Gs2 
	.byte	W11
	.byte	W01
	.byte		        Bn2 
	.byte	W11
	.byte	W01
@ 019   ----------------------------------------
	.byte		N05   , An2 
	.byte	W05
	.byte	W01
	.byte		        Bn2 
	.byte	W05
	.byte	W01
	.byte		N11   , Cn3 
	.byte	W11
	.byte	W01
	.byte		        Dn3 
	.byte	W11
	.byte	W01
	.byte		        En3 
	.byte	W11
	.byte	W01
	.byte		        Fn3 
	.byte	W11
	.byte	W01
	.byte		        En3 
	.byte	W11
	.byte	W01
	.byte		        Cn3 
	.byte	W11
	.byte	W01
	.byte		        Bn2 
	.byte	W11
	.byte	W01
@ 020   ----------------------------------------
	.byte		        An2 
	.byte	W11
	.byte	W01
	.byte		        En3 
	.byte	W11
	.byte	W01
	.byte		        Ds3 
	.byte	W11
	.byte	W01
	.byte		        En3 
	.byte	W11
	.byte	W01
	.byte		        Cn3 
	.byte	W11
	.byte	W01
	.byte		        Dn3 
	.byte	W11
	.byte	W01
	.byte		        Bn2 
	.byte	W11
	.byte	W01
	.byte		        En2 
	.byte	W11
	.byte	W01
@ 021   ----------------------------------------
	.byte		N05   , Fn2 
	.byte	W05
	.byte	W01
	.byte		        Gn2 
	.byte	W05
	.byte	W01
	.byte		        An2 
	.byte	W05
	.byte	W01
	.byte		        Bn2 
	.byte	W05
	.byte	W01
	.byte		N11   , Cn3 
	.byte	W11
	.byte	W01
	.byte		        Bn2 
	.byte	W11
	.byte	W01
	.byte		        An2 
	.byte	W11
	.byte	W01
	.byte		N05   , Gs2 
	.byte	W05
	.byte	W01
	.byte		        Bn2 
	.byte	W05
	.byte	W01
	.byte		N22   , En2 
	.byte	W22
	.byte	W02
@ 022   ----------------------------------------
	.byte		        Fn2 
	.byte	W22
	.byte	W02
	.byte		N05   
	.byte	W05
	.byte	W01
	.byte		        Gn2 
	.byte	W05
	.byte	W01
	.byte		N11   , Gs2 
	.byte	W11
	.byte	W01
	.byte		N22   , An2 
	.byte	W22
	.byte	W02
	.byte		N05   
	.byte	W05
	.byte	W01
	.byte		        Bn2 
	.byte	W05
	.byte	W01
	.byte		N11   , Cn3 
	.byte	W11
	.byte	W01
@ 023   ----------------------------------------
	.byte		N22   , Dn3 
	.byte	W22
	.byte	W02
	.byte		N05   
	.byte	W05
	.byte	W01
	.byte		        En3 
	.byte	W05
	.byte	W01
	.byte		        Fn3 
	.byte	W05
	.byte	W01
	.byte		        Gn3 
	.byte	W05
	.byte	W01
	.byte		N11   , An3 
	.byte	W11
	.byte	W01
	.byte		        Gn3 
	.byte	W11
	.byte	W01
	.byte		        Fn3 
	.byte	W11
	.byte	W01
	.byte		        En3 
	.byte	W11
	.byte	W01
@ 024   ----------------------------------------
	.byte		N22   , Dn3 
	.byte	W22
	.byte	W02
	.byte		N05   
	.byte	W05
	.byte	W01
	.byte		        Cn3 
	.byte	W05
	.byte	W01
	.byte		        Bn2 
	.byte	W05
	.byte	W01
	.byte		        As2 
	.byte	W05
	.byte	W01
	.byte		N22   , Bn2 
	.byte	W22
	.byte	W02
	.byte		N05   
	.byte	W05
	.byte	W01
	.byte		        Cn3 
	.byte	W05
	.byte	W01
	.byte		        Dn3 
	.byte	W05
	.byte	W01
	.byte		        Ds3 
	.byte	W05
	.byte	W01
@ 025   ----------------------------------------
	.byte		N11   , En3 
	.byte	W11
	.byte	W01
	.byte		        En2 
	.byte	W11
	.byte	W01
	.byte		        En3 
	.byte	W11
	.byte	W01
	.byte		        En2 
	.byte	W11
	.byte	W01
	.byte		        En3 
	.byte	W11
	.byte	W01
	.byte		        En2 
	.byte	W11
	.byte	W01
	.byte		N22   , En3 
	.byte	W22
	.byte	W02
@ 026   ----------------------------------------
	.byte	W96
@ 027   ----------------------------------------
	.byte	W96
@ 028   ----------------------------------------
	.byte	W96
@ 029   ----------------------------------------
	.byte	W96
@ 030   ----------------------------------------
	.byte		        Cn3 
	.byte	W22
	.byte	W02
	.byte		        Bn2 
	.byte	W22
	.byte	W02
	.byte		N11   , An2 
	.byte	W11
	.byte	W01
	.byte		N05   , En2 
	.byte	W05
	.byte	W01
	.byte		        Fn2 
	.byte	W05
	.byte	W01
	.byte		N11   , Gs2 
	.byte	W11
	.byte	W01
	.byte		        Bn2 
	.byte	W11
	.byte	W01
@ 031   ----------------------------------------
	.byte		N05   , An2 
	.byte	W05
	.byte	W01
	.byte		        Bn2 
	.byte	W05
	.byte	W01
	.byte		N11   , Cn3 
	.byte	W11
	.byte	W01
	.byte		        Dn3 
	.byte	W11
	.byte	W01
	.byte		        En3 
	.byte	W11
	.byte	W01
	.byte		        Fn3 
	.byte	W11
	.byte	W01
	.byte		        En3 
	.byte	W11
	.byte	W01
	.byte		        Cn3 
	.byte	W11
	.byte	W01
	.byte		        Bn2 
	.byte	W11
	.byte	W01
@ 032   ----------------------------------------
	.byte		        An2 
	.byte	W11
	.byte	W01
	.byte		        En3 
	.byte	W11
	.byte	W01
	.byte		        Ds3 
	.byte	W11
	.byte	W01
	.byte		        En3 
	.byte	W11
	.byte	W01
	.byte		        Cn3 
	.byte	W11
	.byte	W01
	.byte		        Dn3 
	.byte	W11
	.byte	W01
	.byte		        Bn2 
	.byte	W11
	.byte	W01
	.byte		        En2 
	.byte	W11
	.byte	W01
@ 033   ----------------------------------------
	.byte		N05   , Fn2 
	.byte	W05
	.byte	W01
	.byte		        Gn2 
	.byte	W05
	.byte	W01
	.byte		        An2 
	.byte	W05
	.byte	W01
	.byte		        Bn2 
	.byte	W05
	.byte	W01
	.byte		N11   , Cn3 
	.byte	W11
	.byte	W01
	.byte		        Bn2 
	.byte	W11
	.byte	W01
	.byte		        An2 
	.byte	W11
	.byte	W01
	.byte		N05   , Gs2 
	.byte	W05
	.byte	W01
	.byte		        Bn2 
	.byte	W05
	.byte	W01
	.byte		N22   , En2 
	.byte	W22
	.byte	W02
@ 034   ----------------------------------------
	.byte		        Fn2 
	.byte	W22
	.byte	W02
	.byte		N05   
	.byte	W05
	.byte	W01
	.byte		        Gn2 
	.byte	W05
	.byte	W01
	.byte		N11   , Gs2 
	.byte	W11
	.byte	W01
	.byte		N22   , An2 
	.byte	W22
	.byte	W02
	.byte		N05   
	.byte	W05
	.byte	W01
	.byte		        Bn2 
	.byte	W05
	.byte	W01
	.byte		N11   , Cn3 
	.byte	W11
	.byte	W01
@ 035   ----------------------------------------
	.byte		N22   , Dn3 
	.byte	W22
	.byte	W02
	.byte		N05   
	.byte	W05
	.byte	W01
	.byte		        En3 
	.byte	W05
	.byte	W01
	.byte		        Fn3 
	.byte	W05
	.byte	W01
	.byte		        Gn3 
	.byte	W05
	.byte	W01
	.byte		N11   , An3 
	.byte	W11
	.byte	W01
	.byte		        Gn3 
	.byte	W11
	.byte	W01
	.byte		        Fn3 
	.byte	W11
	.byte	W01
	.byte		        En3 
	.byte	W11
	.byte	W01
@ 036   ----------------------------------------
	.byte		N05   , Fn3 
	.byte	W05
	.byte	W01
	.byte		        En3 
	.byte	W05
	.byte	W01
	.byte		        Dn3 
	.byte	W05
	.byte	W07
	.byte		N05   
	.byte	W05
	.byte	W01
	.byte		        Cn3 
	.byte	W05
	.byte	W01
	.byte		        Bn2 
	.byte	W05
	.byte	W07
	.byte		        Gs2 
	.byte	W05
	.byte	W01
	.byte		        An2 
	.byte	W05
	.byte	W01
	.byte		        Bn2 
	.byte	W05
	.byte	W07
	.byte		        En2 
	.byte	W05
	.byte	W01
	.byte		        Fn2 
	.byte	W05
	.byte	W01
	.byte		        Gn2 
	.byte	W05
	.byte	W01
	.byte		        An2 
	.byte	W05
	.byte	W01
@ 037   ----------------------------------------
	.byte		N11   , Bn2 
	.byte	W11
	.byte	W01
	.byte		        En2 
	.byte	W11
	.byte	W01
	.byte		        Bn2 
	.byte	W11
	.byte	W01
	.byte		        En2 
	.byte	W11
	.byte	W01
	.byte		N22   , Bn2 
	.byte	W22
	.byte	W02
	.byte		N05   , En2 
	.byte	W05
	.byte	W01
	.byte		        Fn2 
	.byte	W05
	.byte	W01
	.byte		        Gn2 
	.byte	W05
	.byte	W01
	.byte		        Gs2 
	.byte	W05
	.byte	W01
@ 038   ----------------------------------------
	.byte		N44   , An2 , v080
	.byte	W48
	.byte	0xB2                 @ Direct Jump command
	.word	mus_bb_GO_RETRY_2

@******************************************************@
	.align	2

mus_bb_channelGO:
	.byte	2	@ NumTrks
	.byte	0	@ NumBlks
	.byte	mus_bb_channelGO_pri	@ Priority
	.byte	mus_bb_channelGO_rev	@ Reverb.

	.word	mus_bb_channelGO_grp

	.word	mus_bb_channelGO_1
	.word	mus_bb_channelGO_2

	.end
