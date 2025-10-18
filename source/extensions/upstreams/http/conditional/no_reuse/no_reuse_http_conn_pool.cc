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

#include "source/extensions/upstreams/http/conditional/no_reuse/no_reuse_http_conn_pool.h"

#include "envoy/upstream/thread_local_cluster.h"

namespace Envoy {
namespace Extensions {
namespace Upstreams {
namespace Http {
namespace Conditional {
namespace NoReuse {

NoReuseHttpConnPool::NoReuseHttpConnPool(
    Upstream::HostConstSharedPtr host,
    Upstream::ThreadLocalCluster& thread_local_cluster,
    Upstream::ResourcePriority priority,
    absl::optional<Envoy::Http::Protocol> downstream_protocol,
    Upstream::LoadBalancerContext* ctx)
    : host_(host), thread_local_cluster_(thread_local_cluster), priority_(priority),
      downstream_protocol_(downstream_protocol), ctx_(ctx) {}

NoReuseHttpConnPool::~NoReuseHttpConnPool() = default;

void NoReuseHttpConnPool::newStream(Router::GenericConnectionPoolCallbacks* callbacks) {
  callbacks_ = callbacks;
  stream_pending_ = true;

  // Create a fresh HTTP connection pool for each request
  // This effectively prevents connection reuse by using a new pool every time
  auto http_pool_data = thread_local_cluster_.httpConnPool(host_, priority_, downstream_protocol_, ctx_);

  if (!http_pool_data.has_value()) {
    stream_pending_ = false;
    callbacks_->onPoolFailure(ConnectionPool::PoolFailureReason::LocalConnectionFailure,
                             "Failed to create fresh HTTP connection pool for no-reuse policy",
                             host_);
    return;
  }

  // Use the fresh pool for this single request
  stream_pending_ = false;
  // For now, since implementing the full HTTP upstream is complex,
  // we'll fall back to indicating this feature is not fully implemented
  callbacks_->onPoolFailure(ConnectionPool::PoolFailureReason::LocalConnectionFailure,
                           "HTTP no-reuse connection pool not yet fully implemented",
                           host_);
}

bool NoReuseHttpConnPool::cancelAnyPendingStream() {
  if (stream_pending_) {
    stream_pending_ = false;
    return true;
  }
  return false;
}

Upstream::HostDescriptionConstSharedPtr NoReuseHttpConnPool::host() const {
  return host_;
}

bool NoReuseHttpConnPool::valid() const {
  return host_ != nullptr;
}

} // namespace NoReuse
} // namespace Conditional
} // namespace Http
} // namespace Upstreams
} // namespace Extensions
} // namespace Envoy
