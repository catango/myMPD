/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_WEBRADIO_H
#define MYMPD_API_WEBRADIO_H

#include "dist/sds/sds.h"
#include "src/lib/mympd_state.h"

sds mympd_api_webradio_from_uri_tojson(struct t_mympd_state *mympd_state, const char *uri);
sds mympd_api_webradio_print(struct t_webradio_data *webradio, sds buffer);
struct t_webradio_data *mympd_api_webradio_by_uri(struct t_mympd_state *mympd_state, const char *uri);
sds mympd_api_webradio_get_cover_by_uri(struct t_mympd_state *mympd_state, sds buffer, sds uri);
sds mympd_api_webradio_get_extm3u(struct t_mympd_state *mympd_state, sds buffer, sds uri);

#endif
