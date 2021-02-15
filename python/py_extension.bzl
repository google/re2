# Copyright 2021 The RE2 Authors.  All Rights Reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

load("@rules_cc//cc:action_names.bzl", "C_COMPILE_ACTION_NAME")
load("@rules_cc//cc:find_cc_toolchain.bzl", "find_cc_toolchain")

def _compile_impl(ctx):
    src = ctx.attr.srcs[0].files.to_list()[0]
    out = ctx.actions.declare_file(ctx.label.name)

    #cc_toolchain = ctx.toolchains["@rules_cc//cc:toolchain_type"]
    cc_toolchain = find_cc_toolchain(ctx)

    feature_configuration = cc_common.configure_features(
        ctx = ctx,
        cc_toolchain = cc_toolchain,
        requested_features = ctx.features + ["supports_pic"],
        unsupported_features = ctx.disabled_features,
    )

    executable = cc_common.get_tool_for_action(
        feature_configuration = feature_configuration,
        action_name = C_COMPILE_ACTION_NAME,
    )

    variables = cc_common.create_compile_variables(
        cc_toolchain = cc_toolchain,
        feature_configuration = feature_configuration,
        source_file = src.path,
        output_file = out.path,
        user_compile_flags = ctx.fragments.cpp.copts,
        include_directories = depset(
            transitive = [
                dep[CcInfo].compilation_context.includes
                for dep in ctx.attr.deps
            ],
        ),
        quote_include_directories = depset(
            transitive = [
                dep[CcInfo].compilation_context.quote_includes
                for dep in ctx.attr.deps
            ],
        ),
        system_include_directories = depset(
            transitive = [
                dep[CcInfo].compilation_context.system_includes
                for dep in ctx.attr.deps
            ],
        ),
        framework_include_directories = depset(
            transitive = [
                dep[CcInfo].compilation_context.framework_includes
                for dep in ctx.attr.deps
            ],
        ),
        preprocessor_defines = depset(
            transitive = [
                dep[CcInfo].compilation_context.defines
                for dep in ctx.attr.deps
            ],
        ),
        use_pic = True,
    )

    arguments = cc_common.get_memory_inefficient_command_line(
        feature_configuration = feature_configuration,
        action_name = C_COMPILE_ACTION_NAME,
        variables = variables,
    )

    env = cc_common.get_environment_variables(
        feature_configuration = feature_configuration,
        action_name = C_COMPILE_ACTION_NAME,
        variables = variables,
    )

    py_toolchain = ctx.toolchains["@rules_python//python:toolchain_type"]
    py3_runtime = py_toolchain.py3_runtime

    ctx.actions.run(
        outputs = [out],
        inputs = depset(
            direct = [
                src,
                py3_runtime.interpreter,
                ctx.attr._inject_copts.files.to_list()[0],
            ],
            transitive = [
                dep[CcInfo].compilation_context.headers
                for dep in ctx.attr.deps
            ],
        ),
        executable = py3_runtime.interpreter.path,
        arguments = (
            [
                ctx.attr._inject_copts.files.to_list()[0].path,
                executable,
            ] +
            arguments
        ),
        env = env,
    )

    return [
        DefaultInfo(
            files = depset(
                direct = [
                    out,
                ],
            ),
        ),
    ]

_compile = rule(
    implementation = _compile_impl,
    attrs = {
        "srcs": attr.label_list(
            allow_files = [".cc"],
            mandatory = True,
        ),
        "deps": attr.label_list(
            providers = [CcInfo],
        ),
        "_cc_toolchain": attr.label(
            default = Label("@rules_cc//cc:current_cc_toolchain"),
        ),
        "_inject_copts": attr.label(
            default = Label("//python:inject_copts.py"),
            allow_single_file = True,
        ),
    },
    fragments = ["cpp"],
    toolchains = [
        "@rules_cc//cc:toolchain_type",
        "@rules_python//python:toolchain_type",
    ],
    incompatible_use_toolchain_transition = True,
)

def py_extension(name, srcs, deps):
    _compile(
        name = name + ".pic.o",
        srcs = srcs,
        deps = deps,
    )
    native.cc_binary(
        name = name + ".so",
        srcs = [":" + name + ".pic.o"],
        deps = deps,
        linkshared = True,
        linkstatic = True,
    )
