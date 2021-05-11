# How to build re2 on Windows platforms

You can build RE2 for Windows with Microsoft Visual Studio by the Bazel build tool <https://bazel.build/>

1. Download Bazel Windows binary at <https://github.com/bazelbuild/bazel/releases> (scroll down and look for an .exe file)
2. Put the Bazel binary in the root directory of RE2, or add a directory with the Bazel binary to %PATH%.
3. Run the Visual Studio command prompt from the Start Menu, e.g., the the "x64 Native Tools Command Prompt"
4. In the command prompt, go to your RE2 root directory, e.g. `cd c:\gitrepos\re2`
5. Run `bazel.exe build :all`
6. Bazel will create the following subdirectoris in you RE2 root directory: `bazel-bin`, `bazel-out`, `bazel-re2`, `bazel-testlogs`. You will find the compiled RE2 binaries, i.e. the library and the test programs (.lib, .pdb, .exe) at `bazel-bin`. For example, you may run `regexp_benchmark.exe` from `bazel-bin`
