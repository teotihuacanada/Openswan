/*-
 * The linux port of this code done by David McCullough
 * Copyright (C) 2004-2005 David McCullough <dmccullough@cyberguard.com>
 * The license and original author are listed below.
 *
 * Copyright (c) 2003 Sam Leffler, Errno Consulting
 * Copyright (c) 2003 Global Technology Associates, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/dev/safe/safevar.h,v 1.1 2003/07/21 21:46:07 sam Exp $
 */
#ifndef _SAFE_SAFEVAR_H_
#define	_SAFE_SAFEVAR_H_

/* Maximum queue length */
#ifndef SAFE_MAX_NQUEUE
#define SAFE_MAX_NQUEUE	60
#endif

#define	SAFE_MAX_PART		64	/* Maximum scatter/gather depth */
#define	SAFE_DMA_BOUNDARY	0	/* No boundary for source DMA ops */
#define	SAFE_MAX_DSIZE		2048 /* MCLBYTES Fixed scatter particle size */
#define	SAFE_MAX_SSIZE		0x0ffff	/* Maximum gather particle size */
#define	SAFE_MAX_DMA		0xfffff	/* Maximum PE operand size (20 bits) */
/* total src+dst particle descriptors */
#define	SAFE_TOTAL_DPART	(SAFE_MAX_NQUEUE * SAFE_MAX_PART)
#define	SAFE_TOTAL_SPART	(SAFE_MAX_NQUEUE * SAFE_MAX_PART)

#define	SAFE_RNG_MAXBUFSIZ	128	/* 32-bit words */

#define	SAFE_CARD(sid)		(((sid) & 0xf0000000) >> 28)
#define	SAFE_SESSION(sid)	( (sid) & 0x0fffffff)
#define	SAFE_SID(crd, sesn)	(((crd) << 28) | ((sesn) & 0x0fffffff))

#define SAFE_DEF_RTY		0xff	/* PCI Retry Timeout */
#define SAFE_DEF_TOUT		0xff	/* PCI TRDY Timeout */
#define SAFE_DEF_CACHELINE	0x01	/* Cache Line setting */

#ifdef __KERNEL__
/*
 * Cryptographic operand state.  One of these exists for each
 * source and destination operand passed in from the crypto
 * subsystem.  When possible source and destination operands
 * refer to the same memory.  More often they are distinct.
 * We track the virtual address of each operand as well as
 * where each is mapped for DMA.
 */
struct safe_operand {
	union {
		struct sk_buff *skb;
		struct uio *io;
	} u;
	void			*map;
	int				mapsize;	/* total number of bytes in segs */
	struct {
		dma_addr_t	ds_addr;
		int			ds_len;
		int			ds_tlen;
	} segs[SAFE_MAX_PART];
	int				nsegs;
};

/*
 * Packet engine ring entry and cryptographic operation state.
 * The packet engine requires a ring of descriptors that contain
 * pointers to various cryptographic state.  However the ring
 * configuration register allows you to specify an arbitrary size
 * for ring entries.  We use this feature to collect most of the
 * state for each cryptographic request into one spot.  Other than
 * ring entries only the ``particle descriptors'' (scatter/gather
 * lists) and the actual operand data are kept separate.  The
 * particle descriptors must also be organized in rings.  The
 * operand data can be located aribtrarily (modulo alignment constraints).
 *
 * Note that the descriptor ring is mapped onto the PCI bus so
 * the hardware can DMA data.  This means the entire ring must be
 * contiguous.
 */
struct safe_ringentry {
	struct safe_desc	re_desc;	/* command descriptor */
	struct safe_sarec	re_sa;		/* SA record */
	struct safe_sastate	re_sastate;	/* SA state record */

	struct cryptop		*re_crp;	/* crypto operation */

	struct safe_operand	re_src;		/* source operand */
	struct safe_operand	re_dst;		/* destination operand */

	int			re_sesn;	/* crypto session ID */
	int			re_flags;
#define	SAFE_QFLAGS_COPYOUTIV	0x1		/* copy back on completion */
#define	SAFE_QFLAGS_COPYOUTICV	0x2		/* copy back on completion */
};

#define	re_src_skb	re_src.u.skb
#define	re_src_io	re_src.u.io
#define	re_src_map	re_src.map
#define	re_src_nsegs	re_src.nsegs
#define	re_src_segs	re_src.segs
#define	re_src_mapsize	re_src.mapsize

#define	re_dst_skb	re_dst.u.skb
#define	re_dst_io	re_dst.u.io
#define	re_dst_map	re_dst.map
#define	re_dst_nsegs	re_dst.nsegs
#define	re_dst_segs	re_dst.segs
#define	re_dst_mapsize	re_dst.mapsize

struct rndstate_test;

struct safe_session {
	u_int32_t	ses_used;
	u_int32_t	ses_klen;		/* key length in bits */
	u_int32_t	ses_key[8];		/* DES/3DES/AES key */
	u_int32_t	ses_hminner[5];		/* hmac inner state */
	u_int32_t	ses_hmouter[5];		/* hmac outer state */
	u_int32_t	ses_iv[4];		/* DES/3DES/AES iv */
};

struct safe_pkq {
	struct list_head		 pkq_list;
	struct cryptkop			*pkq_krp;
};

struct safe_softc {
	int						 sc_num;		/* if we have multiple chips */

	struct pci_dev			*sc_dev;

	ocf_iomem_t				 sc_base_addr;
	u32						 sc_irq;

	u32						 sc_cid;		/* crypto tag */

	u32						 sc_chiprev;	/* major/minor chip revision */

	dma_addr_t				 sc_ring_dma;
	void					*sc_ring_vma;
	spinlock_t				 sc_ringmtx;	/* PE ring lock */

	struct safe_ringentry	*sc_ring;		/* PE ring */
	struct safe_ringentry	*sc_ringtop;	/* PE ring top */
	struct safe_ringentry	*sc_front;		/* next free entry */
	struct safe_ringentry	*sc_back;		/* next pending entry */

	dma_addr_t				 sc_sp_dma;
	void					*sc_sp_vma;

	struct safe_pdesc		*sc_spring;		/* src particle ring */
	struct safe_pdesc		*sc_springtop;	/* src particle ring top */
	struct safe_pdesc		*sc_spfree;		/* next free src particle */

	dma_addr_t				 sc_dp_dma;
	void					*sc_dp_vma;

	struct safe_pdesc		*sc_dpring;		/* dest particle ring */
	struct safe_pdesc		*sc_dpringtop;	/* dest particle ring top */
	struct safe_pdesc		*sc_dpfree;		/* next free dest particle */


	int						 sc_flags;		/* device specific flags */
#define	SAFE_FLAGS_KEY		 0x01			/* has key accelerator */
#define	SAFE_FLAGS_RNG		 0x02			/* hardware rng */

	int						 sc_suspended;
	int						 sc_needwakeup;	/* notify crypto layer */

	int						 sc_nqchip;		/* # passed to chip */
	int						 sc_nsessions;	/* # of sessions */
	struct safe_session		*sc_sessions;	/* sessions */

	struct timer_list		 sc_pkto;		/* PK polling */
	spinlock_t				 sc_pkmtx;		/* PK lock */
	struct list_head		 sc_pkq;		/* queue of PK requests */
	struct safe_pkq			*sc_pkq_cur;	/* current processing request */
	u_int32_t				 sc_pk_reslen, sc_pk_resoff;

	int						 sc_max_dsize;	/* maximum safe DMA size */
};
#endif /* __KERNEL__ */

struct safe_stats {
	u_int64_t st_ibytes;
	u_int64_t st_obytes;
	u_int32_t st_ipackets;
	u_int32_t st_opackets;
	u_int32_t st_invalid;		/* invalid argument */
	u_int32_t st_badsession;	/* invalid session id */
	u_int32_t st_badflags;		/* flags indicate !(mbuf | uio) */
	u_int32_t st_nodesc;		/* op submitted w/o descriptors */
	u_int32_t st_badalg;		/* unsupported algorithm */
	u_int32_t st_ringfull;		/* PE descriptor ring full */
	u_int32_t st_peoperr;		/* PE marked error */
	u_int32_t st_dmaerr;		/* PE DMA error */
	u_int32_t st_bypasstoobig;	/* bypass > 96 bytes */
	u_int32_t st_skipmismatch;	/* enc part begins before auth part */
	u_int32_t st_lenmismatch;	/* enc length different auth length */
	u_int32_t st_coffmisaligned;	/* crypto offset not 32-bit aligned */
	u_int32_t st_cofftoobig;	/* crypto offset > 255 words */
	u_int32_t st_iovmisaligned;	/* iov op not aligned */
	u_int32_t st_iovnotuniform;	/* iov op not suitable */
	u_int32_t st_unaligned;		/* unaligned src caused copy */
	u_int32_t st_notuniform;	/* non-uniform src caused copy */
	u_int32_t st_nomap;		/* bus_dmamap_create failed */
	u_int32_t st_noload;		/* bus_dmamap_load_* failed */
	u_int32_t st_nombuf;		/* MGET* failed */
	u_int32_t st_nomcl;		/* MCLGET* failed */
	u_int32_t st_maxqchip;		/* max mcr1 ops out for processing */
	u_int32_t st_rng;		/* RNG requests */
	u_int32_t st_rngalarm;		/* RNG alarm requests */
	u_int32_t st_noicvcopy;		/* ICV data copies suppressed */
};
#endif /* _SAFE_SAFEVAR_H_ */
