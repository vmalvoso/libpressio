#include <set>
#include <string>
#include <algorithm>
#include <iterator>
#include <sstream>
#include "libpressio_ext/cpp/compressor.h"
#include "libpressio_ext/cpp/metrics.h"
#include "libpressio_ext/cpp/options.h"

#include "libpressio_ext/cpp/pressio.h"
#include "pressio_options_iter.h"
#include "pressio_options.h"

libpressio_compressor_plugin::libpressio_compressor_plugin() noexcept :
  pressio_configurable(),
  pressio_errorable(),
  metrics_plugin(metrics_plugins().build("noop")),
  metrics_id("noop")
{}

libpressio_compressor_plugin::~libpressio_compressor_plugin()=default;


namespace {
  std::set<std::string> get_keys(struct pressio_options const& options, std::string const& prefix) {
    std::set<std::string> keys;
    for (auto const& option : options) {
      if(option.first.find(prefix) == 0)
        keys.emplace(option.first);
    }
    return keys;
  }
}

int libpressio_compressor_plugin::check_options(struct pressio_options const& options) {
  clear_error();

  if(metrics_plugin)
    metrics_plugin->begin_check_options(&options);

  struct pressio_options my_options = get_options();
  auto my_keys = get_keys(my_options, prefix());
  auto keys = get_keys(options, prefix());
  std::set<std::string> extra_keys;
  std::set_difference(
      std::begin(keys), std::end(keys),
      std::begin(my_keys), std::end(my_keys),
      std::inserter(extra_keys, std::begin(extra_keys))
  );
  if(!extra_keys.empty()) {
    std::stringstream ss;
    ss << "extra keys: ";

    std::copy(std::begin(extra_keys), std::end(extra_keys), std::ostream_iterator<std::string>(ss, " "));
    set_error(1, ss.str());
    return 1;
  }

  auto ret =  check_options_impl(options);
  if(metrics_plugin)
    metrics_plugin->end_check_options(&options, ret);

  return ret;
}

struct pressio_options libpressio_compressor_plugin::get_configuration() const {
  if(metrics_plugin)
    metrics_plugin->begin_get_configuration();
  auto ret = get_configuration_impl();
  if(metrics_plugin) { 
    ret.copy_from(metrics_plugin->get_configuration());
    metrics_plugin->end_get_configuration(ret);
  }
  return ret;
}

struct pressio_options libpressio_compressor_plugin::get_documentation() const {
  if(metrics_plugin)
    metrics_plugin->begin_get_documentation();
  auto ret = get_documentation_impl();
  set(ret, "pressio:thread_safe", "level of thread safety provided by the compressor");
  set(ret, "pressio:stability", "level of stablity provided by the compressor; see the README for libpressio");
  if(metrics_plugin) { 
    ret.copy_from(metrics_plugin->get_documentation());
    set_meta_docs(ret, get_metrics_key_name(), "metrics to collect when using the compressor", metrics_plugin);
    metrics_plugin->end_get_documentation(ret);
  }
  return ret;
}

struct pressio_options libpressio_compressor_plugin::get_options() const {
  if(metrics_plugin)
    metrics_plugin->begin_get_options();
  pressio_options opts;
  set_meta(opts, get_metrics_key_name(), metrics_id, metrics_plugin);
  set(opts, "metrics:errors_fatal", metrics_errors_fatal);
  set(opts, "metrics:copy_compressor_results", metrics_copy_impl_results);
  opts.copy_from(get_options_impl());
  if(metrics_plugin)
    metrics_plugin->end_get_options(&opts);
  return opts;
}

int libpressio_compressor_plugin::set_options(struct pressio_options const& options) {
  clear_error();
  if(metrics_plugin) {
    if(metrics_plugin->begin_set_options(options) != 0 && metrics_errors_fatal) {
      set_error(metrics_plugin->error_code(), metrics_plugin->error_msg());
      return error_code();
    }
  }
  get_meta(options, get_metrics_key_name(), metrics_plugins(), metrics_id, metrics_plugin);
  get(options, "metrics:errors_fatal", &metrics_errors_fatal);
  get(options, "metrics:copy_compressor_results", &metrics_copy_impl_results);
  auto ret = set_options_impl(options);
  if(metrics_plugin) {
    if(metrics_plugin->end_set_options(options, ret) != 0 && metrics_errors_fatal) {
      set_error(metrics_plugin->error_code(), metrics_plugin->error_msg());
      return error_code();
    }
  }
  return ret;
}

int libpressio_compressor_plugin::compress(const pressio_data *input, struct pressio_data* output) {
  clear_error();
  if(metrics_plugin) {
    if(metrics_plugin->begin_compress(input, output) != 0 && metrics_errors_fatal) {
      set_error(metrics_plugin->error_code(), metrics_plugin->error_msg());
      return error_code();
    }
  }
  auto ret = compress_impl(input, output);
  if(metrics_plugin) {
    if(metrics_plugin->end_compress(input, output, ret) != 0 && metrics_errors_fatal) {
      set_error(metrics_plugin->error_code(), metrics_plugin->error_msg());
      return error_code();
    }
  }
  return ret;
}

int libpressio_compressor_plugin::decompress(const pressio_data *input, struct pressio_data* output) {
  clear_error();
  if(metrics_plugin)
    metrics_plugin->begin_decompress(input, output);
  auto ret = decompress_impl(input, output);
  if(metrics_plugin)
    metrics_plugin->end_decompress(input, output, ret);
  return ret;
}

int libpressio_compressor_plugin::check_options_impl(struct pressio_options const &) { return 0;}


struct pressio_options libpressio_compressor_plugin::get_metrics_results() const {
  pressio_options results_impl = get_metrics_results_impl();
  pressio_options results;
  if(metrics_copy_impl_results) {
    results.copy_from(results_impl);
  }
  if(metrics_plugin) {
    results.copy_from(metrics_plugin->get_metrics_results(results_impl));
  }
  return results;
}

struct pressio_metrics libpressio_compressor_plugin::get_metrics() const {
  return metrics_plugin;
}

void libpressio_compressor_plugin::set_metrics(pressio_metrics& plugin) {
  metrics_plugin = plugin;
  if(plugin) {
    metrics_id = metrics_plugin->prefix();
    if(not get_name().empty()) {
      metrics_plugin->set_name(get_name() + "/" + metrics_plugin->prefix());
    }
  } else {
    metrics_id = "";
  }
}

struct pressio_options libpressio_compressor_plugin::get_metrics_options() const {
  return metrics_plugin->get_options();
}

int libpressio_compressor_plugin::set_metrics_options(struct pressio_options const& options) {
  clear_error();
  return metrics_plugin->set_options(options);
}

struct pressio_options libpressio_compressor_plugin::get_metrics_results_impl() const {
  return {};
}

int libpressio_compressor_plugin::compress_many_impl(compat::span<const pressio_data* const> const& inputs, compat::span<pressio_data*> & outputs) {
    //default returns an error to indicate the option is unsupported;
    if(inputs.size() == 1 && outputs.size() == 1) {
      return compress_impl(inputs.front(), outputs.front());
    } else 
    return set_error(1, "decompress_many not supported");
  }

int libpressio_compressor_plugin::decompress_many_impl(compat::span<const pressio_data* const> const& inputs, compat::span<pressio_data* >& outputs) {
    //default returns an error to indicate the option is unsupported;
    if(inputs.size() == 1 && outputs.size() == 1) {
      return decompress_impl(inputs.front(), outputs.front());
    } else 
    return set_error(1, "decompress_many not supported");
  }


void libpressio_compressor_plugin::set_name(std::string const& new_name) {
    pressio_configurable::set_name(new_name);
    metrics_plugin->set_name(new_name + "/" + metrics_plugin->prefix());
}
