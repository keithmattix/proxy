# Copyright 2025 Istio Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
################################################################################
#

load(
    "@envoy//bazel:envoy_build_system.bzl",
    "envoy_cc_extension",
    "envoy_cc_library",
    "envoy_extension_package",
)

licenses(["notice"])

envoy_extension_package()

envoy_cc_extension(
    name = "config",
    srcs = [
        "config.cc",
    ],
    hdrs = [
        "config.h",
    ],
    repository = "@envoy",
    visibility = ["//visibility:public"],
    deps = [
        ":upstream_request_lib",
    ],
)

envoy_cc_library(
    name = "upstream_request_lib",
    srcs = [
    ],
    hdrs = [
    ],
    repository = "@envoy",
    visibility = ["//visibility:public"],
    deps = [
        ":api_config_cc_proto",
        "@envoy//envoy/http:codes_interface",
        "@envoy//envoy/http:conn_pool_interface",
        "@envoy//envoy/http:filter_interface",
        "@envoy//envoy/router:router_interface",
        "@envoy//envoy/tcp:conn_pool_interface",
        "@envoy//envoy/upstream:thread_local_cluster_interface",
        "@envoy//source/common/http:conn_pool_base_lib",
        "@envoy//source/common/router:router_lib",
        "@envoy//source/common/stream_info:stream_info_lib",
        "@envoy//source/common/upstream:upstream_lib",
        "@envoy//source/extensions/upstreams/http/http:upstream_request_lib",
        "@envoy//source/extensions/upstreams/http/tcp:upstream_request_lib",
        "@envoy//source/extensions/upstreams/http/udp:upstream_request_lib",
        "@envoy//source/common/common:minimal_logger_lib"
    ],
)

cc_proto_library(
    name = "api_config_cc_proto",
    deps = ["api_config"],
)

proto_library(
    name = "api_config",
    srcs = ["config.proto"],
    deps = [
        "@com_google_protobuf//:duration_proto",
        "@envoy_api//envoy/type/metadata/v3:pkg",
    ],
)
