#include "camera.h"

#ifdef USE_HA_DECK_CAMERA

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>

#include <fcntl.h>
#include <netdb.h>
#include <strings.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <JPEGDEC.h>

#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

#ifdef USE_ESP32
#include <esp_pthread.h>
#endif

// Part of LVGL's public API, but lvgl.h does not include its header.
extern "C" void lv_image_cache_drop(const void *src);

namespace esphome::ha_deck {

static const char *const TAG = "ha_deck.camera";

static constexpr lv_style_selector_t MAIN_DEFAULT =
    static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT);

static constexpr size_t RX_BUFFER_SIZE = 4096;
static constexpr size_t MAX_FRAME_SIZE = 128 * 1024;
static constexpr size_t MAX_LINE_LENGTH = 512;
static constexpr uint32_t CONNECT_TIMEOUT_MS = 5000;
static constexpr uint32_t STALL_TIMEOUT_MS = 5000;
static constexpr uint32_t RECEIVE_POLL_MS = 200;
static constexpr uint32_t PRESENT_INTERVAL_MS = 10;
static constexpr uint32_t STATS_INTERVAL_MS = 10000;
static constexpr uint32_t RETRY_DELAYS_MS[] = {1000, 2000, 5000, 10000};
static constexpr lv_opa_t STALE_FRAME_OPA = LV_OPA_40;

template<typename F> static std::thread spawn_thread(const char *name, int priority, F &&body) {
#ifdef USE_ESP32
  esp_pthread_cfg_t cfg = esp_pthread_get_default_config();
  cfg.stack_size = 6144;
  cfg.prio = priority;
  cfg.thread_name = name;
#if !CONFIG_FREERTOS_UNICORE
  cfg.pin_to_core = 0;
#endif
  esp_pthread_set_cfg(&cfg);
  std::thread thread(std::forward<F>(body));
  esp_pthread_cfg_t restore = esp_pthread_get_default_config();
  esp_pthread_set_cfg(&restore);
  return thread;
#else
  return std::thread(std::forward<F>(body));
#endif
}

void HaDeckCamera::mount(lv_obj_t *parent, HaDeckTheme *theme) {
  this->root_ = lv_obj_create(parent);
  lv_obj_set_pos(this->root_, this->x_, this->y_);
  lv_obj_set_size(this->root_, this->width_, this->height_);
  lv_obj_remove_flag(this->root_, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scrollbar_mode(this->root_, LV_SCROLLBAR_MODE_OFF);
  lv_obj_add_flag(this->root_, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_pad_all(this->root_, 0, MAIN_DEFAULT);
  lv_obj_set_style_border_width(this->root_, 0, MAIN_DEFAULT);
  lv_obj_set_style_radius(this->root_, 0, MAIN_DEFAULT);
  lv_obj_set_style_bg_color(this->root_, lv_color_black(), MAIN_DEFAULT);
  lv_obj_set_style_bg_opa(this->root_, LV_OPA_COVER, MAIN_DEFAULT);
  lv_obj_add_event_cb(this->root_, HaDeckCamera::event_callback_, LV_EVENT_CLICKED, this);

  this->image_ = lv_image_create(this->root_);
  lv_obj_set_pos(this->image_, 0, 0);
  lv_obj_set_size(this->image_, this->width_, this->height_);
  lv_obj_remove_flag(this->image_, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(this->image_, LV_OBJ_FLAG_EVENT_BUBBLE);

  this->label_ = lv_label_create(this->root_);
  lv_obj_set_style_text_align(this->label_, LV_TEXT_ALIGN_CENTER, MAIN_DEFAULT);
  lv_obj_center(this->label_);

  // A previously decoded frame may be from long ago; it is shown again only
  // after the stream delivers a new one.
  this->has_frame_ = false;
  if (this->buffers_[this->front_] != nullptr)
    lv_image_set_src(this->image_, &this->descriptors_[this->front_]);
  this->timer_ = lv_timer_create(HaDeckCamera::timer_callback_, PRESENT_INTERVAL_MS, this);

  this->apply_theme(theme);
  this->update(true);
}

void HaDeckCamera::unmount() {
  this->stop_stream_();
  if (this->timer_ != nullptr)
    lv_timer_delete(this->timer_);
  this->timer_ = nullptr;
  this->root_ = nullptr;
  this->image_ = nullptr;
  this->label_ = nullptr;
  this->shown_ = false;
  this->reset_widget_state_();
}

void HaDeckCamera::apply_theme(HaDeckTheme *theme) {
  if (this->label_ == nullptr || theme == nullptr)
    return;
  lv_obj_set_style_text_color(this->label_, lv_color_white(), MAIN_DEFAULT);
#ifdef USE_LVGL_FONT
  if (theme->text_small_font() != nullptr)
    lv_obj_set_style_text_font(this->label_, theme->text_small_font()->get_lv_font(), MAIN_DEFAULT);
#endif
  this->render_status_(true);
}

void HaDeckCamera::on_shown() {
  this->shown_ = true;
  this->paused_ = false;
  this->sync_stream_();
  this->render_status_(false);
}

void HaDeckCamera::on_hidden() {
  this->shown_ = false;
  this->sync_stream_();
}

void HaDeckCamera::update_state_(bool force) {
  if (this->streaming_ && this->timeout_ > 0 && this->root_ != nullptr &&
      lv_display_get_inactive_time(lv_obj_get_display(this->root_)) >= this->timeout_) {
    ESP_LOGD(TAG, "No interaction for %u ms; pausing stream", static_cast<unsigned>(this->timeout_));
    this->paused_ = true;
  }
  this->sync_stream_();
  this->render_status_(force);
}

void HaDeckCamera::sync_stream_() {
  const bool wanted = this->shown_ && this->root_ != nullptr && this->visible_state_ && !this->paused_;
  if (wanted && !this->streaming_)
    this->start_stream_();
  else if (!wanted && this->streaming_)
    this->stop_stream_();
}

bool HaDeckCamera::allocate_buffers_() {
  if (this->buffers_[0] != nullptr)
    return true;

  const size_t frame_size = static_cast<size_t>(this->width_) * this->height_ * 2;
  RAMAllocator<uint8_t> allocator;
  for (auto &buffer : this->buffers_) {
    buffer = allocator.allocate(frame_size);
    if (buffer == nullptr) {
      ESP_LOGE(TAG, "Unable to allocate %u bytes for a camera frame", static_cast<unsigned>(frame_size));
      return false;
    }
    memset(buffer, 0, frame_size);
  }
  for (auto &slot : this->jpeg_slots_) {
    slot = allocator.allocate(MAX_FRAME_SIZE);
    if (slot == nullptr) {
      ESP_LOGE(TAG, "Unable to allocate camera stream buffers");
      return false;
    }
  }
  RAMAllocator<uint8_t> internal(RAMAllocator<uint8_t>::ALLOC_INTERNAL);
  this->rx_ = internal.allocate(RX_BUFFER_SIZE);
  this->jpeg_ = new JPEGDEC();  // NOLINT(cppcoreguidelines-owning-memory)
  if (this->rx_ == nullptr || this->jpeg_ == nullptr) {
    ESP_LOGE(TAG, "Unable to allocate camera stream buffers");
    return false;
  }

  for (size_t i = 0; i < 3; i++) {
    auto &dsc = this->descriptors_[i];
    dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    dsc.header.cf = LV_COLOR_FORMAT_RGB565;
    dsc.header.w = this->width_;
    dsc.header.h = this->height_;
    dsc.header.stride = this->width_ * 2;
    dsc.data_size = frame_size;
    dsc.data = this->buffers_[i];
  }
  return true;
}

void HaDeckCamera::start_stream_() {
  if (this->host_.empty() || !this->allocate_buffers_())
    return;

  ESP_LOGD(TAG, "Starting stream from %s:%u%s", this->host_.c_str(), this->port_, this->path_.c_str());
  this->streaming_ = true;
  this->has_frame_ = false;
  this->state_ = State::CONNECTING;
  // Frames already queued from an earlier session must not be shown.
  this->session_++;
  {
    std::lock_guard<std::mutex> lock(this->frame_mutex_);
    this->fresh_ = false;
  }
  {
    std::lock_guard<std::mutex> lock(this->control_mutex_);
    this->active_ = true;
  }
  if (!this->worker_.joinable()) {
    // Both threads stay off the ESPHome/LVGL loop core. The receiver runs above
    // the decoder so the socket keeps draining while a frame is being decoded.
    this->worker_ = spawn_thread("ha_deck_cam_rx", 3, [this]() { this->worker_loop_(); });
    this->decoder_ = spawn_thread("ha_deck_cam_dec", 1, [this]() { this->decoder_loop_(); });
  }
  this->control_cv_.notify_all();
}

void HaDeckCamera::stop_stream_() {
  if (!this->streaming_)
    return;
  ESP_LOGD(TAG, "Stopping stream");
  this->streaming_ = false;
  {
    std::lock_guard<std::mutex> lock(this->control_mutex_);
    this->active_ = false;
  }
  // Wake a blocking receive immediately; the worker closes the socket itself.
  const int fd = this->socket_.load();
  if (fd >= 0)
    shutdown(fd, SHUT_RDWR);
  this->control_cv_.notify_all();
}

void HaDeckCamera::timer_callback_(lv_timer_t *timer) {
  auto *camera = static_cast<HaDeckCamera *>(lv_timer_get_user_data(timer));
  camera->present_frame_();
}

void HaDeckCamera::present_frame_() {
  if (this->image_ == nullptr || !this->streaming_)
    return;
  {
    std::lock_guard<std::mutex> lock(this->frame_mutex_);
    if (!this->fresh_)
      return;
    std::swap(this->front_, this->latest_);
    this->fresh_ = false;
  }
  auto *dsc = &this->descriptors_[this->front_];
  lv_image_cache_drop(dsc);
  lv_image_set_src(this->image_, dsc);
  lv_obj_invalidate(this->image_);
  if (!this->has_frame_) {
    this->has_frame_ = true;
    this->render_status_(false);
  }
}

void HaDeckCamera::render_status_(bool force) {
  if (this->label_ == nullptr || this->image_ == nullptr)
    return;
  const State state = this->state_.load();
  if (!force && state == this->rendered_state_ && this->paused_ == this->rendered_paused_ &&
      this->has_frame_ == this->rendered_has_frame_)
    return;
  this->rendered_state_ = state;
  this->rendered_paused_ = this->paused_;
  this->rendered_has_frame_ = this->has_frame_;

  const std::string *text = nullptr;
  if (this->paused_)
    text = &this->paused_text_;
  else if (state == State::ERROR)
    text = &this->error_text_;
  else if (!this->has_frame_)
    text = &this->connecting_text_;

  const bool live = text == nullptr;
  if (live || text->empty()) {
    lv_obj_add_flag(this->label_, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_label_set_text(this->label_, text->c_str());
    lv_obj_remove_flag(this->label_, LV_OBJ_FLAG_HIDDEN);
  }
  if (this->has_frame_)
    lv_obj_remove_flag(this->image_, LV_OBJ_FLAG_HIDDEN);
  else
    lv_obj_add_flag(this->image_, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_style_image_opa(this->image_, live ? static_cast<lv_opa_t>(LV_OPA_COVER) : STALE_FRAME_OPA, MAIN_DEFAULT);
}

void HaDeckCamera::event_callback_(lv_event_t *event) {
  auto *camera = static_cast<HaDeckCamera *>(lv_event_get_user_data(event));
  if (camera->paused_) {
    camera->paused_ = false;
    camera->sync_stream_();
    camera->render_status_(false);
    return;
  }
  camera->click_trigger_.trigger();
}

// ---------------------------------------------------------------------------
// Receiver thread
// ---------------------------------------------------------------------------

void HaDeckCamera::wait_(uint32_t ms) {
  std::unique_lock<std::mutex> lock(this->control_mutex_);
  this->control_cv_.wait_for(lock, std::chrono::milliseconds(ms), [this]() { return !this->active_.load(); });
}

void HaDeckCamera::worker_loop_() {
  size_t failures = 0;
  while (true) {
    {
      std::unique_lock<std::mutex> lock(this->control_mutex_);
      this->control_cv_.wait(lock, [this]() { return this->active_.load(); });
    }
    this->state_ = State::CONNECTING;
    this->run_session_();
    this->close_socket_();
    if (!this->active_)
      continue;

    if (this->state_ == State::STREAMING)
      failures = 0;
    this->state_ = State::ERROR;
    const size_t index = std::min(failures, sizeof(RETRY_DELAYS_MS) / sizeof(RETRY_DELAYS_MS[0]) - 1);
    failures++;
    this->wait_(RETRY_DELAYS_MS[index]);
  }
}

void HaDeckCamera::close_socket_() {
  const int fd = this->socket_.exchange(-1);
  if (fd >= 0)
    close(fd);
}

bool HaDeckCamera::connect_() {
  addrinfo hints{};
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  addrinfo *result = nullptr;
  const std::string port = std::to_string(this->port_);
  if (getaddrinfo(this->host_.c_str(), port.c_str(), &hints, &result) != 0 || result == nullptr) {
    ESP_LOGW(TAG, "Unable to resolve %s", this->host_.c_str());
    return false;
  }

  const int fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
  if (fd < 0) {
    freeaddrinfo(result);
    return false;
  }
  this->socket_ = fd;

  const int flags = fcntl(fd, F_GETFL, 0);
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);
  int rc = connect(fd, result->ai_addr, result->ai_addrlen);
  freeaddrinfo(result);
  if (rc != 0 && errno != EINPROGRESS) {
    ESP_LOGW(TAG, "Unable to connect to %s:%u (errno %d)", this->host_.c_str(), this->port_, errno);
    return false;
  }
  if (rc != 0) {
    const uint32_t started = millis();
    while (true) {
      if (!this->active_)
        return false;
      fd_set writable;
      FD_ZERO(&writable);
      FD_SET(fd, &writable);
      timeval tv{0, static_cast<suseconds_t>(RECEIVE_POLL_MS * 1000)};
      rc = select(fd + 1, nullptr, &writable, nullptr, &tv);
      if (rc > 0)
        break;
      if (rc < 0 || millis() - started >= CONNECT_TIMEOUT_MS) {
        ESP_LOGW(TAG, "Timed out connecting to %s:%u", this->host_.c_str(), this->port_);
        return false;
      }
    }
    int error = 0;
    socklen_t length = sizeof(error);
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &length) != 0 || error != 0) {
      ESP_LOGW(TAG, "Unable to connect to %s:%u (error %d)", this->host_.c_str(), this->port_, error);
      return false;
    }
  }
  fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);

  timeval tv{0, static_cast<suseconds_t>(RECEIVE_POLL_MS * 1000)};
  setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
  const int one = 1;
  setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

  // HTTP/1.0 keeps the response unchunked: the body is the raw multipart stream.
  std::string request = "GET " + this->path_ + " HTTP/1.0\r\nHost: " + this->host_ + ":" + port +
                        "\r\nUser-Agent: ha_deck\r\nAccept: multipart/x-mixed-replace\r\n\r\n";
  size_t sent = 0;
  while (sent < request.size()) {
    const ssize_t n = send(fd, request.data() + sent, request.size() - sent, 0);
    if (n <= 0)
      return false;
    sent += static_cast<size_t>(n);
  }
  this->rx_pos_ = 0;
  this->rx_len_ = 0;
  this->last_data_at_ = millis();
  return true;
}

bool HaDeckCamera::fill_() {
  while (this->active_) {
    const int fd = this->socket_.load();
    if (fd < 0)
      return false;
    const ssize_t n = recv(fd, this->rx_, RX_BUFFER_SIZE, 0);
    if (n > 0) {
      this->rx_pos_ = 0;
      this->rx_len_ = static_cast<size_t>(n);
      this->last_data_at_ = millis();
      return true;
    }
    if (n == 0)
      return false;  // server closed the stream
    if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
      return false;
    if (millis() - this->last_data_at_ >= STALL_TIMEOUT_MS) {
      ESP_LOGW(TAG, "Stream stalled");
      return false;
    }
  }
  return false;
}

bool HaDeckCamera::read_line_(std::string &line) {
  line.clear();
  while (true) {
    if (this->rx_pos_ >= this->rx_len_ && !this->fill_())
      return false;
    const char c = static_cast<char>(this->rx_[this->rx_pos_++]);
    if (c == '\n') {
      if (!line.empty() && line.back() == '\r')
        line.pop_back();
      return true;
    }
    if (line.size() >= MAX_LINE_LENGTH)
      return false;
    line.push_back(c);
  }
}

bool HaDeckCamera::read_exact_(uint8_t *dst, size_t size) {
  while (size > 0) {
    if (this->rx_pos_ >= this->rx_len_ && !this->fill_())
      return false;
    const size_t chunk = std::min(size, this->rx_len_ - this->rx_pos_);
    if (dst != nullptr) {
      memcpy(dst, this->rx_ + this->rx_pos_, chunk);
      dst += chunk;
    }
    this->rx_pos_ += chunk;
    size -= chunk;
  }
  return true;
}

static bool starts_with_ci(const std::string &value, const char *prefix) {
  const size_t length = strlen(prefix);
  return value.size() >= length && strncasecmp(value.c_str(), prefix, length) == 0;
}

void HaDeckCamera::run_session_() {
  if (!this->connect_())
    return;

  std::string line;
  if (!this->read_line_(line))
    return;
  const size_t space = line.find(' ');
  const int status = space == std::string::npos ? 0 : atoi(line.c_str() + space + 1);
  if (!starts_with_ci(line, "HTTP/") || status != 200) {
    ESP_LOGW(TAG, "Unexpected HTTP response: %s", line.c_str());
    return;
  }
  bool multipart = false;
  while (true) {
    if (!this->read_line_(line))
      return;
    if (line.empty())
      break;
    if (starts_with_ci(line, "Content-Type:") && line.find("multipart/") != std::string::npos)
      multipart = true;
  }
  if (!multipart) {
    ESP_LOGW(TAG, "Response is not a multipart MJPEG stream");
    return;
  }
  ESP_LOGD(TAG, "Connected");

  while (this->active_) {
    // Part headers follow the boundary line; only Content-Length matters.
    size_t length = 0;
    bool in_headers = false;
    while (true) {
      if (!this->read_line_(line))
        return;
      if (!in_headers) {
        if (line.size() >= 2 && line[0] == '-' && line[1] == '-')
          in_headers = true;
        continue;
      }
      if (line.empty())
        break;
      if (starts_with_ci(line, "Content-Length:"))
        length = static_cast<size_t>(strtoul(line.c_str() + 15, nullptr, 10));
    }
    if (length == 0 || length > MAX_FRAME_SIZE) {
      ESP_LOGW(TAG, "Unsupported frame length %u", static_cast<unsigned>(length));
      return;
    }
    if (!this->read_exact_(this->jpeg_slots_[this->jpeg_rx_], length))
      return;
    this->publish_jpeg_(length);
  }
}

void HaDeckCamera::publish_jpeg_(size_t size) {
  {
    std::lock_guard<std::mutex> lock(this->jpeg_mutex_);
    this->jpeg_sizes_[this->jpeg_rx_] = size;
    this->jpeg_sessions_[this->jpeg_rx_] = this->session_.load();
    // An undecoded JPEG in the latest slot is replaced: only the newest counts.
    if (this->jpeg_fresh_)
      this->jpeg_overwritten_++;
    this->jpeg_received_++;
    std::swap(this->jpeg_rx_, this->jpeg_latest_);
    this->jpeg_fresh_ = true;
  }
  this->jpeg_cv_.notify_one();
}

// ---------------------------------------------------------------------------
// Decoder thread
// ---------------------------------------------------------------------------

void HaDeckCamera::decoder_loop_() {
  uint32_t stats_started = millis();
  uint32_t decoded = 0;
  uint32_t decode_us = 0;
  uint32_t copy_us = 0;
  while (true) {
    size_t size = 0;
    uint32_t session = 0;
    {
      std::unique_lock<std::mutex> lock(this->jpeg_mutex_);
      this->jpeg_cv_.wait_for(lock, std::chrono::milliseconds(1000), [this]() { return this->jpeg_fresh_; });
      if (this->jpeg_fresh_) {
        std::swap(this->jpeg_decode_, this->jpeg_latest_);
        this->jpeg_fresh_ = false;
        size = this->jpeg_sizes_[this->jpeg_decode_];
        session = this->jpeg_sessions_[this->jpeg_decode_];
      }
    }

    if (size > 0 && this->active_ && session == this->session_.load()) {
      const uint32_t started = micros();
      this->copy_us_ = 0;
      if (this->decode_frame_(this->jpeg_slots_[this->jpeg_decode_], size)) {
        decode_us += micros() - started;
        copy_us += this->copy_us_;
        decoded++;
        this->state_ = State::STREAMING;
      }
    }

    const uint32_t elapsed = millis() - stats_started;
    if (elapsed >= STATS_INTERVAL_MS) {
      uint32_t received;
      uint32_t overwritten;
      {
        std::lock_guard<std::mutex> lock(this->jpeg_mutex_);
        received = this->jpeg_received_;
        overwritten = this->jpeg_overwritten_;
        this->jpeg_received_ = this->jpeg_overwritten_ = 0;
      }
      if (received > 0) {
        // Copy time is the part of a decode spent writing pixels into the frame buffer.
        ESP_LOGD(TAG, "%.1f fps decoded, %.1f fps received, %u dropped, %.1f ms/decode (%.1f ms copy)",
                 decoded * 1000.0f / elapsed, received * 1000.0f / elapsed, static_cast<unsigned>(overwritten),
                 decoded > 0 ? decode_us / 1000.0f / decoded : 0.0f, decoded > 0 ? copy_us / 1000.0f / decoded : 0.0f);
      }
      stats_started = millis();
      decoded = decode_us = copy_us = 0;
    }
  }
}

int HaDeckCamera::draw_callback_(JPEGDRAW *draw) {
  auto *camera = static_cast<HaDeckCamera *>(draw->pUser);
  const uint32_t started = micros();
  const int width = camera->width_;
  const int height = camera->height_;
  const int dst_x = draw->x + camera->draw_offset_x_;
  const int x0 = std::max(0, dst_x);
  const int x1 = std::min(width, dst_x + draw->iWidthUsed);
  if (x0 >= x1)
    return 1;
  auto *target = reinterpret_cast<uint16_t *>(camera->buffers_[camera->back_]);
  for (int row = 0; row < draw->iHeight; row++) {
    const int dst_y = draw->y + camera->draw_offset_y_ + row;
    if (dst_y < 0 || dst_y >= height)
      continue;
    memcpy(target + dst_y * width + x0, draw->pPixels + row * draw->iWidth + (x0 - dst_x),
           static_cast<size_t>(x1 - x0) * sizeof(uint16_t));
  }
  camera->copy_us_ += micros() - started;
  return 1;
}

bool HaDeckCamera::decode_frame_(const uint8_t *data, size_t size) {
  if (!this->jpeg_->openRAM(const_cast<uint8_t *>(data), static_cast<int>(size), HaDeckCamera::draw_callback_)) {
    ESP_LOGW(TAG, "Invalid JPEG frame (error %d)", this->jpeg_->getLastError());
    return false;
  }
  const int frame_width = this->jpeg_->getWidth();
  const int frame_height = this->jpeg_->getHeight();
  // Center the frame: smaller frames are letterboxed, larger ones cropped.
  this->draw_offset_x_ = static_cast<int16_t>((this->width_ - frame_width) / 2);
  this->draw_offset_y_ = static_cast<int16_t>((this->height_ - frame_height) / 2);
  const uint8_t back = this->back_;
  if (this->buffer_frame_width_[back] != frame_width || this->buffer_frame_height_[back] != frame_height) {
    if (frame_width < this->width_ || frame_height < this->height_)
      memset(this->buffers_[back], 0, static_cast<size_t>(this->width_) * this->height_ * 2);
    this->buffer_frame_width_[back] = static_cast<uint16_t>(frame_width);
    this->buffer_frame_height_[back] = static_cast<uint16_t>(frame_height);
  }
  this->jpeg_->setUserPointer(this);
  this->jpeg_->setPixelType(RGB565_LITTLE_ENDIAN);
  const bool ok = this->jpeg_->decode(0, 0, 0) != 0;
  this->jpeg_->close();
  if (!ok) {
    ESP_LOGW(TAG, "Unable to decode JPEG frame (error %d)", this->jpeg_->getLastError());
    return false;
  }

  std::lock_guard<std::mutex> lock(this->frame_mutex_);
  std::swap(this->back_, this->latest_);
  this->fresh_ = true;
  return true;
}

}  // namespace esphome::ha_deck

#endif  // USE_HA_DECK_CAMERA
