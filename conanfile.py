from conan import ConanFile
from conan.tools.cmake import cmake_layout


class ClavituneConan(ConanFile):
    name = "clavitune"
    version = "0.1"
    settings = "os", "arch", "compiler", "build_type"
    generators = "CMakeDeps", "CMakeToolchain"
    # alsa-lib dynamically loads its PCM/CTL plugins (hw, pulse, default, ...)
    # at runtime and locates its own plugin directory via dlinfo() on itself.
    # Statically linking it breaks that self-location trick (segfault in
    # snd_dlopen), so it must be built shared.
    default_options = {"libalsa/*:shared": True}

    def requirements(self):
        self.requires("stk/5.0.1")

    def layout(self):
        cmake_layout(self)
