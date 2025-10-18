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

#include "source/extensions/upstreams/http/conditional/no_reuse/no_reuse_tcp_conn_pool.h"

namespace Envoy {
namespace Extensions {
namespace Upstreams {
namespace Http {
namespace Conditional {
namespace NoReuse {

NoReuseTcpConnPool::NoReuseTcpConnPool(
    Upstream::HostConstSharedPtr host,
    Upstream::ThreadLocalCluster& thread_local_cluster,
    Upstream::ResourcePriority priority,
    Upstream::LoadBalancerContext* ctx)
    : host_(host), thread_local_cluster_(thread_local_cluster), priority_(priority), ctx_(ctx) {}

NoReuseTcpConnPool::~NoReuseTcpConnPool() {
  if (upstream_handle_) {
    upstream_handle_->cancel(Envoy::Tcp::ConnectionPool::CancelPolicy::Default);
  }
}

void NoReuseTcpConnPool::newStream(Router::GenericConnectionPoolCallbacks* callbacks) {
  callbacks_ = callbacks;

  // Create a fresh TCP connection pool for each request
  auto conn_pool_data = createFreshConnectionPool();
  if (!conn_pool_data.has_value()) {
    callbacks_->onPoolFailure(ConnectionPool::PoolFailureReason::LocalConnectionFailure,
                               "Failed to create TCP connection pool", host_);
    return;
  }

  // Request a new connection from the fresh pool
  upstream_handle_ = conn_pool_data.value().newConnection(*this);
  if (!upstream_handle_) {
    callbacks_->onPoolFailure(ConnectionPool::PoolFailureReason::LocalConnectionFailure,
                               "Failed to create TCP connection", host_);
  }
}

bool NoReuseTcpConnPool::cancelAnyPendingStream() {
  if (upstream_handle_) {
    upstream_handle_->cancel(Envoy::Tcp::ConnectionPool::CancelPolicy::Default);
    upstream_handle_ = nullptr;
    return true;
  }
  return false;
}

Upstream::HostDescriptionConstSharedPtr NoReuseTcpConnPool::host() const {
  return host_;
}

bool NoReuseTcpConnPool::valid() const {
  return host_ != nullptr;
}

void NoReuseTcpConnPool::onPoolFailure(
    Envoy::Tcp::ConnectionPool::PoolFailureReason reason,
    absl::string_view transport_failure_reason,
    Upstream::HostDescriptionConstSharedPtr host) {
  upstream_handle_ = nullptr;

  // Convert TCP pool failure reason to Router pool failure reason
  ConnectionPool::PoolFailureReason router_reason;
  switch (reason) {
    case Envoy::Tcp::ConnectionPool::PoolFailureReason::Overflow:
      router_reason = ConnectionPool::PoolFailureReason::Overflow;
      break;
    case Envoy::Tcp::ConnectionPool::PoolFailureReason::LocalConnectionFailure:
      router_reason = ConnectionPool::PoolFailureReason::LocalConnectionFailure;
      break;
    case Envoy::Tcp::ConnectionPool::PoolFailureReason::RemoteConnectionFailure:
      router_reason = ConnectionPool::PoolFailureReason::RemoteConnectionFailure;
      break;
    case Envoy::Tcp::ConnectionPool::PoolFailureReason::Timeout:
      router_reason = ConnectionPool::PoolFailureReason::Timeout;
      break;
    default:
      router_reason = ConnectionPool::PoolFailureReason::LocalConnectionFailure;
      break;
  }

  callbacks_->onPoolFailure(router_reason, transport_failure_reason, host);
}

void NoReuseTcpConnPool::onPoolReady(
    Envoy::Tcp::ConnectionPool::ConnectionDataPtr&& conn_data,
    Upstream::HostDescriptionConstSharedPtr host) {
  upstream_handle_ = nullptr;

  auto upstream = std::make_unique<NoReuseTcpUpstream>(std::move(conn_data), host);

  // TODO: Implement proper StreamInfo and ConnectionInfoProvider
  // For now, we need to work around the interface mismatch
  // This is a simplified implementation that may need further work
  PANIC("TCP connection pool implementation needs further work to handle Router callbacks");
}

absl::optional<Envoy::Upstream::TcpPoolData> NoReuseTcpConnPool::createFreshConnectionPool() {
  // Always create a fresh TCP connection pool to ensure no reuse
  return thread_local_cluster_.tcpConnPool(host_, priority_, ctx_);
}

// NoReuseTcpUpstream implementation

NoReuseTcpConnPool::NoReuseTcpUpstream::NoReuseTcpUpstream(
    Envoy::Tcp::ConnectionPool::ConnectionDataPtr&& conn_data,
    Upstream::HostDescriptionConstSharedPtr host)
    : conn_data_(std::move(conn_data)), host_(host) {}

NoReuseTcpConnPool::NoReuseTcpUpstream::~NoReuseTcpUpstream() {
  if (!connection_closed_) {
    closeConnection();
  }
}

Envoy::Http::Status NoReuseTcpConnPool::NoReuseTcpUpstream::encodeHeaders(
    const Envoy::Http::RequestHeaderMap&, bool end_stream) {
  // TCP connections don't encode HTTP headers, but we can use this to signal data transfer
  // In practice, this would need to be adapted for the specific tunneling protocol
  if (end_stream) {
    closeConnection();
  }
  return Envoy::Http::okStatus();
}

void NoReuseTcpConnPool::NoReuseTcpUpstream::encodeData(Buffer::Instance& data, bool end_stream) {
  if (conn_data_ && conn_data_->connection().state() == Network::Connection::State::Open) {
    conn_data_->connection().write(data, end_stream);
  }
  if (end_stream) {
    closeConnection();
  }
}

void NoReuseTcpConnPool::NoReuseTcpUpstream::encodeTrailers(
    const Envoy::Http::RequestTrailerMap&) {
  // TCP connections don't have trailers, but this signals end of stream
  closeConnection();
}

void NoReuseTcpConnPool::NoReuseTcpUpstream::encodeMetadata(
    const Envoy::Http::MetadataMapVector&) {
  // TCP connections don't handle HTTP metadata
}

void NoReuseTcpConnPool::NoReuseTcpUpstream::enableTcpTunneling() {
  // TCP connections are already tunneled by nature
}

void NoReuseTcpConnPool::NoReuseTcpUpstream::readDisable(bool) {
  // For TCP connections, we can't easily disable reading without affecting the connection
  // This is typically handled by the connection manager
}

void NoReuseTcpConnPool::NoReuseTcpUpstream::resetStream() {
  closeConnection();
}

void NoReuseTcpConnPool::NoReuseTcpUpstream::setAccount(
    Buffer::BufferMemoryAccountSharedPtr) {
  // TCP connections don't typically need memory accounting in this context
}

const StreamInfo::BytesMeterSharedPtr& NoReuseTcpConnPool::NoReuseTcpUpstream::bytesMeter() {
  // Return a dummy bytes meter for TCP connections
  static StreamInfo::BytesMeterSharedPtr dummy_meter = nullptr;
  if (!dummy_meter) {
    // Create a dummy implementation - just return an empty shared_ptr for now
    // since we don't actually need to measure bytes for this use case
  }
  return dummy_meter;
}

void NoReuseTcpConnPool::NoReuseTcpUpstream::closeConnection() {
  if (conn_data_ && !connection_closed_) {
    connection_closed_ = true;
    // Force close the connection to prevent reuse
    conn_data_->connection().close(Network::ConnectionCloseType::NoFlush);
  }
}

} // namespace NoReuse
} // namespace Conditional
} // namespace Http
} // namespace Upstreams
} // namespace Extensions
} // namespace Envoy
