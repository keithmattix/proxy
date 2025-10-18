/* Copyright 2025 Istio Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "source/extensions/upstreams/http/conditional/config.h"

#include "source/common/config/metadata.h"

namespace Envoy {
namespace Extensions {
namespace Upstreams {
namespace Http {
namespace Conditional {
using UpstreamProtocol = Envoy::Router::GenericConnPoolFactory::UpstreamProtocol;

Router::GenericConnPoolPtr HttpConditionalConnPoolFactory::createGenericConnPool(
    Upstream::HostConstSharedPtr host, Upstream::ThreadLocalCluster& thread_local_cluster,
    Router::GenericConnPoolFactory::UpstreamProtocol upstream_protocol,
    Upstream::ResourcePriority priority, absl::optional<Envoy::Http::Protocol> downstream_protocol,
    Upstream::LoadBalancerContext* ctx, const Protobuf::Message& config) const {
  const auto& conditional_config =
      dynamic_cast<const istio::envoy::upstreams::http::conditional::ConditionalUpstream&>(config);

  const auto key = std::make_unique<Envoy::Config::MetadataKey>(conditional_config.key());
  const auto expected_value = conditional_config.value();
  const auto actual_value = Envoy::Config::Metadata::metadataValue(host->metadata().get(), *key);
  if (!actual_value.has_string_value()) {
    ENVOY_LOG(trace,
              "Metadata value for key {} and path {} not found or not a string. Proceeding with "
              "default behavior",
              key->key_, key->path_);
    return defaultGenericConnPool(host, thread_local_cluster, upstream_protocol, priority,
                                  downstream_protocol, ctx);
  }
  if (actual_value.string_value() != expected_value) {
    ENVOY_LOG(
        trace,
        "Metadata value '{}' does not match expected value '{}'. Proceeding with default behavior",
        actual_value.string_value(), expected_value);
    return defaultGenericConnPool(host, thread_local_cluster, upstream_protocol, priority,
                                  downstream_protocol, ctx);
  }
  ENVOY_LOG(debug, "Metadata value '{}' matches expected value '{}'. Applying conditional policy",
            actual_value.string_value(), expected_value);
  // Matched; now check the policy
  switch (conditional_config.policy()) {
  case istio::envoy::upstreams::http::conditional::ConditionalUpstream_Policy::
      ConditionalUpstream_Policy_ALWAYS_CREATE_NEW_CONNECTION:
    ENVOY_LOG(info, "Creating new connection pool as per ALWAYS_CREATE_NEW_CONNECTION policy");
    return defaultGenericConnPool(host, thread_local_cluster, upstream_protocol, priority,
                                  downstream_protocol, ctx); // TODO
  case istio::envoy::upstreams::http::conditional::ConditionalUpstream_Policy::
      ConditionalUpstream_Policy_UNSET:
    return defaultGenericConnPool(host, thread_local_cluster, upstream_protocol, priority,
                                  downstream_protocol, ctx);
  default:
    PANIC("not implemented");
  }
}

REGISTER_FACTORY(HttpConditionalConnPoolFactory, Router::GenericConnPoolFactory);
} // namespace Conditional
} // namespace Http
} // namespace Upstreams
} // namespace Extensions
} // namespace Envoy
