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

#include "source/extensions/upstreams/http/conditional/no_reuse/no_reuse_udp_conn_pool.h"

namespace Envoy {
namespace Extensions {
namespace Upstreams {
namespace Http {
namespace Conditional {
namespace NoReuse {

NoReuseUdpConnPool::NoReuseUdpConnPool(Upstream::HostConstSharedPtr host) : host_(host) {}

NoReuseUdpConnPool::~NoReuseUdpConnPool() {
  // Cleanup any pending streams
}

void NoReuseUdpConnPool::newStream(Router::GenericConnectionPoolCallbacks* callbacks) {
  // UDP connection pools are not commonly used for HTTP traffic.
  // For now, we immediately signal a failure since this is primarily
  // a demonstration of the no-reuse pattern.
  callbacks->onPoolFailure(ConnectionPool::PoolFailureReason::LocalConnectionFailure,
                           "UDP connection pools not implemented for no-reuse policy",
                           host_);
}

bool NoReuseUdpConnPool::cancelAnyPendingStream() {
  if (stream_pending_) {
    stream_pending_ = false;
    return true;
  }
  return false;
}

Upstream::HostDescriptionConstSharedPtr NoReuseUdpConnPool::host() const {
  return host_;
}

bool NoReuseUdpConnPool::valid() const {
  return host_ != nullptr;
}

} // namespace NoReuse
} // namespace Conditional
} // namespace Http
} // namespace Upstreams
} // namespace Extensions
} // namespace Envoy
