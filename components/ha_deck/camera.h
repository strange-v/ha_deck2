#pragma once

#include "esphome/core/defines.h"

#ifdef USE_HA_DECK_CAMERA

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>

#include "esphome/core/automation.h"

#include "widget.h"

class JPEGDEC;
struct jpeg_draw_tag;

namespace esphome::ha_deck {

// Full-frame MJPEG viewer. A receiver thread drains a multipart/x-mixed-replace
// HTTP stream into the newest-JPEG slot, a decoder thread turns the newest JPEG
// into one of three RGB565 buffers, and the LVGL thread shows the newest frame.
// Receiving never waits for decoding, so the server never queues stale frames.
// The stream is open only while the owning screen is active, the widget is
// visible, and the viewer is not paused.
class HaDeckCamera : public HaDeckWidget {
 public:
  void set_geometry(int16_t x, int16_t y, uint16_t width, uint16_t height) {
    this->x_ = x;
    this->y_ = y;
    this->width_ = width;
    this->height_ = height;
  }
  void set_endpoint(const std::string &host, uint16_t port, const std::string &path) {
    this->host_ = host;
    this->port_ = port;
    this->path_ = path;
  }
  void set_timeout(uint32_t timeout) { this->timeout_ = timeout; }
  void set_connecting_text(const std::string &text) { this->connecting_text_ = text; }
  void set_error_text(const std::string &text) { this->error_text_ = text; }
  void set_paused_text(const std::string &text) { this->paused_text_ = text; }
  Trigger<> *get_click_trigger() { return &this->click_trigger_; }

  void mount(lv_obj_t *parent, HaDeckTheme *theme) override;
  void unmount() override;
  void apply_theme(HaDeckTheme *theme) override;
  void on_shown() override;
  void on_hidden() override;
  uint32_t screen_timeout() const override { return this->timeout_; }

 protected:
  enum class State : uint8_t { IDLE, CONNECTING, STREAMING, ERROR };

  lv_obj_t *get_root_obj_() const override { return this->root_; }
  void update_state_(bool force) override;

  static void event_callback_(lv_event_t *event);
  static void timer_callback_(lv_timer_t *timer);
  static int draw_callback_(jpeg_draw_tag *draw);

  // LVGL thread.
  void sync_stream_();
  void start_stream_();
  void stop_stream_();
  void present_frame_();
  void render_status_(bool force);
  bool allocate_buffers_();

  // Receiver thread.
  void worker_loop_();
  void run_session_();
  bool connect_();
  void close_socket_();
  bool fill_();
  bool read_line_(std::string &line);
  bool read_exact_(uint8_t *dst, size_t size);
  void publish_jpeg_(size_t size);
  void wait_(uint32_t ms);

  // Decoder thread.
  void decoder_loop_();
  bool decode_frame_(const uint8_t *data, size_t size);

  int16_t x_{0};
  int16_t y_{0};
  uint16_t width_{480};
  uint16_t height_{480};
  std::string host_{};
  uint16_t port_{80};
  std::string path_{"/"};
  uint32_t timeout_{300000};
  std::string connecting_text_{};
  std::string error_text_{};
  std::string paused_text_{};
  Trigger<> click_trigger_{};

  lv_obj_t *root_{nullptr};
  lv_obj_t *image_{nullptr};
  lv_obj_t *label_{nullptr};
  lv_timer_t *timer_{nullptr};
  bool shown_{false};
  bool paused_{false};
  bool streaming_{false};
  bool has_frame_{false};
  State rendered_state_{State::IDLE};
  bool rendered_paused_{false};
  bool rendered_has_frame_{false};

  // Triple buffering: LVGL owns front_, the worker owns back_, and latest_ holds
  // the newest completed frame until LVGL swaps it in.
  uint8_t *buffers_[3]{nullptr, nullptr, nullptr};
  uint16_t buffer_frame_width_[3]{0, 0, 0};
  uint16_t buffer_frame_height_[3]{0, 0, 0};
  lv_image_dsc_t descriptors_[3]{};
  uint8_t front_{0};
  uint8_t latest_{1};
  uint8_t back_{2};
  bool fresh_{false};
  std::mutex frame_mutex_{};

  std::thread worker_{};
  std::thread decoder_{};
  std::atomic<uint32_t> session_{0};
  std::mutex control_mutex_{};
  std::condition_variable control_cv_{};
  std::atomic<bool> active_{false};
  std::atomic<State> state_{State::IDLE};
  std::atomic<int> socket_{-1};

  // Same scheme for compressed frames: the receiver owns jpeg_rx_, the decoder
  // owns jpeg_decode_, and jpeg_latest_ holds the newest complete JPEG.
  uint8_t *jpeg_slots_[3]{nullptr, nullptr, nullptr};
  size_t jpeg_sizes_[3]{0, 0, 0};
  uint32_t jpeg_sessions_[3]{0, 0, 0};
  uint8_t jpeg_rx_{0};
  uint8_t jpeg_latest_{1};
  uint8_t jpeg_decode_{2};
  bool jpeg_fresh_{false};
  uint32_t jpeg_received_{0};
  uint32_t jpeg_overwritten_{0};
  std::mutex jpeg_mutex_{};
  std::condition_variable jpeg_cv_{};

  JPEGDEC *jpeg_{nullptr};
  uint8_t *rx_{nullptr};
  size_t rx_pos_{0};
  size_t rx_len_{0};
  uint32_t last_data_at_{0};
  int16_t draw_offset_x_{0};
  int16_t draw_offset_y_{0};
  uint32_t copy_us_{0};
};

}  // namespace esphome::ha_deck

#endif  // USE_HA_DECK_CAMERA
