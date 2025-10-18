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
#include "envoy/tcp/conn_pool.h"
#include "envoy/upstream/thread_local_cluster.h"

namespace Envoy {
namespace Extensions {
namespace Upstreams {
namespace Http {
namespace Conditional {
namespace NoReuse {

/**
 * A TCP connection pool that never reuses connections. Each request
 * gets a fresh connection which is immediately closed after use.
 *
 * This implements Router::GenericConnPool interface and creates
 * fresh TCP connection pools for each request.
 */
class NoReuseTcpConnPool : public Router::GenericConnPool,
                            public Envoy::Tcp::ConnectionPool::Callbacks {
public:
  NoReuseTcpConnPool(
      Upstream::HostConstSharedPtr host,
      Upstream::ThreadLocalCluster& thread_local_cluster,
      Upstream::ResourcePriority priority,
      Upstream::LoadBalancerContext* ctx);

  ~NoReuseTcpConnPool() override;

  // Router::GenericConnPool
  void newStream(Router::GenericConnectionPoolCallbacks* callbacks) override;
  bool cancelAnyPendingStream() override;
  Upstream::HostDescriptionConstSharedPtr host() const override;
  bool valid() const override;

  // Envoy::Tcp::ConnectionPool::Callbacks
  void onPoolFailure(
      Envoy::Tcp::ConnectionPool::PoolFailureReason reason,
      absl::string_view transport_failure_reason,
      Upstream::HostDescriptionConstSharedPtr host) override;
  void onPoolReady(
      Envoy::Tcp::ConnectionPool::ConnectionDataPtr&& conn_data,
      Upstream::HostDescriptionConstSharedPtr host) override;

private:
  absl::optional<Envoy::Upstream::TcpPoolData> createFreshConnectionPool();

  Upstream::HostConstSharedPtr host_;
  Upstream::ThreadLocalCluster& thread_local_cluster_;
  Upstream::ResourcePriority priority_;
  Upstream::LoadBalancerContext* ctx_;
  Router::GenericConnectionPoolCallbacks* callbacks_{nullptr};
  Envoy::Tcp::ConnectionPool::Cancellable* upstream_handle_{nullptr};

  /**
   * Custom upstream that immediately closes TCP connections after use.
   */
  class NoReuseTcpUpstream : public Router::GenericUpstream {
  public:
    NoReuseTcpUpstream(
        Envoy::Tcp::ConnectionPool::ConnectionDataPtr&& conn_data,
        Upstream::HostDescriptionConstSharedPtr host);

    ~NoReuseTcpUpstream() override;

    // Router::GenericUpstream
    Envoy::Http::Status encodeHeaders(const Envoy::Http::RequestHeaderMap& headers, bool end_stream) override;
    void encodeData(Buffer::Instance& data, bool end_stream) override;
    void encodeTrailers(const Envoy::Http::RequestTrailerMap& trailers) override;
    void encodeMetadata(const Envoy::Http::MetadataMapVector& metadata_map_vector) override;
    void enableTcpTunneling() override;
    void readDisable(bool disable) override;
    void resetStream() override;
    void setAccount(Buffer::BufferMemoryAccountSharedPtr account) override;
    const StreamInfo::BytesMeterSharedPtr& bytesMeter() override;

  private:
    void closeConnection();

    Envoy::Tcp::ConnectionPool::ConnectionDataPtr conn_data_;
    Upstream::HostDescriptionConstSharedPtr host_;
    bool connection_closed_{false};
  };
};

} // namespace NoReuse
} // namespace Conditional
} // namespace Http
} // namespace Upstreams
} // namespace Extensions
} // namespace Envoy
