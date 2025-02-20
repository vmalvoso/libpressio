#include <chrono>
#include "pressio_options.h"
#include "pressio_compressor.h"
#include "libpressio_ext/cpp/metrics.h"
#include "libpressio_ext/cpp/options.h"
#include "libpressio_ext/cpp/pressio.h"
#include "std_compat/memory.h"

using std::chrono::high_resolution_clock;
using std::chrono::time_point;
using std::chrono::duration_cast;
using std::chrono::milliseconds;

namespace time_metrics{
  struct time_range{
    time_point<high_resolution_clock> begin;
    time_point<high_resolution_clock> end;
    unsigned int elapsed() const { return duration_cast<milliseconds>(end-begin).count(); }
  };
  using timer = compat::optional<time_range>;
}

class time_plugin : public libpressio_metrics_plugin {
  public:
    int begin_check_options_impl(struct pressio_options const* ) override {
    check_options = time_metrics::time_range();
    check_options->begin = high_resolution_clock::now();
    return 0;
  }

  int end_check_options_impl(struct pressio_options const*, int ) override {
    check_options->end = high_resolution_clock::now();
    return 0;
  }

  int begin_get_options_impl() override {
    get_options = time_metrics::time_range();
    get_options->begin = high_resolution_clock::now();
    return 0;
  }

  int end_get_options_impl(struct pressio_options const* ) override {
    get_options->end = high_resolution_clock::now();
    return 0;
  }

  int begin_get_configuration_impl() override {
    get_configuration_tracker = time_metrics::time_range();
    get_configuration_tracker->begin = high_resolution_clock::now();
    return 0;
  }

  int end_get_configuration_impl(struct pressio_options const& ) override {
    get_configuration_tracker->end = high_resolution_clock::now();
    return 0;
  }

  int begin_set_options_impl(struct pressio_options const& ) override {
    set_options = time_metrics::time_range();
    set_options->begin = high_resolution_clock::now();
    return 0;
  }

  int end_set_options_impl(struct pressio_options const& , int ) override {
    set_options->end = high_resolution_clock::now();
    return 0;
  }

  int begin_compress_impl(const struct pressio_data * , struct pressio_data const * ) override {
    compress = time_metrics::time_range();
    compress->begin = high_resolution_clock::now();
    return 0;
  }

  int end_compress_impl(struct pressio_data const* , pressio_data const * , int ) override {
    compress->end = high_resolution_clock::now();
    return 0;
  }

  int begin_decompress_impl(struct pressio_data const* , pressio_data const* ) override {
    decompress = time_metrics::time_range();
    decompress->begin = high_resolution_clock::now();
    return 0;
  }

  int end_decompress_impl(struct pressio_data const* , pressio_data const* , int ) override {
    decompress->end = high_resolution_clock::now();
    return 0;
  }

  int begin_compress_many_impl(compat::span<const pressio_data* const> const&,
                                   compat::span<const pressio_data* const> const&) override {
    compress_many = time_metrics::time_range();
    compress_many->begin = high_resolution_clock::now();
    return 0;
  }

  int end_compress_many_impl(compat::span<const pressio_data* const> const& ,
                                   compat::span<const pressio_data* const> const& , int ) override {
    compress_many->end = high_resolution_clock::now();
    return 0;
   
  }

  int begin_decompress_many_impl(compat::span<const pressio_data* const> const& ,
                                   compat::span<const pressio_data* const> const& ) override {
    decompress_many = time_metrics::time_range();
    decompress_many->begin = high_resolution_clock::now();
    return 0;
  }

  int end_decompress_many_impl(compat::span<const pressio_data* const> const& ,
                                   compat::span<const pressio_data* const> const& , int ) override {
    decompress_many->end = high_resolution_clock::now();
    return 0;
  }

  struct pressio_options get_configuration() const override {
    pressio_options opts;
    set(opts, "pressio:stability", "stable");
    set(opts, "pressio:thread_safe", static_cast<int32_t>(pressio_thread_safety_multiple));
    return opts;
  }

  struct pressio_options get_documentation_impl() const override {
    pressio_options opts;
    set(opts, "pressio:description", "records time used in each operation");
    set(opts, "time:check_options", "time in check_options");
    set(opts, "time:set_options", "time in set options");
    set(opts, "time:get_options", "time in get options");
    set(opts, "time:compress", "time in compress");
    set(opts, "time:decompress", "time in decompress");
    set(opts, "time:compress_many", "time in compress_many");
    set(opts, "time:decompress_many", "time in decompress_many");
    return opts;
  }

  pressio_options get_metrics_results(pressio_options const &)  override {
    struct pressio_options opt;

    auto set_or = [&opt, this](const char* key, time_metrics::timer time) {
      if(time) {
        set(opt, key, time->elapsed());
      } else {
        set_type(opt, key, pressio_option_uint32_type);
      }
    };

    set_or("time:check_options", check_options);
    set_or("time:set_options", set_options);
    set_or("time:get_options", get_options);
    set_or("time:compress", compress);
    set_or("time:decompress", decompress);
    set_or("time:compress_many", compress_many);
    set_or("time:decompress_many", decompress_many);

    return opt;
  }

  std::unique_ptr<libpressio_metrics_plugin> clone() override {
    return compat::make_unique<time_plugin>(*this);
  }

  const char* prefix() const override {
    return "time";
  }

  private:
  time_metrics::timer check_options;
  time_metrics::timer set_options;
  time_metrics::timer get_options;
  time_metrics::timer get_configuration_tracker;
  time_metrics::timer compress;
  time_metrics::timer compress_many;
  time_metrics::timer decompress;
  time_metrics::timer decompress_many;
};

static pressio_register metrics_time_plugin(metrics_plugins(), "time", [](){ return compat::make_unique<time_plugin>(); });
