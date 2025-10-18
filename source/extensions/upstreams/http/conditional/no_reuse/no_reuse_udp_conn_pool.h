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

#include "source/extensions/upstreams/http/conditional/no_reuse/no_reuse_conn_pool_wrapper.h"
#include "source/extensions/upstreams/http/udp/upstream_request.h"

namespace Envoy {
namespace Extensions {
namespace Upstreams {
namespace Http {
namespace Conditional {
namespace NoReuse {

/**
 * UDP-specific no-reuse connection pool.
 * Since UDP is connectionless, this implementation ensures that UDP sockets
 * are closed immediately after each use and new sockets are created for each request.
 *
 * NOTE: This is a simplified stub implementation. A full UDP connection pool would require
 * more complex handling of UDP sockets and may not be needed for typical HTTP use cases.
 */
class NoReuseUdpConnPool : public Router::GenericConnPool {
public:
  NoReuseUdpConnPool(Upstream::HostConstSharedPtr host);

  ~NoReuseUdpConnPool() override;

  // Router::GenericConnPool
  void newStream(Router::GenericConnectionPoolCallbacks* callbacks) override;
  bool cancelAnyPendingStream() override;
  Upstream::HostDescriptionConstSharedPtr host() const override;
  bool valid() const override;

private:
  Upstream::HostConstSharedPtr host_;
  bool stream_pending_{false};
};

} // namespace NoReuse
} // namespace Conditional
} // namespace Http
} // namespace Upstreams
} // namespace Extensions
} // namespace Envoy
