// Bloob's own demuxer (3.0 engine). See demux.h.
#include "demux.h"

#include <algorithm>
#include <cstring>
#include <deque>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/bsf.h>
#include <libavformat/avformat.h>
#include <libavutil/audio_fifo.h>
#include <libavutil/dovi_meta.h>
#include <libavutil/mastering_display_metadata.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

#include "log.h"

namespace {

constexpr AVRational kMicro = {1, 1000000};
constexpr int kOutRate = 48000;

std::string av_error(int code) {
  char buf[AV_ERROR_MAX_STRING_SIZE] = {0};
  av_strerror(code, buf, sizeof buf);
  return buf;
}

std::string tag(AVDictionary *meta, const char *key) {
  AVDictionaryEntry *e = av_dict_get(meta, key, nullptr, 0);
  return e && e->value ? e->value : "";
}

// Sound named as people know it ("TrueHD", "DTS-HD MA", "E-AC-3").
std::string audio_name(const AVCodecParameters *p) {
  switch (p->codec_id) {
    case AV_CODEC_ID_AC3: return "AC-3";
    case AV_CODEC_ID_EAC3: return p->profile == AV_PROFILE_EAC3_DDP_ATMOS ? "E-AC-3 Atmos" : "E-AC-3";
    case AV_CODEC_ID_TRUEHD: return "TrueHD";
    case AV_CODEC_ID_MLP: return "MLP";
    case AV_CODEC_ID_DTS:
      switch (p->profile) {
        case AV_PROFILE_DTS_HD_MA: return "DTS-HD MA";
        case AV_PROFILE_DTS_HD_MA_X:
        case AV_PROFILE_DTS_HD_MA_X_IMAX: return "DTS:X";
        case AV_PROFILE_DTS_HD_HRA: return "DTS-HD HRA";
        case AV_PROFILE_DTS_ES: return "DTS-ES";
        default: return "DTS";
      }
    case AV_CODEC_ID_AAC:
    case AV_CODEC_ID_AAC_LATM: return "AAC";
    case AV_CODEC_ID_FLAC: return "FLAC";
    case AV_CODEC_ID_OPUS: return "Opus";
    case AV_CODEC_ID_VORBIS: return "Vorbis";
    case AV_CODEC_ID_MP3: return "MP3";
    default: break;
  }
  if (p->codec_id >= AV_CODEC_ID_FIRST_AUDIO && p->codec_id < AV_CODEC_ID_ADPCM_IMA_QT) return "PCM";
  const AVCodecDescriptor *d = avcodec_descriptor_get(p->codec_id);
  return d ? d->name : "unknown";
}

std::string channels_name(int n) {
  switch (n) {
    case 1: return "Mono";
    case 2: return "Stereo";
    case 6: return "5.1";
    case 8: return "7.1";
    default: return std::to_string(n) + " ch";
  }
}

std::string text_codec(AVCodecID id) {
  switch (id) {
    case AV_CODEC_ID_SUBRIP:
    case AV_CODEC_ID_TEXT: return "SubRip";
    case AV_CODEC_ID_ASS:
    case AV_CODEC_ID_SSA: return "ASS";
    case AV_CODEC_ID_WEBVTT: return "WebVTT";
    case AV_CODEC_ID_MOV_TEXT: return "mov_text";
    case AV_CODEC_ID_HDMV_PGS_SUBTITLE: return "PGS";
    default: return "";
  }
}

std::string video_codec(AVCodecID id) {
  switch (id) {
    case AV_CODEC_ID_H264: return "H264";
    case AV_CODEC_ID_HEVC: return "H265";
    case AV_CODEC_ID_AV1: return "AV1";
    case AV_CODEC_ID_VP9: return "VP9";
    default: return "";
  }
}

}  // namespace

int demux_interrupt(void *opaque) {
  return static_cast<Demuxer *>(opaque)->interrupted_.load() ? 1 : 0;
}

struct Demuxer::Impl {
  AVFormatContext *fmt = nullptr;
  int video_stream = -1;
  std::vector<int> audio_streams;  // facts.audio[i] -> stream index
  std::vector<int> text_streams;
  int audio_stream = -1, text_stream = -1;
  long long origin_us = 0;  // the file's start time, so the film starts at 0
  AVBSFContext *bsf = nullptr;
  bool strip_dv = false;
  AVPacket *pkt = nullptr;
  // Sound conversion (when the TV can't take the track as it is).
  AVCodecContext *dec = nullptr;
  AVCodecContext *enc = nullptr;
  SwrContext *swr = nullptr;
  AVAudioFifo *fifo = nullptr;
  AVFrame *frame = nullptr;
  AVFrame *enc_frame = nullptr;
  // Headphones mode: the binaural chain between decoder and encoder.
  HeadphoneOptions hp;
  HeadphoneChain chain;
  AVFrame *hp_frame = nullptr;
  bool hp_failed = false;
  long long lead_us = 0;  // sound sent this much ahead of the picture
  long long next_pts_us = AV_NOPTS_VALUE;  // of the next encoded frame
  std::deque<DemuxPacket> ready;
  // Picture subtitles.
  AVCodecContext *sub_dec = nullptr;

  void close_audio() {
    avcodec_free_context(&dec);
    avcodec_free_context(&enc);
    swr_free(&swr);
    if (fifo) av_audio_fifo_free(fifo);
    fifo = nullptr;
    av_frame_free(&frame);
    av_frame_free(&enc_frame);
    av_frame_free(&hp_frame);
    chain.close();
    hp = HeadphoneOptions();
    hp_failed = false;
    lead_us = 0;
    next_pts_us = AV_NOPTS_VALUE;
  }
  void close_sub() { avcodec_free_context(&sub_dec); }
  void close() {
    close_audio();
    close_sub();
    av_bsf_free(&bsf);
    av_packet_free(&pkt);
    avformat_close_input(&fmt);
    ready.clear();
    video_stream = audio_stream = text_stream = -1;
    audio_streams.clear();
    text_streams.clear();
  }
  long long to_us(int64_t ts, AVRational tb) const {
    if (ts == AV_NOPTS_VALUE) return AV_NOPTS_VALUE;
    return av_rescale_q(ts, tb, kMicro) - origin_us;
  }
};

Demuxer::Demuxer() : impl_(new Impl) {}
Demuxer::~Demuxer() { close(); }

void Demuxer::close() {
  impl_->close();
  facts_ = DemuxFacts();
  audio_out_ = AudioOut();
  audio_choice_ = text_choice_ = -1;
}

bool Demuxer::open(const std::string &url, std::string *error) {
  close();
  Impl &m = *impl_;
  m.fmt = avformat_alloc_context();
  m.fmt->interrupt_callback.callback = demux_interrupt;
  m.fmt->interrupt_callback.opaque = this;
  AVDictionary *opts = nullptr;
  av_dict_set(&opts, "reconnect", "1", 0);
  av_dict_set(&opts, "reconnect_on_network_error", "1", 0);
  av_dict_set(&opts, "reconnect_delay_max", "4", 0);
  av_dict_set(&opts, "rw_timeout", "20000000", 0);
  av_dict_set(&opts, "probesize", "8000000", 0);
  int r = avformat_open_input(&m.fmt, url.c_str(), nullptr, &opts);
  av_dict_free(&opts);
  if (r < 0) {
    if (error) *error = "open: " + av_error(r);
    m.fmt = nullptr;
    return false;
  }
  m.fmt->max_analyze_duration = 3 * AV_TIME_BASE;
  r = avformat_find_stream_info(m.fmt, nullptr);
  if (r < 0) {
    if (error) *error = "stream info: " + av_error(r);
    m.close();
    return false;
  }
  DemuxFacts &f = facts_;
  f.container = m.fmt->iformat ? m.fmt->iformat->name : "";
  // MPEG-TS clocks start anywhere; Matroska and MP4 start at the film's zero
  // (their start_time is only the first packet's, which would shift chapters).
  if (f.container == "mpegts" && m.fmt->start_time != AV_NOPTS_VALUE) m.origin_us = av_rescale_q(m.fmt->start_time, AV_TIME_BASE_Q, kMicro);
  if (f.container.find("matroska") != std::string::npos) f.container = "MKV";
  else if (f.container.find("mp4") != std::string::npos) f.container = "MP4";
  else if (f.container == "mpegts") f.container = "TS";
  if (m.fmt->duration != AV_NOPTS_VALUE) f.duration_ms = m.fmt->duration / 1000;

  int best_video = av_find_best_stream(m.fmt, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
  for (unsigned i = 0; i < m.fmt->nb_streams; i++) {
    AVStream *st = m.fmt->streams[i];
    AVCodecParameters *p = st->codecpar;
    st->discard = AVDISCARD_ALL;  // until chosen
    if (p->codec_type == AVMEDIA_TYPE_AUDIO) {
      MediaTrack t;
      t.index = static_cast<int>(f.audio.size());
      t.language = tag(st->metadata, "language");
      t.title = tag(st->metadata, "title");
      t.codec = audio_name(p);
      t.channels = p->ch_layout.nb_channels;
      t.is_default = st->disposition & AV_DISPOSITION_DEFAULT;
      t.commentary = (st->disposition & AV_DISPOSITION_COMMENT) || t.title.find("ommentary") != std::string::npos;
      t.hearing_impaired = st->disposition & AV_DISPOSITION_HEARING_IMPAIRED;
      f.audio.push_back(t);
      m.audio_streams.push_back(static_cast<int>(i));
    } else if (p->codec_type == AVMEDIA_TYPE_SUBTITLE && !text_codec(p->codec_id).empty()) {
      MediaTrack t;
      t.index = static_cast<int>(f.text.size());
      t.language = tag(st->metadata, "language");
      t.title = tag(st->metadata, "title");
      t.codec = text_codec(p->codec_id);
      t.is_default = st->disposition & AV_DISPOSITION_DEFAULT;
      t.forced = (st->disposition & AV_DISPOSITION_FORCED) || t.title.find("orced") != std::string::npos;
      t.hearing_impaired = (st->disposition & AV_DISPOSITION_HEARING_IMPAIRED) || t.title.find("SDH") != std::string::npos;
      f.text.push_back(t);
      m.text_streams.push_back(static_cast<int>(i));
    } else if (static_cast<int>(i) == best_video && p->codec_type == AVMEDIA_TYPE_VIDEO) {
      m.video_stream = static_cast<int>(i);
      f.video_codec = video_codec(p->codec_id);
      f.width = p->width;
      f.height = p->height;
      AVRational fr = st->avg_frame_rate.num ? st->avg_frame_rate : st->r_frame_rate;
      f.fps_num = fr.num;
      f.fps_den = fr.den;
      f.transfer = p->color_trc;
      f.primaries = p->color_primaries;
      f.matrix = p->color_space;
      f.full_range = p->color_range == AVCOL_RANGE_JPEG;
      const AVPacketSideData *sd = av_packet_side_data_get(p->coded_side_data, p->nb_coded_side_data, AV_PKT_DATA_DOVI_CONF);
      if (sd && sd->size >= static_cast<int>(sizeof(AVDOVIDecoderConfigurationRecord))) {
        auto *dv = reinterpret_cast<const AVDOVIDecoderConfigurationRecord *>(sd->data);
        f.dv_profile = dv->dv_profile;
        f.dv_level = dv->dv_level;
        f.dv_el = dv->el_present_flag;
        f.dv_rpu = dv->rpu_present_flag;
        f.dv_bl_compatible = dv->dv_bl_signal_compatibility_id != 0;
      }
      sd = av_packet_side_data_get(p->coded_side_data, p->nb_coded_side_data, AV_PKT_DATA_MASTERING_DISPLAY_METADATA);
      if (sd) {
        auto *md = reinterpret_cast<const AVMasteringDisplayMetadata *>(sd->data);
        if (md->has_primaries) {
          f.has_mastering = true;
          for (int c = 0; c < 3; c++) {
            f.primaries_xy[c][0] = static_cast<int>(av_rescale(md->display_primaries[c][0].num, 50000, md->display_primaries[c][0].den));
            f.primaries_xy[c][1] = static_cast<int>(av_rescale(md->display_primaries[c][1].num, 50000, md->display_primaries[c][1].den));
          }
          f.white_xy[0] = static_cast<int>(av_rescale(md->white_point[0].num, 50000, md->white_point[0].den));
          f.white_xy[1] = static_cast<int>(av_rescale(md->white_point[1].num, 50000, md->white_point[1].den));
        }
        if (md->has_luminance) {
          f.has_mastering = true;
          f.max_luminance = av_rescale(md->max_luminance.num, 10000, md->max_luminance.den);
          f.min_luminance = av_rescale(md->min_luminance.num, 10000, md->min_luminance.den);
        }
      }
      sd = av_packet_side_data_get(p->coded_side_data, p->nb_coded_side_data, AV_PKT_DATA_CONTENT_LIGHT_LEVEL);
      if (sd) {
        auto *ll = reinterpret_cast<const AVContentLightMetadata *>(sd->data);
        f.has_light_level = true;
        f.max_cll = static_cast<int>(ll->MaxCLL);
        f.max_fall = static_cast<int>(ll->MaxFALL);
      }
    }
  }
  // Dolby Vision this TV shows as Dolby Vision: single-layer profiles 5, 8 and 9 (and 10).
  // Profile 7 (disc, two layers) plays its HDR10 base: the extra layer and RPU go.
  bool dv_plays = f.dv_profile == 5 || f.dv_profile == 8 || f.dv_profile == 9 || f.dv_profile == 10;
  m.strip_dv = f.dv_profile == 7 || (f.dv_profile >= 0 && !dv_plays);
  if (dv_plays) f.hdr = "Dolby Vision";
  else if (f.transfer == AVCOL_TRC_SMPTE2084) f.hdr = "HDR10";
  else if (f.transfer == AVCOL_TRC_ARIB_STD_B67) f.hdr = "HLG";
  else f.hdr = "SDR";

  for (unsigned i = 0; i < m.fmt->nb_chapters; i++) {
    AVChapter *c = m.fmt->chapters[i];
    DemuxFacts::Chapter ch;
    ch.start_ms = m.to_us(c->start, c->time_base) / 1000;
    ch.end_ms = m.to_us(c->end, c->time_base) / 1000;
    ch.title = tag(c->metadata, "title");
    f.chapters.push_back(ch);
  }

  if (m.video_stream < 0 || f.video_codec.empty()) {
    if (error) *error = m.video_stream < 0 ? "no video" : "video codec not supported";
    m.close();
    return false;
  }
  AVStream *vs = m.fmt->streams[m.video_stream];
  vs->discard = AVDISCARD_DEFAULT;
  const char *filter = vs->codecpar->codec_id == AV_CODEC_ID_H264 ? "h264_mp4toannexb"
                       : vs->codecpar->codec_id == AV_CODEC_ID_HEVC ? "hevc_mp4toannexb" : "null";
  const AVBitStreamFilter *bsf = av_bsf_get_by_name(filter);
  if (!bsf || av_bsf_alloc(bsf, &m.bsf) < 0 || avcodec_parameters_copy(m.bsf->par_in, vs->codecpar) < 0) {
    if (error) *error = std::string("bitstream filter ") + filter;
    m.close();
    return false;
  }
  m.bsf->time_base_in = vs->time_base;
  if (av_bsf_init(m.bsf) < 0) {
    if (error) *error = std::string("bitstream filter init ") + filter;
    m.close();
    return false;
  }
  m.pkt = av_packet_alloc();
  logf("demux", "%s %s %dx%d %s%s, %zu audio, %zu subtitles, %zu chapters, %lld ms", f.container.c_str(),
       f.video_codec.c_str(), f.width, f.height, f.hdr.c_str(),
       f.dv_profile >= 0 ? (" (Dolby Vision profile " + std::to_string(f.dv_profile) + (m.strip_dv ? ", base layer only)" : ")")).c_str() : "",
       f.audio.size(), f.text.size(), f.chapters.size(), f.duration_ms);
  return true;
}

bool Demuxer::select_audio(int index) {
  Impl &m = *impl_;
  if (!m.fmt) return false;
  if (m.audio_stream >= 0) m.fmt->streams[m.audio_stream]->discard = AVDISCARD_ALL;
  m.close_audio();
  m.ready.clear();
  m.audio_stream = -1;
  audio_choice_ = -1;
  audio_out_ = AudioOut();
  for (auto &t : facts_.audio) t.selected = false;
  if (index < 0 || index >= static_cast<int>(m.audio_streams.size())) return index < 0;
  AVStream *st = m.fmt->streams[m.audio_streams[static_cast<size_t>(index)]];
  AVCodecParameters *p = st->codecpar;
  AudioOut out;
  out.from = facts_.audio[static_cast<size_t>(index)].codec + " " + channels_name(p->ch_layout.nb_channels);
  const bool hp = headphones_.enabled;
  if (!hp && (p->codec_id == AV_CODEC_ID_AC3 || p->codec_id == AV_CODEC_ID_EAC3)) {
    out.codec = p->codec_id == AV_CODEC_ID_AC3 ? "AC3" : "AC3 PLUS";
    out.channels = p->ch_layout.nb_channels;
    out.sample_rate = p->sample_rate;
    out.atmos = p->profile == AV_PROFILE_EAC3_DDP_ATMOS;
  } else {
    // Decode, then E-AC-3: 5.1 for anything with surround, stereo otherwise;
    // in Headphones mode, the binaural render's stereo.
    const AVCodec *dc = avcodec_find_decoder(p->codec_id);
    const AVCodec *ec = avcodec_find_encoder(AV_CODEC_ID_EAC3);
    if (!dc || !ec) {
      logf("demux", "no decoder for %s", out.from.c_str());
      return false;
    }
    m.dec = avcodec_alloc_context3(dc);
    avcodec_parameters_to_context(m.dec, p);
    m.dec->pkt_timebase = st->time_base;
    AVDictionary *dopts = nullptr;
    // Dolby decoders: the stream's compression off for full dynamics (Night
    // keeps it). Dialogue normalisation isn't applied by FFmpeg's decoder.
    if (hp) av_dict_set(&dopts, "drc_scale", headphones_.night ? "1" : "0", 0);
    int opened = avcodec_open2(m.dec, dc, &dopts);
    av_dict_free(&dopts);
    if (opened < 0) {
      m.close_audio();
      logf("demux", "could not open the %s decoder", out.from.c_str());
      return false;
    }
    int in_ch = p->ch_layout.nb_channels;
    AVChannelLayout layout;
    av_channel_layout_default(&layout, hp ? 2 : in_ch >= 3 ? 6 : in_ch == 1 ? 1 : 2);  // 5.1, mono or stereo
    m.enc = avcodec_alloc_context3(ec);
    m.enc->sample_fmt = AV_SAMPLE_FMT_FLTP;
    m.enc->sample_rate = kOutRate;
    av_channel_layout_copy(&m.enc->ch_layout, &layout);
    m.enc->bit_rate = hp ? 448000 : in_ch >= 3 ? 1024000 : 256000;
    m.enc->time_base = AVRational{1, kOutRate};
    if (avcodec_open2(m.enc, ec, nullptr) < 0) {
      m.close_audio();
      logf("demux", "could not open the E-AC-3 encoder");
      return false;
    }
    // The decoder's layout may only be known after the first frame; swr (and
    // the headphone chain) are set up then.
    m.fifo = av_audio_fifo_alloc(AV_SAMPLE_FMT_FLTP, layout.nb_channels, m.enc->frame_size * 4);
    m.frame = av_frame_alloc();
    m.enc_frame = av_frame_alloc();
    if (hp) {
      m.hp = headphones_;
      m.hp_frame = av_frame_alloc();
      m.lead_us = std::max(0, headphones_.lead_ms) * 1000LL;
    }
    out.codec = "AC3 PLUS";
    out.channels = layout.nb_channels;
    out.sample_rate = kOutRate;
    out.converted = true;
    out.headphones = hp;
  }
  st->discard = AVDISCARD_DEFAULT;
  m.audio_stream = m.audio_streams[static_cast<size_t>(index)];
  audio_choice_ = index;
  audio_out_ = out;
  facts_.audio[static_cast<size_t>(index)].selected = true;
  logf("demux", "sound: %s%s", out.from.c_str(),
       out.headphones ? " -> headphones, Dolby Digital Plus stereo" : out.converted ? " -> Dolby Digital Plus (converted by Bloob)" : " (passed to the TV)");
  return true;
}

void Demuxer::select_text(int index) {
  Impl &m = *impl_;
  if (!m.fmt) return;
  if (m.text_stream >= 0) m.fmt->streams[m.text_stream]->discard = AVDISCARD_ALL;
  m.close_sub();
  m.text_stream = -1;
  text_choice_ = -1;
  for (auto &t : facts_.text) t.selected = false;
  if (index < 0 || index >= static_cast<int>(m.text_streams.size())) return;
  AVStream *st = m.fmt->streams[m.text_streams[static_cast<size_t>(index)]];
  if (st->codecpar->codec_id == AV_CODEC_ID_HDMV_PGS_SUBTITLE) {
    const AVCodec *dc = avcodec_find_decoder(AV_CODEC_ID_HDMV_PGS_SUBTITLE);
    m.sub_dec = dc ? avcodec_alloc_context3(dc) : nullptr;
    if (m.sub_dec) {
      avcodec_parameters_to_context(m.sub_dec, st->codecpar);
      m.sub_dec->pkt_timebase = st->time_base;
      if (avcodec_open2(m.sub_dec, dc, nullptr) < 0) m.close_sub();
    }
    if (!m.sub_dec) return;
  }
  st->discard = AVDISCARD_DEFAULT;
  m.text_stream = m.text_streams[static_cast<size_t>(index)];
  text_choice_ = index;
  facts_.text[static_cast<size_t>(index)].selected = true;
}

bool Demuxer::seek(long long ms) {
  Impl &m = *impl_;
  if (!m.fmt) return false;
  int64_t target = av_rescale_q(ms * 1000 + m.origin_us, kMicro, AV_TIME_BASE_Q);
  int r = av_seek_frame(m.fmt, -1, target, AVSEEK_FLAG_BACKWARD);
  if (r < 0) {
    logf("demux", "seek to %lld ms failed: %s", ms, av_error(r).c_str());
    return false;
  }
  av_bsf_flush(m.bsf);
  if (m.dec) avcodec_flush_buffers(m.dec);
  if (m.enc) avcodec_flush_buffers(m.enc);
  if (m.fifo) av_audio_fifo_reset(m.fifo);
  swr_free(&m.swr);
  m.chain.close();  // reopened at the next frame, without the old sound in it
  m.next_pts_us = AV_NOPTS_VALUE;
  if (m.sub_dec) avcodec_flush_buffers(m.sub_dec);
  m.ready.clear();
  return true;
}

namespace {

// Encodes whole frames from the FIFO into ready packets.
void drain_fifo(Demuxer::Impl &m) {
  const int n = m.enc->frame_size;
  while (av_audio_fifo_size(m.fifo) >= n) {
    av_frame_unref(m.enc_frame);
    m.enc_frame->nb_samples = n;
    m.enc_frame->format = AV_SAMPLE_FMT_FLTP;
    m.enc_frame->sample_rate = kOutRate;
    av_channel_layout_copy(&m.enc_frame->ch_layout, &m.enc->ch_layout);
    if (av_frame_get_buffer(m.enc_frame, 0) < 0) return;
    av_audio_fifo_read(m.fifo, reinterpret_cast<void **>(m.enc_frame->data), n);
    m.enc_frame->pts = av_rescale_q(m.next_pts_us, kMicro, m.enc->time_base);
    long long pts = m.next_pts_us;
    m.next_pts_us += static_cast<long long>(n) * 1000000 / kOutRate;
    if (avcodec_send_frame(m.enc, m.enc_frame) < 0) continue;
    AVPacket *out = av_packet_alloc();
    while (avcodec_receive_packet(m.enc, out) == 0) {
      DemuxPacket p;
      p.kind = DemuxPacket::Audio;
      p.data.assign(out->data, out->data + out->size);
      p.pts_us = pts - m.lead_us;
      if (p.pts_us < 0) {  // led out of the film's start: nothing to play
        av_packet_unref(out);
        continue;
      }
      p.duration_us = static_cast<long long>(n) * 1000000 / kOutRate;
      m.ready.push_back(std::move(p));
      av_packet_unref(out);
    }
    av_packet_free(&out);
  }
}

// Resamples a frame into the encoder's layout and encodes what fills a frame.
void queue_samples(Demuxer::Impl &m, AVFrame *f) {
  if (!m.swr) {
    if (swr_alloc_set_opts2(&m.swr, &m.enc->ch_layout, AV_SAMPLE_FMT_FLTP, kOutRate, &f->ch_layout,
                            static_cast<AVSampleFormat>(f->format), f->sample_rate, 0, nullptr) < 0 ||
        swr_init(m.swr) < 0) {
      swr_free(&m.swr);
      return;
    }
  }
  int out_max = swr_get_out_samples(m.swr, f->nb_samples);
  uint8_t **buf = nullptr;
  if (av_samples_alloc_array_and_samples(&buf, nullptr, m.enc->ch_layout.nb_channels, out_max, AV_SAMPLE_FMT_FLTP, 0) >= 0) {
    int got = swr_convert(m.swr, buf, out_max, const_cast<const uint8_t **>(f->extended_data), f->nb_samples);
    if (got > 0) av_audio_fifo_write(m.fifo, reinterpret_cast<void **>(buf), got);
    av_freep(&buf[0]);
    av_freep(&buf);
  }
  drain_fifo(m);
}

void convert_audio(Demuxer::Impl &m, AVPacket *pkt) {
  if (avcodec_send_packet(m.dec, pkt) < 0) return;
  while (avcodec_receive_frame(m.dec, m.frame) == 0) {
    if (m.next_pts_us == AV_NOPTS_VALUE) {
      int64_t ts = m.frame->best_effort_timestamp;
      m.next_pts_us = ts == AV_NOPTS_VALUE ? 0 : m.to_us(ts, m.dec->pkt_timebase);
    }
    if (m.hp_frame && !m.hp_failed) {
      // Headphones: through the binaural chain (built for the decoder's real layout).
      if (!m.chain.is_open()) {
        std::string why;
        if (!m.chain.open(m.hp, &m.frame->ch_layout, m.frame->sample_rate, m.frame->format, &why)) {
          // Without the chain the sound still plays: a plain stereo downmix.
          logf("demux", "headphones off for this track: %s", why.c_str());
          m.hp_failed = true;
        }
      }
      if (!m.hp_failed) {
        m.frame->pts = av_rescale_q(m.frame->best_effort_timestamp, m.dec->pkt_timebase, AVRational{1, m.frame->sample_rate});
        m.chain.push(m.frame);
        av_frame_unref(m.frame);
        while (m.chain.pull(m.hp_frame)) {
          queue_samples(m, m.hp_frame);
          av_frame_unref(m.hp_frame);
        }
        continue;
      }
    }
    queue_samples(m, m.frame);
    av_frame_unref(m.frame);
  }
}

}  // namespace

DemuxPacket Demuxer::read() {
  Impl &m = *impl_;
  DemuxPacket out;
  if (!m.ready.empty()) {
    out = std::move(m.ready.front());
    m.ready.pop_front();
    return out;
  }
  if (!m.fmt) {
    out.kind = DemuxPacket::Error;
    out.text = "not open";
    return out;
  }
  int r = av_read_frame(m.fmt, m.pkt);
  if (r == AVERROR_EOF) {
    out.kind = DemuxPacket::End;
    return out;
  }
  if (r < 0) {
    out.kind = interrupted_.load() || r == AVERROR(EAGAIN) ? DemuxPacket::Again : DemuxPacket::Error;
    out.text = av_error(r);
    return out;
  }
  AVStream *st = m.fmt->streams[m.pkt->stream_index];
  if (m.pkt->stream_index == m.video_stream) {
    if (av_bsf_send_packet(m.bsf, m.pkt) < 0) {
      av_packet_unref(m.pkt);
      return out;
    }
    while (av_bsf_receive_packet(m.bsf, m.pkt) == 0) {
      DemuxPacket p;
      p.kind = DemuxPacket::Video;
      p.data.assign(m.pkt->data, m.pkt->data + m.pkt->size);
      if (m.strip_dv) demux::strip_hevc_nals(p.data, 62, 63);
      int64_t ts = m.pkt->pts != AV_NOPTS_VALUE ? m.pkt->pts : m.pkt->dts;
      p.pts_us = m.to_us(ts, m.bsf->time_base_out);
      p.duration_us = m.pkt->duration > 0 ? av_rescale_q(m.pkt->duration, m.bsf->time_base_out, kMicro) : 0;
      p.key = m.pkt->flags & AV_PKT_FLAG_KEY;
      m.ready.push_back(std::move(p));
      av_packet_unref(m.pkt);
    }
  } else if (m.pkt->stream_index == m.audio_stream) {
    if (m.dec) {
      convert_audio(m, m.pkt);
    } else {
      DemuxPacket p;
      p.kind = DemuxPacket::Audio;
      p.data.assign(m.pkt->data, m.pkt->data + m.pkt->size);
      p.pts_us = m.to_us(m.pkt->pts != AV_NOPTS_VALUE ? m.pkt->pts : m.pkt->dts, st->time_base);
      p.duration_us = m.pkt->duration > 0 ? av_rescale_q(m.pkt->duration, st->time_base, kMicro) : 0;
      m.ready.push_back(std::move(p));
    }
  } else if (m.pkt->stream_index == m.text_stream) {
    DemuxPacket p;
    p.pts_us = m.to_us(m.pkt->pts, st->time_base);
    p.duration_us = m.pkt->duration > 0 ? av_rescale_q(m.pkt->duration, st->time_base, kMicro) : 0;
    if (m.sub_dec) {
      AVSubtitle sub;
      int got = 0;
      if (avcodec_decode_subtitle2(m.sub_dec, &sub, &got, m.pkt) >= 0 && got) {
        auto pic = std::make_shared<SubPicture>();
        pic->canvas_w = m.sub_dec->width > 0 ? m.sub_dec->width : 1920;
        pic->canvas_h = m.sub_dec->height > 0 ? m.sub_dec->height : 1080;
        for (unsigned i = 0; i < sub.num_rects; i++) {
          AVSubtitleRect *rect = sub.rects[i];
          if (rect->type != SUBTITLE_BITMAP || rect->w <= 0 || rect->h <= 0) continue;
          SubPicture::Piece piece;
          piece.x = rect->x;
          piece.y = rect->y;
          piece.w = rect->w;
          piece.h = rect->h;
          piece.rgba.resize(static_cast<size_t>(rect->w) * rect->h);
          const uint32_t *palette = reinterpret_cast<const uint32_t *>(rect->data[1]);  // ARGB in native order
          for (int y = 0; y < rect->h; y++)
            for (int x = 0; x < rect->w; x++) {
              uint32_t argb = palette[rect->data[0][y * rect->linesize[0] + x]];
              uint8_t px[4] = {static_cast<uint8_t>(argb >> 16), static_cast<uint8_t>(argb >> 8), static_cast<uint8_t>(argb),
                               static_cast<uint8_t>(argb >> 24)};
              std::memcpy(&piece.rgba[static_cast<size_t>(y) * rect->w + x], px, 4);
            }
          pic->pieces.push_back(std::move(piece));
        }
        if (sub.end_display_time && sub.end_display_time != UINT32_MAX) p.duration_us = sub.end_display_time * 1000LL;
        p.pts_us += sub.start_display_time * 1000LL;
        avsubtitle_free(&sub);
        p.kind = DemuxPacket::Picture;
        p.picture = pic;
        m.ready.push_back(std::move(p));
      }
    } else {
      p.kind = DemuxPacket::Text;
      std::string payload(reinterpret_cast<const char *>(m.pkt->data), static_cast<size_t>(m.pkt->size));
      p.text = demux::cue_text(text_codec(st->codecpar->codec_id), payload);
      if (!p.text.empty()) m.ready.push_back(std::move(p));
    }
  }
  av_packet_unref(m.pkt);
  if (!m.ready.empty()) {
    out = std::move(m.ready.front());
    m.ready.pop_front();
  }
  return out;
}

namespace demux {

void strip_hevc_nals(std::vector<uint8_t> &b, int type_a, int type_b) {
  // Each unit runs from its start code (00 00 01, or 00 00 00 01) to the next one.
  const size_t n = b.size();
  std::vector<size_t> starts;  // where each unit's start code begins
  for (size_t k = 0; k + 2 < n; k++) {
    if (b[k] == 0 && b[k + 1] == 0 && b[k + 2] == 1) {
      starts.push_back(k > 0 && b[k - 1] == 0 ? k - 1 : k);
      k += 2;
    }
  }
  bool drop = false;
  for (size_t s : starts) {
    size_t payload = s + (b[s + 2] == 1 ? 3 : 4);
    int type = payload < n ? (b[payload] >> 1) & 0x3f : -1;
    drop = drop || type == type_a || type == type_b;
  }
  if (!drop) return;
  std::vector<uint8_t> out;
  out.reserve(n);
  if (!starts.empty() && starts[0] > 0) out.insert(out.end(), b.begin(), b.begin() + static_cast<long>(starts[0]));
  for (size_t u = 0; u < starts.size(); u++) {
    size_t s = starts[u], e = u + 1 < starts.size() ? starts[u + 1] : n;
    size_t payload = s + (b[s + 2] == 1 ? 3 : 4);
    int type = payload < n ? (b[payload] >> 1) & 0x3f : -1;
    if (type == type_a || type == type_b) continue;
    out.insert(out.end(), b.begin() + static_cast<long>(s), b.begin() + static_cast<long>(e));
  }
  b.swap(out);
}

std::string cue_text(const std::string &codec, const std::string &payload) {
  std::string t = payload;
  if (codec == "mov_text" && t.size() >= 2) {
    size_t len = (static_cast<unsigned char>(t[0]) << 8) | static_cast<unsigned char>(t[1]);
    t = t.substr(2, std::min(len, t.size() - 2));
  }
  if (codec == "ASS") {
    // Matroska ASS: ReadOrder,Layer,Style,Name,MarginL,MarginR,MarginV,Effect,Text
    size_t pos = 0;
    for (int commas = 0; commas < 8 && pos != std::string::npos; commas++) {
      pos = t.find(',', pos);
      if (pos != std::string::npos) pos++;
    }
    if (pos != std::string::npos) t = t.substr(pos);
    std::string out;
    for (size_t k = 0; k < t.size(); k++) {
      if (t[k] == '{') {
        size_t close = t.find('}', k);
        if (close == std::string::npos) break;
        k = close;
      } else if (t[k] == '\\' && k + 1 < t.size() && (t[k + 1] == 'N' || t[k + 1] == 'n')) {
        out += '\n';
        k++;
      } else if (t[k] == '\\' && k + 1 < t.size() && t[k + 1] == 'h') {
        out += ' ';
        k++;
      } else {
        out += t[k];
      }
    }
    t = out;
  } else {
    // SubRip/WebVTT: drop <i>, <b>, <font …> and similar tags.
    std::string out;
    for (size_t k = 0; k < t.size(); k++) {
      if (t[k] == '<') {
        size_t close = t.find('>', k);
        if (close == std::string::npos) {
          out += t.substr(k);
          break;
        }
        k = close;
      } else if (t[k] != '\r') {
        out += t[k];
      }
    }
    t = out;
  }
  while (!t.empty() && (t.back() == '\n' || t.back() == ' ')) t.pop_back();
  while (!t.empty() && (t.front() == '\n' || t.front() == ' ')) t.erase(t.begin());
  return t;
}

}  // namespace demux
