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
#include "envoy/http/conn_pool.h"
#include "envoy/tcp/conn_pool.h"
#include "envoy/upstream/upstream.h"

namespace Envoy {
namespace Extensions {
namespace Upstreams {
namespace Http {
namespace Conditional {
namespace NoReuse {

/**
 * A wrapper for GenericConnPool that prevents connection reuse by always creating
 * new connections and immediately closing them after use.
 */
template<typename BaseConnPool>
class NoReuseConnPoolWrapper : public Router::GenericConnPool {
public:
  NoReuseConnPoolWrapper(std::unique_ptr<BaseConnPool> wrapped_pool)
      : wrapped_pool_(std::move(wrapped_pool)) {}

  // Router::GenericConnPool
  void newStream(Router::GenericConnectionPoolCallbacks* callbacks) override {
    // Store callbacks to intercept connection ready events
    callbacks_ = callbacks;
    wrapped_callbacks_ = std::make_unique<WrappedCallbacks>(*this);
    wrapped_pool_->newStream(wrapped_callbacks_.get());
  }

  bool cancelAnyPendingStream() override {
    return wrapped_pool_->cancelAnyPendingStream();
  }

  Upstream::HostDescriptionConstSharedPtr host() const override {
    return wrapped_pool_->host();
  }

  bool valid() const override {
    return wrapped_pool_->valid();
  }

private:
  class WrappedCallbacks : public Router::GenericConnectionPoolCallbacks {
  public:
    WrappedCallbacks(NoReuseConnPoolWrapper& parent) : parent_(parent) {}

    void onPoolReady(std::unique_ptr<Router::GenericUpstream>&& upstream,
                     Upstream::HostDescriptionConstSharedPtr host,
                     const Network::ConnectionInfoProvider& address_provider,
                     StreamInfo::StreamInfo& info,
                     absl::optional<Envoy::Http::Protocol> protocol) override {
      // Wrap the upstream to ensure connection is closed after use
      auto no_reuse_upstream = std::make_unique<NoReuseUpstreamWrapper>(std::move(upstream));
      parent_.callbacks_->onPoolReady(std::move(no_reuse_upstream), host, address_provider, info, protocol);
    }

    void onPoolFailure(ConnectionPool::PoolFailureReason reason,
                       absl::string_view transport_failure_reason,
                       Upstream::HostDescriptionConstSharedPtr host) override {
      parent_.callbacks_->onPoolFailure(reason, transport_failure_reason, host);
    }

    Router::UpstreamToDownstream& upstreamToDownstream() override {
      return parent_.callbacks_->upstreamToDownstream();
    }

  private:
    NoReuseConnPoolWrapper& parent_;
  };

  class NoReuseUpstreamWrapper : public Router::GenericUpstream {
  public:
    NoReuseUpstreamWrapper(std::unique_ptr<Router::GenericUpstream> wrapped_upstream)
        : wrapped_upstream_(std::move(wrapped_upstream)) {}

    // Router::GenericUpstream interface
    void encodeData(Buffer::Instance& data, bool end_stream) override {
      wrapped_upstream_->encodeData(data, end_stream);
    }

    void encodeMetadata(const Envoy::Http::MetadataMapVector& metadata_map_vector) override {
      wrapped_upstream_->encodeMetadata(metadata_map_vector);
    }

    Envoy::Http::Status encodeHeaders(const Envoy::Http::RequestHeaderMap& headers, bool end_stream) override {
      auto result = wrapped_upstream_->encodeHeaders(headers, end_stream);
      if (end_stream) {
        closeConnection();
      }
      return result;
    }

    void encodeTrailers(const Envoy::Http::RequestTrailerMap& trailers) override {
      wrapped_upstream_->encodeTrailers(trailers);
      // Force connection closure after trailers (implies end_stream)
      closeConnection();
    }

    void enableTcpTunneling() override {
      wrapped_upstream_->enableTcpTunneling();
    }

    void readDisable(bool disable) override {
      wrapped_upstream_->readDisable(disable);
    }

    void resetStream() override {
      wrapped_upstream_->resetStream();
      // Force connection closure after reset
      closeConnection();
    }

    void setAccount(Buffer::BufferMemoryAccountSharedPtr account) override {
      wrapped_upstream_->setAccount(account);
    }

    const StreamInfo::BytesMeterSharedPtr& bytesMeter() override {
      return wrapped_upstream_->bytesMeter();
    }

    ~NoReuseUpstreamWrapper() override {
      // Ensure connection is closed when upstream is destroyed
      closeConnection();
    }

  private:
    void closeConnection() {
      // Force stream reset to close the connection
      if (wrapped_upstream_) {
        wrapped_upstream_->resetStream();
        // Additional connection-specific cleanup would go here
      }
    }

    std::unique_ptr<Router::GenericUpstream> wrapped_upstream_;
  };

  std::unique_ptr<BaseConnPool> wrapped_pool_;
  Router::GenericConnectionPoolCallbacks* callbacks_{nullptr};
  std::unique_ptr<WrappedCallbacks> wrapped_callbacks_;
};

} // namespace NoReuse
} // namespace Conditional
} // namespace Http
} // namespace Upstreams
} // namespace Extensions
} // namespace Envoy
