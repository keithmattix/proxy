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

#include "envoy/router/router.h"
#include "envoy/upstream/upstream.h"

namespace Envoy {
namespace Extensions {
namespace Upstreams {
namespace Http {
namespace Conditional {
namespace NoReuse {

/**
 * An HTTP connection pool that never reuses connections. Each request
 * gets a fresh connection which is immediately closed after use.
 *
 * This implements Router::GenericConnPool interface and creates
 * fresh HTTP connection pools for each request, similar to the TCP approach.
 */
class NoReuseHttpConnPool : public Router::GenericConnPool {
public:
  NoReuseHttpConnPool(
      Upstream::HostConstSharedPtr host,
      Upstream::ThreadLocalCluster& thread_local_cluster,
      Upstream::ResourcePriority priority,
      absl::optional<Envoy::Http::Protocol> downstream_protocol,
      Upstream::LoadBalancerContext* ctx);

  ~NoReuseHttpConnPool() override;

  // Router::GenericConnPool
  void newStream(Router::GenericConnectionPoolCallbacks* callbacks) override;
  bool cancelAnyPendingStream() override;
  Upstream::HostDescriptionConstSharedPtr host() const override;
  bool valid() const override;

private:
  Upstream::HostConstSharedPtr host_;
  Upstream::ThreadLocalCluster& thread_local_cluster_;
  Upstream::ResourcePriority priority_;
  absl::optional<Envoy::Http::Protocol> downstream_protocol_;
  Upstream::LoadBalancerContext* ctx_;
  Router::GenericConnectionPoolCallbacks* callbacks_{nullptr};
  bool stream_pending_{false};
};

} // namespace NoReuse
} // namespace Conditional
} // namespace Http
} // namespace Upstreams
} // namespace Extensions
} // namespace Envoy
