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

#include "source/extensions/upstreams/http/conditional/upstream_request.h"


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

  // Access the server factory context through the singleton
  auto server_context =
      Server::Configuration::ServerFactoryContextInstance::getExisting();

  const auto matcher = std::make_unique<Envoy::Matchers::MetadataMatcher>(conditional_config.metadata_matcher(), *server_context);

  if (matcher->match(*host->metadata())) {
    // Matched; now check the policy
    switch (conditional_config.policy()) {
    case istio::envoy::upstreams::http::conditional::ConditionalUpstream_Policy::ConditionalUpstream_Policy_ALWAYS_CREATE_NEW_CONNECTION:
    case istio::envoy::upstreams::http::conditional::ConditionalUpstream_Policy::ConditionalUpstream_Policy_UNSET:
      return defaultGenericConnPool(host, thread_local_cluster, upstream_protocol, priority,
                                    downstream_protocol, ctx);
    default:
      PANIC("not implemented");
    }
  }

  // No match, use default behavior
  return defaultGenericConnPool(host, thread_local_cluster, upstream_protocol, priority,
                                downstream_protocol,  ctx);
}

} // namespace Conditional
} // namespace Http
} // namespace Upstreams
} // namespace Extensions
} // namespace Envoy
