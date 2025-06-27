#ifndef __HCTINYPLAYER_REGISTRY_H__
#define __HCTINYPLAYER_REGISTRY_H__

struct AVCodecTag;
struct AVFormatContext;
struct AVProbeData;
struct AVPacket;
typedef struct AVInputFormat {
    /**
     * A comma separated list of short names for the format. New names
     * may be appended with a minor bump.
     */
    const char *name;

    /**
     * Descriptive name for the format, meant to be more human-readable
     * than name. You should use the NULL_IF_CONFIG_SMALL() macro
     * to define it.
     */
    const char *long_name;

    /**
     * Can use flags: AVFMT_NOFILE, AVFMT_NEEDNUMBER, AVFMT_SHOW_IDS,
     * AVFMT_NOTIMESTAMPS, AVFMT_GENERIC_INDEX, AVFMT_TS_DISCONT, AVFMT_NOBINSEARCH,
     * AVFMT_NOGENSEARCH, AVFMT_NO_BYTE_SEEK, AVFMT_SEEK_TO_PTS.
     */
    int flags;

    /**
     * If extensions are defined, then no probe is done. You should
     * usually not use extension format guessing because it is not
     * reliable enough
     */
    const char *extensions;

    const struct AVCodecTag * const *codec_tag;

    /**
     * Comma-separated list of mime types.
     * It is used check for matching mime types while probing.
     * @see av_probe_input_format2
     */
    const char *mime_type;

    /*****************************************************************
     * No fields below this line are part of the public API. They
     * may not be used outside of libavformat and can be changed and
     * removed at will.
     * New public fields should be added right above.
     *****************************************************************
     */
    /**
     * Raw demuxers store their codec ID here.
     */
    int raw_codec_id;

    /**
     * Size of private data so that it can be allocated in the wrapper.
     */
    int priv_data_size;

    /**
     * Internal flags. See FF_FMT_FLAG_* in internal.h.
     */
    int flags_internal;

    /**
     * Tell if a given file has a chance of being parsed as this format.
     * The buffer provided is guaranteed to be AVPROBE_PADDING_SIZE bytes
     * big so you do not have to check for that unless you need more.
     */
    int (*read_probe)(const struct AVProbeData *s);

    /**
     * Read the format header and initialize the AVFormatContext
     * structure. Return 0 if OK. 'avformat_new_stream' should be
     * called to create new streams.
     */
    int (*read_header)(struct AVFormatContext *s);

    /**
     * Read one packet and put it in 'pkt'. pts and flags are also
     * set. 'avformat_new_stream' can be called only if the flag
     * AVFMTCTX_NOHEADER is used and only in the calling thread (not in a
     * background thread).
     * @return 0 on success, < 0 on error.
     *         Upon returning an error, pkt must be unreferenced by the caller.
     */
    int (*read_packet)(struct AVFormatContext *s, struct AVPacket *pkt);

    /**
     * Close the stream. The AVFormatContext and AVStreams are not
     * freed by this function
     */
    int (*read_close)(struct AVFormatContext *s);

    /**
     * Seek to a given timestamp relative to the frames in
     * stream component stream_index.
     * @param stream_index Must not be -1.
     * @param flags Selects which direction should be preferred if no exact
     *              match is available.
     * @return >= 0 on success (but not necessarily the new offset)
     */
    int (*read_seek)(struct AVFormatContext *s,
                     int stream_index, int64_t timestamp, int flags);

    /**
     * Get the next timestamp in stream[stream_index].time_base units.
     * @return the timestamp or AV_NOPTS_VALUE if an error occurred
     */
    int64_t (*read_timestamp)(struct AVFormatContext *s, int stream_index,
                              int64_t *pos, int64_t pos_limit);
} AVInputFormat;

struct URLContext;
typedef struct URLProtocol {
    const char *name;
    int     (*url_open)(struct URLContext *h, const char *url, int flags);
    /**
     * Read data from the protocol.
     * If data is immediately available (even less than size), EOF is
     * reached or an error occurs (including EINTR), return immediately.
     * Otherwise:
     * In non-blocking mode, return AVERROR(EAGAIN) immediately.
     * In blocking mode, wait for data/EOF/error with a short timeout (0.1s),
     * and return AVERROR(EAGAIN) on timeout.
     * Checking interrupt_callback, looping on EINTR and EAGAIN and until
     * enough data has been read is left to the calling function; see
     * retry_transfer_wrapper in avio.c.
     */
    int     (*url_read)(struct URLContext *h, unsigned char *buf, int size);
    int64_t (*url_seek)(struct URLContext *h, int64_t pos, int whence);
    int     (*url_close)(struct URLContext *h);
    int (*url_read_pause)(struct URLContext *h, int pause);
    int64_t (*url_read_seek)(struct URLContext *h, int stream_index,
                             int64_t timestamp, int flags);
    int (*url_get_file_handle)(struct URLContext *h);
    int priv_data_size;
    int flags;
} URLProtocol;

struct AVCodecParserContext;
typedef struct AVCodecParser {
    int codec_ids[7]; /* several codec IDs are permitted */
    int priv_data_size;
    int (*parser_init)(struct AVCodecParserContext *s);
    /* This callback never returns an error, a negative value means that
     * the frame start was in a previous packet. */
    int (*parser_parse)(struct AVCodecParserContext *s,
                        const uint8_t **poutbuf, int *poutbuf_size,
                        const uint8_t *buf, int buf_size);
    void (*parser_close)(struct AVCodecParserContext *s);
    int (*split)(struct AVCodecParserContext *s, const uint8_t *buf, int buf_size);
} AVCodecParser;

#endif