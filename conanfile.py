from conan import ConanFile
from conan.tools.cmake import cmake_layout


class ClavituneConan(ConanFile):
    name = "clavitune"
    version = "0.1"
    settings = "os", "arch", "compiler", "build_type"
    generators = "CMakeDeps", "CMakeToolchain"

    def requirements(self):
        self.requires("stk/5.0.1")

    def layout(self):
        cmake_layout(self)
