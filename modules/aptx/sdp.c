/**
 * @file aptx/sdp.c aptX SDP Functions
 *
 * Copyright (C) 2019 Hessischer Rundfunk
 */

#include <re.h>
#include <baresip.h>
#include <openaptx.h>
#include "aptx.h"


bool aptx_fmtp_cmp(const char *lfmtp, const char *rfmtp, void *arg)
{
	(void)lfmtp;
	(void)arg;

	struct pl pl, val;

	if (!rfmtp)
		return false;

	pl_set_str(&pl, rfmtp);

	debug("aptx: compare: %s\n", rfmtp);

	if (fmt_param_get(&pl, "variant", &val)) {
		if (pl_strcasecmp(&val, "standard") &&
		    pl_strcasecmp(&val, "hd"))
			return false;
	}

	return true;
}
