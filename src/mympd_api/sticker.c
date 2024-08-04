/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief myMPD sticker API
 */

#include "compile_time.h"
#include "src/mympd_api/sticker.h"

#include "src/lib/jsonrpc.h"
#include "src/mpd_client/stickerdb.h"
#include "src/mympd_api/trigger.h"

/**
 * Gets a sticker value
 * @param stickerdb pointer to stickerdb
 * @param buffer already allocated sds string to append the list
 * @param request_id jsonrpc request id
 * @param uri Sticker uri
 * @param type MPD sticker type
 * @param name Sticker name
 * @return Pointer to buffer
 */
sds mympd_api_sticker_get(struct t_stickerdb_state *stickerdb, sds buffer, unsigned request_id,
    sds uri, enum mpd_sticker_type type, sds name)
{
    sds value = stickerdb_get(stickerdb, type, uri, name);
    if (value == NULL) {
        return jsonrpc_respond_message(buffer, MYMPD_API_STICKER_GET, request_id,
            JSONRPC_FACILITY_STICKER, JSONRPC_SEVERITY_ERROR, "Sticker not found");
    }
    buffer = jsonrpc_respond_start(buffer, MYMPD_API_STICKER_GET, request_id);
    buffer = tojson_char(buffer, "value", value, false);
    buffer = jsonrpc_end(buffer);
    return buffer;
}

/**
 * Gets all MPD stickers for an uri.
 * @param stickerdb pointer to stickerdb
 * @param buffer already allocated sds string to append the list
 * @param request_id jsonrpc request id
 * @param uri Sticker uri
 * @param type MPD sticker type
 * @return Pointer to buffer
 */
sds mympd_api_sticker_list(struct t_stickerdb_state *stickerdb, sds buffer, unsigned request_id,
    sds uri, enum mpd_sticker_type type)
{
    struct t_stickers stickers;
    stickers_enable_all(&stickers);
    buffer = jsonrpc_respond_start(buffer, MYMPD_API_STICKER_LIST, request_id);
    buffer = mympd_api_sticker_get_print(buffer, stickerdb, type, uri, &stickers);
    buffer = jsonrpc_end(buffer);
    return buffer;
}

/**
 * Sets the like sticker and triggers the feedback event
 * @param stickerdb pointer to stickerdb
 * @param trigger_list pointer to trigger list
 * @param partition_name the partition name
 * @param sticker_type MPD sticker type
 * @param uri uri to set the feedback
 * @param feedback_type feedback type
 * @param value feedback value to set
 * @param error already allocated sds string to append the error message
 * @return true on success, else false
 */
bool mympd_api_sticker_set_feedback(struct t_stickerdb_state *stickerdb, struct t_list *trigger_list, const char *partition_name,
    enum mpd_sticker_type sticker_type, sds uri, enum mympd_feedback_type feedback_type, int value, sds *error)
{
    if (stickerdb->mpd_state->feat.stickers == false) {
        *error = sdscat(*error, "MPD stickers are disabled");
        return false;
    }
    bool rc = feedback_type == FEEDBACK_LIKE
        ? value == 1
            ? stickerdb_remove(stickerdb, sticker_type, uri, "like")
            : stickerdb_set_like(stickerdb, sticker_type, uri, (enum sticker_like)value)
        : value == 0
            ? stickerdb_remove(stickerdb, sticker_type, uri, "rating")
            : stickerdb_set_rating(stickerdb, sticker_type, uri, value);
    if (rc == false) {
        *error = sdscat(*error, "Failed to set feedback");
        return false;
    }
    //mympd_feedback trigger
    mympd_api_trigger_execute_feedback(trigger_list, uri, feedback_type, value, partition_name);
    return true;
}

/**
 * Gets the stickers from stickerdb and returns a json list
 * Shortcut for stickerdb_get_all and print_sticker
 * @param buffer already allocated sds string to append the list
 * @param stickerdb pointer to stickerdb
 * @param type MPD sticker type
 * @param uri song uri
 * @param stickers array of stickers to print
 * @return pointer to the modified buffer
 */
sds mympd_api_sticker_get_print(sds buffer, struct t_stickerdb_state *stickerdb,
        enum mpd_sticker_type type, const char *uri, const struct t_stickers *stickers)
{
    if (stickers->len == 0) {
        return buffer;
    }
    struct t_sticker sticker;
    if (stickerdb_get_all(stickerdb, type, uri, &sticker, stickers->user_defined) != NULL) {
        buffer = mympd_api_sticker_print(buffer, &sticker, stickers);
        sticker_struct_clear(&sticker);
    }
    return buffer;
}

/**
 * Gets the stickers from stickerdb and returns a json list.
 * Shortcut for stickerdb_get_all_batch and print_sticker.
 * You must exit the stickerdb idle mode before.
 * @param buffer already allocated sds string to append the list
 * @param stickerdb pointer to stickerdb
 * @param type MPD sticker type
 * @param uri song uri
 * @param stickers array of stickers to print
 * @return pointer to the modified buffer
 */
sds mympd_api_sticker_get_print_batch(sds buffer, struct t_stickerdb_state *stickerdb,
        enum mpd_sticker_type type, const char *uri, const struct t_stickers *stickers)
{
    if (stickers->len == 0) {
        return buffer;
    }
    struct t_sticker sticker;
    if (stickerdb_get_all_batch(stickerdb, type, uri, &sticker, stickers->user_defined) != NULL) {
        buffer = mympd_api_sticker_print(buffer, &sticker, stickers);
        sticker_struct_clear(&sticker);
    }
    return buffer;
}

/**
 * Print the sticker struct as json list
 * @param buffer already allocated sds string to append the list
 * @param sticker pointer to sticker struct to print
 * @param stickers array of stickers to print
 * @return pointer to the modified buffer
 */
sds mympd_api_sticker_print(sds buffer, struct t_sticker *sticker, const struct t_stickers *stickers) {
    if (sticker == NULL) {
        return buffer;
    }
    buffer = json_comma(buffer);
    for (size_t i = 0; i < stickers->len; i++) {
        if (i > 0) {
            buffer = sdscatlen(buffer, ",", 1);
        }
        buffer = tojson_int64(buffer, sticker_name_lookup(stickers->stickers[i]), sticker->mympd[stickers->stickers[i]], false);
    }
    if (stickers->user_defined == true) {
        struct t_list_node *current = sticker->user.head;
        while (current != NULL) {
            buffer = sdscatlen(buffer, ",", 1);
            buffer = tojson_char(buffer, current->key, current->value_p, false);
            current = current->next;
        }
    }
    return buffer;
}
