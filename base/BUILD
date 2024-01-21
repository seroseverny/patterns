package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "tracing_example",
    srcs = ["tracing_example.cc"],
    deps = [
        ":tracing",
        "@com_google_absl//absl/flags:parse",
    ],
)

cc_library(
    name = "tracing",
    srcs = ["tracing.cc"],
    hdrs = ["tracing.h"],
    deps = [
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/log",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/strings:str_format",
    ],
)
