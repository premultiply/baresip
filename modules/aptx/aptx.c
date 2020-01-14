/**
 * @file aptx.c aptX Audio Codec
 *
 * Copyright (C) 2019 Hessischer Rundfunk
 */

#include <re.h>
#include <baresip.h>
#include "aptx.h"

/**
 * @defgroup aptx aptx
 *
 * aptX audio codec (Standard and HD variant)
 *
 * Supported version:
 *    libopenaptx 0.1.0 or later
 *
 * References:
 *    RFC 7310  RTP Payload Format for Standard apt-X
 *              and Enhanced apt-X Codecs
 *
 * TODOs:
 *    - Code check, cleanup and error handling
 *    - Add SDP fmtp negotiation & config preconfiguration
 *    - Check and implement other sampling rates and channel modes
 *    - Add optional real 24 bit audio I/O support
 *
 */

static struct aucodec aptx = {
	.name = "aptx",
	.srate = APTX_SRATE,
	.crate = APTX_SRATE,
	.ch = APTX_CHANNELS,
	.pch = APTX_CHANNELS,
	.ptime = 4,
	.encupdh = aptx_encode_update,
	.ench = aptx_encode_frm,
	.decupdh = aptx_decode_update,
	.dech = aptx_decode_frm,
	.fmtp_ench = aptx_fmtp_enc,
	.fmtp_cmph = aptx_fmtp_cmp,
};

static char fmtp_local[256] = "";
static char fmtp_mirror[256];

uint32_t aptx_variant, aptx_bitresolution;


void aptx_encode_fmtp(const struct aptx_param *prm)
{
	(void)re_snprintf(fmtp_local, sizeof(fmtp_local),
	                  "variant=%s; bitresolution=%u",
	                  prm->variant == APTX_VARIANT_HD ? "hd" : "standard",
	                  prm->bitresolution);
}


/* Parse remote fmtp string and map it to aptx_param struct */
void aptx_decode_fmtp(struct aptx_param *prm, const char *fmtp)
{
	struct pl pl, val;

	if (!prm || !fmtp)
		return;

	pl_set_str(&pl, fmtp);

	if (fmt_param_get(&pl, "variant", &val)) {
		if (!pl_strcasecmp(&val, "standard"))
			prm->variant = APTX_VARIANT_STANDARD;
		if (!pl_strcasecmp(&val, "hd"))
			prm->variant = APTX_VARIANT_HD;
	}

	if (fmt_param_get(&pl, "bitresolution", &val))
		prm->bitresolution = pl_u32(&val);

	warning("aptx: variant: %u, bitresolution: %u\n",
	      prm->variant, prm->bitresolution);
}


/* describe local encoded format to remote */
int aptx_fmtp_enc(struct mbuf *mb, const struct sdp_format *fmt, bool offer,
                  void *arg)
{
	bool mirror;

	(void)offer;
	(void)arg;

	if (!mb || !fmt)
		return 0;

	mirror = !offer && str_isset(fmtp_mirror);

	return mbuf_printf(mb, "a=fmtp:%s %s\r\n", fmt->id,
	                   mirror ? fmtp_mirror : fmtp_local);
}


void aptx_mirror_params(const char *x)
{
	debug("aptx: mirror parameters: \"%s\"\n", x);

	str_ncpy(fmtp_mirror, x, sizeof(fmtp_mirror));
}


static int module_init(void)
{
	struct conf *conf = conf_cur();
	struct aptx_param prm;

	/* default encoder configuration */
	aptx_variant = 0;

	/* encoder configuration from config file */
	(void)conf_get_u32(conf, "aptx_variant", &aptx_variant);

	prm.variant = aptx_variant;
	prm.bitresolution = (aptx_variant == APTX_VARIANT_HD) ? 24 : 16;

	aptx_encode_fmtp(&prm);

	debug("aptx: fmtp=\"%s\"\n", fmtp_local);

	aucodec_register(baresip_aucodecl(), &aptx);

	return 0;
}


static int module_close(void)
{
	aucodec_unregister(&aptx);

	return 0;
}


EXPORT_SYM const struct mod_export DECL_EXPORTS(aptx) = {
	"aptx",
	"audio codec",
	module_init,
	module_close,
};
