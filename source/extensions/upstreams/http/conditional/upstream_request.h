#pragma once

#include "source/extensions/upstreams/http/conditional/config.pb.h"
#include "envoy/http/conn_pool.h"
#include "envoy/upstream/thread_local_cluster.h"

namespace Envoy {
namespace Extensions {
namespace Upstreams {
namespace Http {
namespace Conditional {
class ConditionalGenericHttpConnPool : public Envoy::Router::GenericConnPool,
                                public Envoy::Http::ConnectionPool::Callbacks {
public:
  ConditionalGenericHttpConnPool(Envoy::Upstream::HostConstSharedPtr host,
                          Envoy::Upstream::ThreadLocalCluster& thread_local_cluster,
                          Envoy::Upstream::Proto
                          Envoy::Upstream::ResourcePriority priority,
                          absl::optional<Envoy::Http::Protocol> downstream_protocol,
                          Envoy::Upstream::LoadBalancerContext* ctx,
                          istio::envoy::upstreams::http::conditional::ConditionalUpstream& config
                          ) {
                            auto ret = std::make_unique<HttpConnPool>(host, thread_local_cluster, priority,
                                            downstream_protocol, ctx);
                            return (ret->valid() ? std::move(ret) : nullptr);
                          }
};
} // namespace Conditional
} // namespace Http
} // namespace Upstreams
} // namespace Extensions
} // namespace Envoy
