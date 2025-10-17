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

#pragma once

#include "source/extensions/upstreams/http/conditional/config.pb.h"
#include "source/extensions/upstreams/http/http/upstream_request.h"
#include "source/extensions/upstreams/http/tcp/upstream_request.h"
#include "source/extensions/upstreams/http/udp/upstream_request.h"
#include "envoy/router/router.h"

namespace Envoy {
namespace Extensions {
namespace Upstreams {
namespace Http {
namespace Conditional {

/**
 * Config registration for the HttpConditionalConnPool. @see Router::GenericConnPoolFactory
 */
class HttpConditionalConnPoolFactory : public Router::GenericConnPoolFactory {
public:
  std::string name() const override { return "istio.filters.connection_pools.http.conditional"; }
  std::string category() const override { return "envoy.upstreams"; }
  Router::GenericConnPoolPtr createGenericConnPool(
      Upstream::HostConstSharedPtr host, Upstream::ThreadLocalCluster& thread_local_cluster,
      Router::GenericConnPoolFactory::UpstreamProtocol upstream_protocol,
      Upstream::ResourcePriority priority,
      absl::optional<Envoy::Http::Protocol> downstream_protocol, Upstream::LoadBalancerContext* ctx,
      const Protobuf::Message&) const override;
  Router::GenericConnPoolPtr defaultGenericConnPool(
      Upstream::HostConstSharedPtr host, Upstream::ThreadLocalCluster& thread_local_cluster,
      Router::GenericConnPoolFactory::UpstreamProtocol upstream_protocol,
      Upstream::ResourcePriority priority,
      absl::optional<Envoy::Http::Protocol> downstream_protocol,
      Upstream::LoadBalancerContext* ctx) const {
    Router::GenericConnPoolPtr conn_pool;
    switch (upstream_protocol) {
    case UpstreamProtocol::HTTP:
      conn_pool = std::make_unique<Upstreams::Http::Http::HttpConnPool>(
          host, thread_local_cluster, priority, downstream_protocol, ctx);
      return (conn_pool->valid() ? std::move(conn_pool) : nullptr);
    case UpstreamProtocol::TCP:
      conn_pool = std::make_unique<Upstreams::Http::Tcp::TcpConnPool>(host, thread_local_cluster,
                                                                      priority, ctx);
      return (conn_pool->valid() ? std::move(conn_pool) : nullptr);
    case UpstreamProtocol::UDP:
      conn_pool = std::make_unique<Upstreams::Http::Udp::UdpConnPool>(host);
      return (conn_pool->valid() ? std::move(conn_pool) : nullptr);
    }

    return nullptr;
  }
  ProtobufTypes::MessagePtr createEmptyConfigProto() override {
    return std::make_unique<istio::envoy::upstreams::http::conditional::ConditionalUpstream>();
  }
};

DECLARE_FACTORY(HttpConditionalConnPoolFactory);

} // namespace Conditional
} // namespace Http
} // namespace Upstreams
} // namespace Extensions
} // namespace Envoy
